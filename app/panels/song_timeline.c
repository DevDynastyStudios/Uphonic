typedef enum
{
    UPH_BLOCK_INTERACTION_NONE,
    UPH_BLOCK_INTERACTION_MOVE,
    UPH_BLOCK_INTERACTION_RESIZE_LEFT,
    UPH_BLOCK_INTERACTION_RESIZE_RIGHT
}
Uph_BlockInteractionMode;

typedef struct
{
    Naui_Vec2 zoom;
    Naui_Vec2 scroll;
    Naui_Vec2 last_mouse_pos;

    Uph_BlockInteractionMode block_interaction_mode;
    uint32_t interaction_track_index;
    uint32_t interaction_block_index;
    float interaction_grab_beat_offset;
    double interaction_orig_start_beat;
    double interaction_orig_length_beats;
    double interaction_orig_start_offset_beats;
}
Uph_SongTimelineData;

typedef struct
{
    Uph_SongTimelineData *timeline_data;
    Naui_PanelID panel_id;
    uint32_t track_index;
}
Uph_TrackCustomDrawData;

typedef struct
{
    Uph_SongTimelineData *timeline_data;
}
Uph_GridCustomDrawData,
Uph_OverlayCustomDrawData,
Uph_RulerCustomDrawData;

NAUI_PANEL_WITH_DATA(song_timeline, Uph_SongTimelineData)

static void song_timeline_on_attach(Uph_SongTimelineData *timeline_data)
{
    timeline_data->zoom = naui_vec2(NAUI_DPI(24.0f), NAUI_DPI(80.0f));
    timeline_data->scroll = naui_vec2(0.0f, 0.0f);
    timeline_data->last_mouse_pos = naui_vec2((float)naui_mouse_x(), (float)naui_mouse_y());

    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Song Timeline");
    Leaf_Color colors[] = {
        leaf_hex(0x99A5F6),
        leaf_hex(0x9FC2EF),
        leaf_hex(0x9AFAE5),
        leaf_hex(0xD4FC7A),
        leaf_hex(0xF9D95C),
        leaf_hex(0xE5A17B),
        leaf_hex(0xE797A4),
        leaf_hex(0xC272D9)
    };

    Uph_Sample sample_resource = uph_audio_engine_load_sample(NAUI_PATH("put your file here"));
    naui_list_push(uph_state.project.samples, sample_resource);

    for (int i = 0; i < 8; i++)
    {
        Uph_Track track = { 0 };
        track.color = colors[i];
        track.volume = 1.0f;
        track.name = naui_string_from_cstr("Untitled Track");

        if (i == 0)
        {
            track.type = UPH_TRACK_AUDIO;
            Uph_TimelineBlock block = {
                .length_beats = 10.0,
                .start_beat = 5.0,
                .stretch_scale = 1.0,
                .resource_index = 0,
                .type = UPH_TIMELINE_BLOCK_SAMPLE
            };
            naui_list_push(track.blocks, block);
        }


        naui_list_push(uph_state.project.tracks, track);
    }
}

static void song_timeline_on_detach(Uph_SongTimelineData *timeline_data)
{
    
}

static void song_timeline_update_zoom_x(Uph_SongTimelineData *timeline_data, Naui_Vec2 mouse_pos, Leaf_BoundingBox viewport, float track_area_x)
{
    float scroll_delta = naui_mouse_scroll_delta();
    if (scroll_delta == 0.0f)
        return;

    float old_zoom_x = timeline_data->zoom.x;

    const float zoom_speed = 0.08f;
    float zoom_factor = powf(1.0f + zoom_speed, scroll_delta);
    float new_zoom_x = old_zoom_x * zoom_factor;

    new_zoom_x = fminf(fmaxf(new_zoom_x, NAUI_DPI(1.0f)), NAUI_DPI(500.0f));

    float local_mouse_x = (mouse_pos.x - viewport.x) - track_area_x;
    float beat_under_mouse = (local_mouse_x - timeline_data->scroll.x) / old_zoom_x;
    timeline_data->scroll.x = local_mouse_x - beat_under_mouse * new_zoom_x;

    timeline_data->zoom.x = new_zoom_x;
}

static void song_timeline_update_scroll_y_wheel(Uph_SongTimelineData *timeline_data)
{
    timeline_data->scroll.y += naui_mouse_scroll_delta() * 10.0f;
}

static void song_timeline_update_zoom_y(Uph_SongTimelineData *timeline_data, Naui_Vec2 mouse_pos, Leaf_BoundingBox viewport, float dy)
{
    const float zoom_y_speed = 0.005f;
    float zoom_factor = powf(1.0f + zoom_y_speed, -dy);
    float old_zoom_y = timeline_data->zoom.y;
    float new_zoom_y = old_zoom_y * zoom_factor;
    new_zoom_y = fminf(fmaxf(new_zoom_y, NAUI_DPI(32.0f)), NAUI_DPI(300.0f));

    if (new_zoom_y == old_zoom_y)
        return;

    const float track_list_y = NAUI_DPI(24.0f);
    float local_mouse_y = (mouse_pos.y - viewport.y) - track_list_y;

    float row_under_mouse = (local_mouse_y - timeline_data->scroll.y) / old_zoom_y;

    timeline_data->scroll.y = local_mouse_y - row_under_mouse * new_zoom_y;
    timeline_data->zoom.y = new_zoom_y;
}

static void song_timeline_update_pan(Uph_SongTimelineData *timeline_data, float dx, float dy)
{
    timeline_data->scroll.x += dx;
    timeline_data->scroll.y += dy;
}

static void song_timeline_update_middle_mouse_drag(Uph_SongTimelineData *timeline_data, Naui_Vec2 mouse_pos, Leaf_BoundingBox viewport, bool ctrl_down)
{
    if (!naui_mouse_down(NAUI_MOUSE_MIDDLE))
        return;

    float dx = mouse_pos.x - timeline_data->last_mouse_pos.x;
    float dy = mouse_pos.y - timeline_data->last_mouse_pos.y;

    if (ctrl_down)
        song_timeline_update_zoom_y(timeline_data, mouse_pos, viewport, dy);
    else
        song_timeline_update_pan(timeline_data, dx, dy);
}

static void song_timeline_clamp_scroll(Uph_SongTimelineData *timeline_data, float viewport_height)
{
    timeline_data->scroll.x = fminf(timeline_data->scroll.x, 0.0f);

    float content_height = naui_list_len(uph_state.project.tracks) * timeline_data->zoom.y;
    float min_scroll_y = fminf(0.0f, viewport_height - content_height);

    timeline_data->scroll.y = fminf(timeline_data->scroll.y, 0.0f);
    timeline_data->scroll.y = fmaxf(timeline_data->scroll.y, min_scroll_y);
}

static Leaf_Color uph_saturate_color(Leaf_Color color, float amount)
{
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;

    float gray = r * 0.299f + g * 0.587f + b * 0.114f;

    r = gray + (r - gray) * amount;
    g = gray + (g - gray) * amount;
    b = gray + (b - gray) * amount;

    r = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
    g = g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g);
    b = b < 0.0f ? 0.0f : (b > 1.0f ? 1.0f : b);

    return (Leaf_Color){
        .r = (uint8_t)(r * 255.0f),
        .g = (uint8_t)(g * 255.0f),
        .b = (uint8_t)(b * 255.0f),
        .a = color.a
    };
}

static inline Leaf_Color uph_darken_color(Leaf_Color color, float amount)
{
    return (Leaf_Color){
        .r = (uint8_t)(color.r * amount),
        .g = (uint8_t)(color.g * amount),
        .b = (uint8_t)(color.b * amount),
        .a = color.a
    };
}

static inline Leaf_Color uph_apply_opacity_to_color(Leaf_Color color, float opacity)
{
    color.a = (uint8_t)(color.a * opacity);
    return color;
}

static void uph_draw_sample_waveform(const Uph_Sample *resource, const Uph_TimelineBlock *block, Naui_Vec2 position, Naui_Vec2 size, Leaf_Color color)
{
    if (!resource->frames || resource->frame_count == 0)
        return;

    const uint64_t peak_count = (uint64_t)naui_list_len(resource->waveform_peaks);
    if (peak_count == 0)
        return;

    const float bpm = uph_state.project.bpm;

    const double seconds_per_beat = 60.0 / (double)bpm;
    const double timeline_seconds = block->length_beats * seconds_per_beat;
    const double stretch_scale = block->stretch_scale > 0.0 ? block->stretch_scale : 1.0;
    const double audio_seconds = timeline_seconds / stretch_scale;
    const double start_offset_seconds = block->start_offset_beats * seconds_per_beat / stretch_scale;

    const double sample_rate = (double)resource->sample_rate;
    uint64_t frame_start_global = (uint64_t)(start_offset_seconds * sample_rate);
    uint64_t frame_count_visible = (uint64_t)(audio_seconds * sample_rate);

    if (frame_start_global >= resource->frame_count)
        return;
    if (frame_start_global + frame_count_visible > resource->frame_count)
        frame_count_visible = resource->frame_count - frame_start_global;
    if (frame_count_visible == 0)
        return;

    uint64_t bin_start = frame_start_global / UPH_SAMPLE_FRAME_COUNT;
    uint64_t bin_end = (frame_start_global + frame_count_visible) / UPH_SAMPLE_FRAME_COUNT;
    if (bin_end >= peak_count)
        bin_end = peak_count - 1;
    if (bin_end <= bin_start)
        bin_end = bin_start + 1;

    const float header_height = NAUI_DPI(16.0f);
    const float top = position.y + header_height;
    const float available_height = size.y - header_height;
    if (available_height <= 0.0f)
        return;

    const float mid_y = top + available_height * 0.5f;
    const float half_height = available_height * 0.5f * 0.9f;

    if (size.x <= 0.0f)
    {
        return;
    }

    const uint64_t bin_count_visible = bin_end - bin_start + 1;
    const float pixels_per_bin = size.x / (float)bin_count_visible;

    uint64_t bin_step = 1;
    if (pixels_per_bin < 1.0f)
        bin_step = (uint64_t)(1.0f / pixels_per_bin) + 1;

    Naui_Vec2 prev_point = { 0 };
    bool has_prev = false;
    bool use_max = true;

    const float rect_left = position.x;
    const float rect_right = position.x + size.x;
    const float rect_top = position.y;
    const float rect_bottom = position.y + size.y;

    for (uint64_t bin = bin_start; bin <= bin_end; bin += bin_step)
    {
        uint64_t range_end = bin + bin_step;
        if (range_end > bin_end + 1) range_end = bin_end + 1;

        float min_v = 1.0f;
        float max_v = -1.0f;
        for (uint64_t b = bin; b < range_end && b < peak_count; b++)
        {
            Uph_WaveformPeak p = resource->waveform_peaks[b];
            if (p.min < min_v) min_v = p.min;
            if (p.max > max_v) max_v = p.max;
        }

        float x = position.x + (float)(bin - bin_start) * pixels_per_bin;
        float value = use_max ? max_v : min_v;
        float y = mid_y - value * half_height;

        Naui_Vec2 cur_point = naui_vec2(x, y);

        bool segment_in_rect = has_prev
            && x >= rect_left && x <= rect_right
            && prev_point.x >= rect_left && prev_point.x <= rect_right
            && y >= rect_top && y <= rect_bottom
            && prev_point.y >= rect_top && prev_point.y <= rect_bottom;

        if (segment_in_rect)
            naui_draw_line(prev_point, cur_point, color, NAUI_DPI(1.0f));

        prev_point = cur_point;
        has_prev = true;
        use_max = !use_max;
    }
}

static void song_timeline_render_timeline_block(Uph_TimelineBlock *block, Naui_Vec2 position, Naui_Vec2 size, Leaf_Color color)
{
    naui_fill_rect(
        position,
        size,
        uph_apply_opacity_to_color(color, 0.2f),
        NAUI_DPI(5.0f),
        NAUI_CORNER_ALL
    );

    naui_fill_rect(
        position,
        naui_vec2(size.x, NAUI_DPI(16.0f)),
        uph_saturate_color(uph_darken_color(color, 0.95f), 1.5f),
        NAUI_DPI(5.0f),
        NAUI_CORNER_TL | NAUI_CORNER_TR
    );

    naui_push_clip_rect(
        position.x, position.y,
        size.x, size.y
    );

    if (block->type == UPH_TIMELINE_BLOCK_SAMPLE)
    {
        const Uph_Sample *resource = &uph_state.project.samples[block->resource_index];

        const float font_size = NAUI_DPI(11.0f);
        const char *text = resource->name.data;
        naui_draw_text(naui_vec2(position.x + NAUI_DPI(3.0f), position.y + NAUI_DPI(3.0f)), text, font_size, 0, LEAF_COLOR_WHITE);
        uph_draw_sample_waveform(resource, block, position, size, color);
    }

    naui_pop_clip_rect();
}

static inline bool vec4_contains_point(Naui_Vec4 rect, Naui_Vec2 point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z && point.y >= rect.y && point.y <= rect.y + rect.w;
}

static void song_timeline_begin_block_interaction(Uph_SongTimelineData *timeline_data, uint32_t track_index, uint32_t block_index, Uph_BlockInteractionMode mode, float mouse_x, Naui_Vec2 block_pos, Naui_Vec2 block_size)
{
    Uph_Track *track = &uph_state.project.tracks[track_index];
    Uph_TimelineBlock *block = &track->blocks[block_index];

    timeline_data->block_interaction_mode = mode;
    timeline_data->interaction_track_index = track_index;
    timeline_data->interaction_block_index = block_index;
    timeline_data->interaction_orig_start_beat = block->start_beat;
    timeline_data->interaction_orig_length_beats = block->length_beats;
    timeline_data->interaction_orig_start_offset_beats = block->start_offset_beats;

    if (mode == UPH_BLOCK_INTERACTION_MOVE)
        timeline_data->interaction_grab_beat_offset = (mouse_x - block_pos.x) / timeline_data->zoom.x;
    else
        timeline_data->interaction_grab_beat_offset = 0.0f;
}

static uint32_t song_timeline_track_index_at_y(float mouse_y, Leaf_BoundingBox viewport, Uph_SongTimelineData *timeline_data)
{
    const float ruler_height = NAUI_DPI(24.0f);
    float local_y = (mouse_y - viewport.y) - ruler_height - timeline_data->scroll.y;
    if (local_y < 0.0f)
        return 0;

    uint32_t track_count = (uint32_t)naui_list_len(uph_state.project.tracks);
    uint32_t index = (uint32_t)(local_y / timeline_data->zoom.y);
    if (index >= track_count)
        index = track_count - 1;

    return index;
}

static double uph_block_max_length_beats(const Uph_TimelineBlock *block)
{
    if (block->type != UPH_TIMELINE_BLOCK_SAMPLE)
        return -1.0;

    const Uph_Sample *resource = &uph_state.project.samples[block->resource_index];
    const double seconds_per_beat = 60.0 / (double)uph_state.project.bpm;
    const double stretch_scale = block->stretch_scale > 0.0 ? block->stretch_scale : 1.0;
    const double available_seconds = (double)resource->frame_count / (double)resource->sample_rate;
    const double max_audio_seconds = available_seconds - (block->start_offset_beats * seconds_per_beat / stretch_scale);
    const double max_length_beats = (max_audio_seconds * stretch_scale) / seconds_per_beat;

    return max_length_beats;
}

static void uph_block_clamp_length(Uph_TimelineBlock *block)
{
    double max_length_beats = uph_block_max_length_beats(block);

    if (max_length_beats >= 0.0 && block->length_beats > max_length_beats)
        block->length_beats = max_length_beats > 1.0 ? floor(max_length_beats) : 1.0;

    if (block->length_beats < 1.0)
        block->length_beats = 1.0;
}

static void song_timeline_update_block_interaction(Uph_SongTimelineData *timeline_data, Leaf_BoundingBox viewport, float track_area_x)
{
    if (timeline_data->block_interaction_mode == UPH_BLOCK_INTERACTION_NONE)
        return;

    if (timeline_data->block_interaction_mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT || timeline_data->block_interaction_mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
        naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
    else
        naui_set_cursor(NAUI_CURSOR_HAND);

    if (!naui_mouse_down(NAUI_MOUSE_LEFT))
    {
        timeline_data->block_interaction_mode = UPH_BLOCK_INTERACTION_NONE;
        return;
    }

    float mouse_x = (float)naui_mouse_x();
    float mouse_y = (float)naui_mouse_y();
    float local_x = (mouse_x - viewport.x) - track_area_x;
    float beat_at_mouse = (local_x - timeline_data->scroll.x) / timeline_data->zoom.x;

    uint32_t track_index = timeline_data->interaction_track_index;
    uint32_t block_index = timeline_data->interaction_block_index;
    Uph_Track *track = &uph_state.project.tracks[track_index];
    Uph_TimelineBlock *block = &track->blocks[block_index];

    if (timeline_data->block_interaction_mode == UPH_BLOCK_INTERACTION_MOVE)
    {
        double new_start_beat = roundf(beat_at_mouse - timeline_data->interaction_grab_beat_offset);
        if (new_start_beat < 0.0)
            new_start_beat = 0.0;
        block->start_beat = new_start_beat;

        uint32_t target_track_index = song_timeline_track_index_at_y(mouse_y, viewport, timeline_data);
        Uph_Track *target_track = &uph_state.project.tracks[target_track_index];

        if (target_track_index != track_index && (target_track->type == track->type || target_track->type == UPH_TRACK_NONE))
        {
            Uph_TimelineBlock moved_block = *block;

            for (uint32_t i = block_index; i + 1 < (uint32_t)naui_list_len(track->blocks); i++)
                track->blocks[i] = track->blocks[i + 1];
            naui_list_pop(track->blocks);

            naui_list_push(target_track->blocks, moved_block);

            timeline_data->interaction_track_index = target_track_index;
            timeline_data->interaction_block_index = (uint32_t)naui_list_len(target_track->blocks) - 1;

            if (target_track->type == UPH_TRACK_NONE)
                target_track->type = track->type;
            if (naui_list_len(track->blocks) == 0)
                track->type = UPH_TRACK_NONE;
        }
    }
    else if (timeline_data->block_interaction_mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
    {
        double new_end_beat = roundf(beat_at_mouse);
        double new_length = new_end_beat - timeline_data->interaction_orig_start_beat;
        if (new_length < 1.0)
            new_length = 1.0;

        block->length_beats = new_length;
        uph_block_clamp_length(block);
    }
    else if (timeline_data->block_interaction_mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
    {
        double orig_end_beat = timeline_data->interaction_orig_start_beat + timeline_data->interaction_orig_length_beats;
        double new_start_beat = roundf(beat_at_mouse);
        if (new_start_beat > orig_end_beat - 1.0)
            new_start_beat = orig_end_beat - 1.0;
        if (new_start_beat < 0.0)
            new_start_beat = 0.0;

        double delta_beats = new_start_beat - timeline_data->interaction_orig_start_beat;
        double new_start_offset_beats = timeline_data->interaction_orig_start_offset_beats + delta_beats;

        if (new_start_offset_beats < 0.0)
        {
            delta_beats -= new_start_offset_beats;
            new_start_offset_beats = 0.0;
            new_start_beat = timeline_data->interaction_orig_start_beat + delta_beats;
        }

        block->start_beat = new_start_beat;
        block->length_beats = orig_end_beat - new_start_beat;
        block->start_offset_beats = new_start_offset_beats;
    }
}

static void song_timeline_track_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    Uph_TrackCustomDrawData *data = (Uph_TrackCustomDrawData*)user_data;
    Uph_Track *track = &uph_state.project.tracks[data->track_index];
    Naui_Vec2 zoom = data->timeline_data->zoom;
    float scroll_x = data->timeline_data->scroll.x;

    const bool playing = uph_state.interact.song_timeline_playing;
    const double playhead_beat = uph_state.interact.song_timeline_playhead_position;

    naui_push_clip_rect(box.x, box.y, box.width, box.height);
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->blocks); i++)
    {
        Uph_TimelineBlock *block = &track->blocks[i];
        uph_block_clamp_length(block);

        Naui_Vec2 positon = naui_vec2(box.x + scroll_x + block->start_beat * zoom.x, box.y);
        Naui_Vec2 size = naui_vec2(zoom.x * block->length_beats, box.height);

        if (playing && !(track->state & UPH_TRACK_STATE_MUTE) && playhead_beat >= block->start_beat && playhead_beat < block->start_beat + block->length_beats)
            track->glow_effect = 1.0f;

        if (positon.x + size.x < box.x || positon.x > box.x + box.width)
            continue;

        song_timeline_render_timeline_block(block, positon, size, (track->state & UPH_TRACK_STATE_MUTE) ? uph_apply_opacity_to_color(track->color, 0.35f) : track->color);

        bool hovered = data->timeline_data->block_interaction_mode == UPH_BLOCK_INTERACTION_NONE
            && naui_panel_hovered(data->panel_id)
            && vec4_contains_point(naui_vec4(positon.x, positon.y, size.x, size.y), naui_vec2(naui_mouse_x(), naui_mouse_y()));
        if (hovered)
        {
            const float edge_grab_width = NAUI_DPI(6.0f);
            float mouse_x = (float)naui_mouse_x();
            bool on_left_edge = mouse_x <= positon.x + edge_grab_width;
            bool on_right_edge = mouse_x >= positon.x + size.x - edge_grab_width;

            if (on_left_edge || on_right_edge)
                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            else
                naui_set_cursor(NAUI_CURSOR_HAND);

            if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
            {
                Uph_BlockInteractionMode mode = UPH_BLOCK_INTERACTION_MOVE;
                if (on_left_edge)
                    mode = UPH_BLOCK_INTERACTION_RESIZE_LEFT;
                else if (on_right_edge)
                    mode = UPH_BLOCK_INTERACTION_RESIZE_RIGHT;

                song_timeline_begin_block_interaction(data->timeline_data, data->track_index, i, mode, mouse_x, positon, size);
            }
        }
    }
    naui_pop_clip_rect();
}

static void song_timeline_grid_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    Uph_GridCustomDrawData *data = (Uph_GridCustomDrawData*)user_data;

    const float pixels_per_beat = data->timeline_data->zoom.x;
    const int   beats_per_bar   = 4;
    const float line_width = NAUI_DPI(1.5f);
    const float scroll_x = data->timeline_data->scroll.x;

    const Leaf_Color beat_color = naui_theme_color("uph_track_grid_beat_color");
    const Leaf_Color bar_color  = naui_theme_color("uph_track_grid_bar_color");

    const float offset_beats = -scroll_x / pixels_per_beat;
    const int start_beat_index = (int)floorf(offset_beats);
    const int beat_count = (int)(box.width / pixels_per_beat) + 2;

    const float min_beat_line_spacing_px = NAUI_DPI(6.0f);
    const bool draw_beat_lines = pixels_per_beat >= min_beat_line_spacing_px;

    naui_push_clip_rect(box.x, box.y, box.width, box.height);

    for (int i = start_beat_index; i < start_beat_index + beat_count; i++)
    {
        bool is_bar_line = ((i % beats_per_bar) + beats_per_bar) % beats_per_bar == 0;

        if (!is_bar_line && !draw_beat_lines)
            continue;

        float x = box.x + scroll_x + i * pixels_per_beat;
        if (x < box.x - pixels_per_beat) continue;
        if (x > box.x + box.width) break;

        naui_draw_line(
            naui_vec2(x, box.y),
            naui_vec2(x, box.y + box.height),
            is_bar_line ? bar_color : beat_color,
            line_width
        );
    }

    naui_pop_clip_rect();
}

static void song_timeline_overlay_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    if (uph_state.interact.song_timeline_playhead_position <= 0.0)
        return;
    Uph_OverlayCustomDrawData *data = (Uph_OverlayCustomDrawData*)user_data;
    const float header_width = NAUI_DPI(naui_theme_float("uph_track_header_width"));
    const float header_padding = NAUI_DPI(naui_theme_vec2("uph_track_header_padding").x * 2.0f);
    const float x = box.x + header_width + header_padding + data->timeline_data->scroll.x
        + uph_state.interact.song_timeline_playhead_position * data->timeline_data->zoom.x;
    naui_push_clip_rect(box.x + header_width + header_padding, box.y, box.width, box.height);
    naui_draw_line(
        naui_vec2(x, box.y),
        naui_vec2(x, box.height + box.y),
        leaf_hex(0xEFFBB5),
        NAUI_DPI(1.0f)
    );
    naui_pop_clip_rect();
}

static void song_timeline_ruler_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    Uph_RulerCustomDrawData *data = (Uph_RulerCustomDrawData*)user_data;
    Naui_Vec2 zoom = data->timeline_data->zoom;
    float scroll_x = data->timeline_data->scroll.x;

if (naui_mouse_down(NAUI_MOUSE_LEFT) && vec4_contains_point(naui_vec4(box.x, box.y, box.width, box.height), naui_vec2(naui_mouse_x(), naui_mouse_y())))
{
    float local_mouse_x = naui_mouse_x() - box.x;
    float beat = (local_mouse_x - scroll_x) / zoom.x;
    beat = roundf(beat);
    beat = fmaxf(beat, 0.0f);

    if ((int)beat != (int)uph_state.interact.song_timeline_playhead_position)
        uph_state.interact.song_timeline_playhead_position = beat;
}

    const int beats_per_bar = 4;
    const float pixels_per_beat = zoom.x;
    const float pixels_per_bar = pixels_per_beat * beats_per_bar;

    const Leaf_Color beat_tick_color = naui_theme_color("uph_track_grid_beat_color");
    const Leaf_Color bar_tick_color  = naui_theme_color("uph_track_grid_bar_color");
    const Leaf_Color label_color     = naui_theme_color("uph_track_grid_text_color");

    const float track_area_left  = box.x;
    const float track_area_width = box.width;
    if (track_area_width <= 0.0f)
        return;

    naui_push_clip_rect(track_area_left, box.y, track_area_width, box.height);

    const float offset_beats = -scroll_x / pixels_per_beat;
    const int start_beat_index = (int)floorf(offset_beats);
    const int beat_count = (int)(track_area_width / pixels_per_beat) + 2;

    const float bar_tick_height  = box.height * 0.6f;
    const float beat_tick_height = box.height * 0.3f;
    const float font_size = NAUI_DPI(12.0f);

    const float min_label_spacing_px = NAUI_DPI(36.0f);
    const float min_beat_tick_spacing_px = NAUI_DPI(6.0f);

    const bool draw_beat_ticks = pixels_per_beat >= min_beat_tick_spacing_px;

    float bars_needed = min_label_spacing_px / fmaxf(pixels_per_bar, 0.0001f);

    int label_stride = 1;
    while ((float)label_stride < bars_needed)
        label_stride *= 2;

    for (int i = start_beat_index; i < start_beat_index + beat_count; i++)
    {
        bool is_bar_line = ((i % beats_per_bar) + beats_per_bar) % beats_per_bar == 0;

        if (!is_bar_line && !draw_beat_ticks)
            continue;

        float x = track_area_left + scroll_x + i * pixels_per_beat;
        if (x < track_area_left - pixels_per_beat) continue;
        if (x > track_area_left + track_area_width) break;

        float tick_height = is_bar_line ? bar_tick_height : beat_tick_height;

        naui_draw_line(
            naui_vec2(x, box.y + box.height - tick_height),
            naui_vec2(x, box.y + box.height),
            is_bar_line ? bar_tick_color : beat_tick_color,
            NAUI_DPI(1.0f)
        );

        if (is_bar_line)
        {
            int bar_number = i / beats_per_bar + 1;
            int bar_index_zero_based = bar_number - 1;
            if (bar_index_zero_based % label_stride == 0)
            {
                char label[16];
                snprintf(label, sizeof(label), "%d", bar_number);
                naui_draw_text(naui_vec2(x + NAUI_DPI(3.0f), box.y + NAUI_DPI(2.0f)), label, font_size, 0, label_color);
            }
        }
    }

    naui_pop_clip_rect();
}

static bool uph_track_toggle_button(const char *text, bool *toggled)
{
    Leaf_ID id = naui_widgets_alloc_id();
    bool hovered = naui_panel_hovered(naui_current_panel()) && leaf_hovered(id);

    bool clicked = hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT);
    if (clicked)
        *toggled = !(*toggled);

    Naui_Color color;
    if (hovered)
        color = naui_mouse_down(NAUI_MOUSE_LEFT) ?
            naui_theme_color("naui_widget_frame_pressed_bg_color") :
            naui_theme_color("naui_widget_frame_hovered_bg_color");
    else if (*toggled)
        color = naui_theme_color("naui_widget_frame_toggled_bg_color");
    else color = naui_theme_color("naui_widget_frame_bg_color");

    const float font_size = NAUI_DPI(naui_theme_float("uph_track_font_size"));

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(font_size)), LEAF_SIZE_FIXED(NAUI_DPI(font_size))},
        .padding = LEAF_PADDING_ALL(NAUI_DPI(1.0f)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = color,
        .border = {
            .width = 1.0f,
            .sides = LEAF_SIDE_ALL,
            .color = naui_theme_color("naui_widget_frame_border")
        },
        .rounding = {
            .value = NAUI_DPI(naui_theme_float("naui_widget_frame_rounding")),
            .corners = LEAF_CORNER_ALL
        }
    })
    {
        leaf_text(text, {
            .font_size = font_size,
            .color = naui_theme_color("uph_track_text_color")
        });
    }

    return clicked;
}

static void song_timeline_on_update(Uph_SongTimelineData *timeline_data)
{
    const float header_width = naui_theme_float("uph_track_header_width");
    const Naui_Vec2 header_padding = naui_theme_vec2("uph_track_header_padding");

    if (naui_panel_hovered(naui_current_panel()))
    {
        const float track_area_x = NAUI_DPI(header_width) + NAUI_DPI(header_padding.x * 2.0f);

        Leaf_BoundingBox viewport = leaf_get_bounding_box(leaf_id("uph_song_timeline_viewport"));

        Naui_Vec2 mouse_pos = naui_vec2((float)naui_mouse_x(), (float)naui_mouse_y());
        bool ctrl_down = naui_key_down(NAUI_KEY_CONTROL);

        if (ctrl_down)
            song_timeline_update_zoom_x(timeline_data, mouse_pos, viewport, track_area_x);
        else
            song_timeline_update_scroll_y_wheel(timeline_data);

        song_timeline_update_middle_mouse_drag(timeline_data, mouse_pos, viewport, ctrl_down);

        timeline_data->last_mouse_pos = mouse_pos;

        const float ruler_height = NAUI_DPI(24.0f);
        float track_list_height = viewport.height - ruler_height;
        song_timeline_clamp_scroll(timeline_data, track_list_height);
    }

    if (timeline_data->block_interaction_mode != UPH_BLOCK_INTERACTION_NONE)
    {
        const float track_area_x = NAUI_DPI(header_width) + NAUI_DPI(header_padding.x * 2.0f);
        Leaf_BoundingBox viewport = leaf_get_bounding_box(leaf_id("uph_song_timeline_viewport"));
        song_timeline_update_block_interaction(timeline_data, viewport, track_area_x);
    }

    const Leaf_Color border_color = naui_theme_color("uph_track_border_color");
    const Leaf_Color track_header_color = naui_theme_color("uph_track_header_color");
    const Leaf_Color track_header_border_color = naui_theme_color("uph_track_header_border_color");

    const Leaf_Color track_bg1_color = naui_theme_color("uph_track_bg1_color");
    const Leaf_Color track_bg2_color = naui_theme_color("uph_track_bg2_color");

    const Leaf_Color track_text_color = naui_theme_color("uph_track_text_color");
    const float font_size = naui_theme_float("uph_track_font_size");

    leaf({
        .id = leaf_id("uph_song_timeline_viewport"),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
    })
    {
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(24.0f))},
        })
        {
            leaf({
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width)), LEAF_SIZE_FULL},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .padding = LEAF_PADDING_AXES(NAUI_DPI(header_padding.x), NAUI_DPI(header_padding.y)),
                .child_gap = NAUI_DPI(12.0f),
                .border = {
                    .width = 1.0f,
                    .sides = LEAF_SIDE_RIGHT,
                    .color = track_header_color
                }
            })
            {
                
            }

            Uph_RulerCustomDrawData ruler_data = {
                .timeline_data = timeline_data
            };
            leaf({
                .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
                .custom_draw = song_timeline_ruler_custom_draw,
                .custom_draw_data = LEAF_DATA_SLICE(ruler_data)
            });
        }

        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .child_offset = {0.0f, -timeline_data->scroll.y},
            .clip_children = true
        })
        for (uint32_t i = 0; i < naui_list_len(uph_state.project.tracks); i++)
        {
            Uph_Track *track = &uph_state.project.tracks[i];

            leaf({
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(timeline_data->zoom.y)}
            })
            {
                leaf({
                    .direction = LEAF_DIRECTION_HORIZONTAL,
                    .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
                })
                {
                    leaf({
                        .direction = LEAF_DIRECTION_HORIZONTAL,
                        .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width)), LEAF_SIZE_FULL},
                        .padding = LEAF_PADDING_AXES(NAUI_DPI(header_padding.x), NAUI_DPI(header_padding.y)),
                        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                        .child_gap = NAUI_DPI(12.0f),
                        .color = track_header_color,
                        .border = {
                            .width = 1.0f,
                            .sides = LEAF_SIDE_ALL,
                            .color = track_header_border_color
                        },
                        .clip_children = true
                    })
                    {
                        leaf({
                            .size = {LEAF_SIZE_FIXED(NAUI_DPI(4.0f)), LEAF_SIZE_PERCENT(0.85f)},
                            .color = track->color,
                            .rounding = {
                                .value = NAUI_DPI(4.0f),
                                .corners = LEAF_CORNER_ALL
                            }
                        });
                        leaf({
                            .size = {LEAF_SIZE_GROW, LEAF_SIZE_PERCENT(0.85f)},
                            .child_gap = NAUI_DPI(10.0f)
                        })
                        {
                            leaf({
                                .direction = LEAF_DIRECTION_HORIZONTAL,
                                .child_gap = NAUI_DPI(6.0f)
                            })
                            {
                                Naui_Image *icon;
                                switch (track->type)
                                {
                                    case UPH_TRACK_AUDIO: icon = naui_get_image("uph_icon_wave"); break;
                                    case UPH_TRACK_MIDI: icon = naui_get_image("uph_icon_wave"); break;
                                    default: icon = NULL; break;
                                }
                                if (icon)
                                leaf({
                                    .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_FIXED(NAUI_DPI(font_size * 0.9f))},
                                    .image = icon,
                                    .color = track_text_color,
                                    .aspect_ratio = NAUI_IMAGE_ASPECT_RATIO(icon)
                                });
                                leaf_text(track->name.data, {
                                    .font_size = NAUI_DPI(font_size),
                                    .color = track_text_color
                                });
                            }
                            leaf({
                                .direction = LEAF_DIRECTION_HORIZONTAL,
                                .child_gap = NAUI_DPI(2.0f)
                            })
                            {
                                //uph_track_toggle_button("M", &track->muted);
                                //uph_track_toggle_button("S", &track->soloed);
                                //uph_track_toggle_button("R", &track->recording);
                            }
                        }
                    }
                    leaf({
                        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                        .floating = {
                            .parent_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_TOP},
                            .self_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_TOP}
                        },
                        .size = {LEAF_SIZE_FIXED(NAUI_DPI(6.0f)), LEAF_SIZE_FULL},
                        .color = uph_apply_opacity_to_color(track->color, track->glow_effect)
                    });
                }

                Uph_GridCustomDrawData grid_draw_data = {
                    .timeline_data = timeline_data
                };
                leaf({
                    .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
                    .color = (i & 1) ? track_bg2_color : track_bg1_color,
                    .custom_draw = song_timeline_grid_custom_draw,
                    .custom_draw_data = LEAF_DATA_SLICE(grid_draw_data),
                    .border = {
                        .width = 1.0f,
                        .sides = LEAF_SIDE_ALL,
                        .color = border_color
                    }
                })
                {
                    Uph_TrackCustomDrawData draw_data = {
                        .timeline_data = timeline_data,
                        .panel_id = naui_current_panel(),
                        .track_index = i
                    };
                    leaf({
                        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
                        .custom_draw = song_timeline_track_custom_draw,
                        .custom_draw_data = LEAF_DATA_SLICE(draw_data)
                    });
                }
            }
            track->glow_effect = naui_lerp(track->glow_effect, 0.0f, naui_delta_time() * 15.0f);
        }

        Uph_OverlayCustomDrawData overlay_data = {
            .timeline_data = timeline_data
        };
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .custom_draw = song_timeline_overlay_custom_draw,
            .custom_draw_data = LEAF_DATA_SLICE(overlay_data)
        });
    }
}