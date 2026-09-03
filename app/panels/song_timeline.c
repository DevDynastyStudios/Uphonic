#define UPH_SONG_TIMELINE_ZOOM_X_MIN 8.0f
#define UPH_SONG_TIMELINE_ZOOM_X_MAX 256.0f
#define UPH_SONG_TIMELINE_PAN_SPEED 1.0f
#define UPH_SONG_TIMELINE_SCROLL_Y_SPEED 40.0f
#define UPH_SONG_TIMELINE_ZOOM_SPEED 0.1f
#define UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH 6.0f

typedef uint8_t Uph_BlockInteractionMode;
enum
{
    UPH_BLOCK_INTERACTION_NONE,
    UPH_BLOCK_INTERACTION_MOVE,
    UPH_BLOCK_INTERACTION_RESIZE_LEFT,
    UPH_BLOCK_INTERACTION_RESIZE_RIGHT
};

typedef struct
{
    double initial_drag_beat_offset;
    double initial_start_beat;
    double initial_length_beats;
    double initial_start_offset_beats;
    uint32_t block_index;
    uint32_t track_index;
    bool active;
    Uph_BlockInteractionMode mode;
}
Uph_DraggingBlockState;

typedef struct
{
    uint32_t block_index;
    uint32_t track_index;
    bool active;
}
Uph_HoveredBlockState;

typedef struct
{
    Naui_Vec2 scroll;
    Naui_Vec2 zoom;
    Uph_DraggingBlockState drag;
    Uph_HoveredBlockState hovered_block;
    Leaf_BoundingBox panel_bounding_box;
    uint32_t current_options_track_index;
	Uph_ActionMode current_action_mode;
    bool panel_hovered;
    bool tracks_hovered;
    bool disable_space_to_play;
}
Uph_SongTimelineData;

static Uph_SongTimelineData uph_song_timeline_data;

NAUI_PANEL(uph_song_timeline)

static Uph_TimelineBlock uph_song_timeline_init_block(double start_beat, uint32_t resource_index, Uph_ResourceType block_type)
{
	Uph_TimelineBlock block = {
		.start_beat = start_beat,
        .start_offset_beats = uph_state.shared.song_timeline_current_block_start_offset,
        .length_beats = uph_state.shared.song_timeline_current_block_length,
		.resource_index = resource_index,
		.type = block_type
	};
    return block;
}

static void uph_song_timeline_on_attach(void)
{
    Naui_PanelID panel = naui_current_panel();

    naui_panel_set_title(panel, NAUI_TR("song_timeline.title"));

    uph_song_timeline_data.scroll = (Naui_Vec2) { 0.0f, 0.0f };
    uph_song_timeline_data.zoom = (Naui_Vec2) { 32.0f, 90.0f };
}

static void uph_song_timeline_on_detach(void)
{

}

static void uph_song_timeline_on_open(void)
{
    
}

static void uph_song_timeline_on_close(void)
{
    
}

static void uph_song_timeline_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x)
{
    const Leaf_Color beat_color = naui_theme_color("uph_track_grid_beat_color");
    const Leaf_Color bar_color = naui_theme_color("uph_track_grid_bar_color");

    const int32_t first_line = (int32_t)(scroll_x / zoom_x);
    const uint32_t line_count = (uint32_t)(bbox.width / zoom_x) + 2;

    for (uint32_t i = 0; i < line_count; i++)
    {
        const int32_t line_index = first_line + (int32_t)i;
        if (line_index < 0)
            continue;

        const float x = bbox.x + (float)line_index * zoom_x - scroll_x;

        if (x < bbox.x || x > bbox.x + bbox.width)
            continue;

        const bool is_downbeat = (line_index % 4) == 0;
        const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;

        naui_draw_line(
            (Naui_Vec2) { x, bbox.y },
            (Naui_Vec2) { x, bbox.y + bbox.height },
            line_color,
            1.0f
        );
    }
}

static inline bool upb_song_timeline_vec4_contains_vec2(const Naui_Vec4 rect, const Naui_Vec2 point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z &&
           point.y >= rect.y && point.y <= rect.y + rect.w;
}

static void uph_song_timeline_update_playhead_drag(Leaf_BoundingBox bbox)
{
    Uph_SongTimelineData *data = &uph_song_timeline_data;

    static bool dragging_playhead;

    const bool mouse_over_ruler = upb_song_timeline_vec4_contains_vec2(
        (Naui_Vec4) { bbox.x, bbox.y, bbox.width, bbox.height },
        (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }
    );

    if (!dragging_playhead && mouse_over_ruler && naui_mouse_pressed(NAUI_MOUSE_LEFT))
        dragging_playhead = true;

    if (!dragging_playhead)
        return;

    const double mouse_beat_raw =
        ((double)naui_mouse_x() - bbox.x + data->scroll.x) / data->zoom.x;
    const double mouse_beat = fmax(0.0, mouse_beat_raw);

    if (uph_state.shared.song_timeline_playing)
    {
        const double current_beat = (double)uph_state.shared.song_timeline_playhead_position;
        const double distance = mouse_beat - current_beat;

        if (fabs(distance) > 1.0)
            uph_state.shared.song_timeline_playhead_position = (float)round(mouse_beat);
    }
    else
    {
        uph_state.shared.song_timeline_playhead_position = (float)round(mouse_beat);
    }

    naui_set_cursor(NAUI_CURSOR_HAND);

    if (naui_mouse_released(NAUI_MOUSE_LEFT))
        dragging_playhead = false;
}

static void uph_song_timeline_render_top_ruler(Leaf_BoundingBox bbox)
{
    const Leaf_Color beat_color = naui_theme_color("uph_track_grid_beat_color");
    const Leaf_Color bar_color = naui_theme_color("uph_track_grid_bar_color");
    const Leaf_Color number_color = naui_theme_color("uph_track_grid_text_color");

    const int32_t first_line = (int32_t)(uph_song_timeline_data.scroll.x / uph_song_timeline_data.zoom.x);
    const uint32_t line_count = (uint32_t)(bbox.width / uph_song_timeline_data.zoom.x) + 2;

    uph_song_timeline_update_playhead_drag(bbox);

    naui_push_clip_rect(bbox.x - 0.5f, bbox.y, bbox.width, bbox.height);
    for (uint32_t i = 0; i < line_count; i++)
    {
        const int32_t line_index = first_line + (int32_t)i;
        if (line_index < 0)
            continue;

        const float x = bbox.x + (float)line_index * uph_song_timeline_data.zoom.x - uph_song_timeline_data.scroll.x;

        if (x < bbox.x - uph_song_timeline_data.zoom.x || x > bbox.x + bbox.width)
            continue;

        const bool is_downbeat = (line_index % 4) == 0;
        const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;

        naui_draw_line(
            (Naui_Vec2) { x, bbox.y + bbox.height * (is_downbeat ? 0.4f : 0.6f)},
            (Naui_Vec2) { x, bbox.y + bbox.height },
            line_color,
            1.0f
        );

        if (is_downbeat)
        {
            char label[16];
            snprintf(label, sizeof(label), "%d", line_index / 4);
            naui_draw_text((Naui_Vec2) { x + 4.0f, bbox.y + bbox.height * 0.4f }, label, NAUI_DPI(13.0f), 0, number_color);
        }
    }
    naui_pop_clip_rect();
}

static void uph_song_timeline_render_midi_pattern(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Naui_Color color,
    double start_offset,
    Leaf_BoundingBox visible_bbox,
    Uph_MidiPattern* pattern
)
{
    const uint32_t note_count = (uint32_t)naui_list_len(pattern->notes);

    if (note_count == 0)
        return;

    uint8_t lowest_key = UINT8_MAX;
    uint8_t highest_key = 0;

    for (uint32_t i = 0; i < note_count; i++)
    {
        const Uph_MidiNote *note = &pattern->notes[i];
        if (note->key_number < lowest_key)
            lowest_key = note->key_number;
        if (note->key_number > highest_key)
            highest_key = note->key_number;
    }

    const uint32_t key_range = (uint32_t)(highest_key - lowest_key) + 1;

    const float slot_height = size.y / (float)key_range;
    const float note_height = fmaxf(slot_height, 1.0f);

    for (uint32_t i = 0; i < note_count; i++)
    {
        const Uph_MidiNote *note = &pattern->notes[i];

        const double note_start_beat = note->start_beat - start_offset;

        if (note_start_beat + note->length_beats < 0.0)
            continue;

        const float x = position.x + (float)(note_start_beat * uph_song_timeline_data.zoom.x);
        const float width = (float)(note->length_beats * uph_song_timeline_data.zoom.x);

        if (x + width < visible_bbox.x || x > visible_bbox.x + visible_bbox.width)
            continue;

        const uint32_t key_offset_from_top = (uint32_t)(highest_key - note->key_number);
        const float y = position.y + (float)key_offset_from_top * slot_height;

        naui_fill_rect(
            (Naui_Vec2) { x, y },
            (Naui_Vec2) { fmaxf(width, 1.0f), note_height },
            color,
            0,
            NAUI_CORNER_NONE
        );
    }
}


float easeOutQuint(float x) {
    return 1.0f - powf(1.0f - x, 5.0f);
}

float easeOutElastic(float x) {
    const float c4 = (2.0f * (float)M_PI) / 3.0f;

    return x == 0.0f
        ? 0.0f
        : x == 1.0f
        ? 1.0f
        : powf(2.0f, -10.0f * x) *
              sinf((x * 10.0f - 0.75f) * c4) +
          1.0f;
}


static void uph_song_timeline_render_timeline_block(Naui_Vec2 position, Naui_Vec2 size, Naui_Color color, float opacity, const Uph_TimelineBlock *block, Leaf_BoundingBox visible_bbox)
{
    const float anim_scale = NAUI_DPI(30.0f);
    position.x += size.x * 0.5f;
    position.y += size.y * 0.5f;
    size.x = size.x + (easeOutElastic(block->visual_lifetime) - 1.0f) * anim_scale;
    size.y = size.y + (easeOutElastic(block->visual_lifetime) - 1.0f) * anim_scale;
    position.x -= size.x * 0.5f;
    position.y -= size.y * 0.5f;

    const float title_padding = NAUI_DPI(2.0f);
    const float font_size = NAUI_DPI(13.0f);
    const float title_height = font_size + title_padding * 2.0f;
    const float rounding = NAUI_DPI(6.0f);

    naui_push_clip_rect(position.x, position.y, size.x, size.y);

    naui_fill_rect(
        position,
        (Naui_Vec2) { size.x, title_height },
        leaf_rgba(color.r, color.g, color.b, (uint8_t)(200 * opacity)),
        rounding,
        NAUI_CORNER_TL | NAUI_CORNER_TR
    );

    naui_fill_rect(
        (Naui_Vec2) { position.x, position.y + title_height },
        (Naui_Vec2) { size.x, size.y - title_height },
        leaf_rgba(color.r, color.g, color.b, (uint8_t)(60 * opacity)),
        rounding,
        NAUI_CORNER_BL | NAUI_CORNER_BR
    );

    if (block->type == UPH_RESOURCE_SAMPLE)
    {
        naui_draw_text(
            (Naui_Vec2) { position.x + title_padding, position.y + title_padding },
            uph_state.project.samples[block->resource_index].name.data,
            font_size,
            0,
            leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity))
        );

        uph_ui_waveform_zoomable(
            (Naui_Vec2) { position.x, position.y + title_height },
            (Naui_Vec2) { size.x, size.y - title_height },
            leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity)),
            uph_song_timeline_data.zoom.x,
            block->start_offset_beats,
            visible_bbox,
            &uph_state.project.samples[block->resource_index]
        );
    }
    else if (block->type == UPH_RESOURCE_PATTERN)
    {
        naui_draw_text(
            (Naui_Vec2) { position.x + title_padding, position.y + title_padding },
            uph_state.project.midi_patterns[block->resource_index].name.data,
            font_size,
            0,
            leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity))
        );

        uph_song_timeline_render_midi_pattern(
            (Naui_Vec2) { position.x, position.y + title_height },
            (Naui_Vec2) { size.x, size.y - title_height },
            leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity)),
            block->start_offset_beats,
            visible_bbox,
            &uph_state.project.midi_patterns[block->resource_index]
        );
    }

    naui_pop_clip_rect();
}

static void uph_song_timeline_update_drag_track_switch(void)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    if (!drag->active)
        return;

    if (drag->mode != UPH_BLOCK_INTERACTION_MOVE)
        return;

    float mouse_y = (float)naui_mouse_y() + uph_song_timeline_data.scroll.y - uph_song_timeline_data.panel_bounding_box.y;
    int32_t hovered_track = (int32_t)(mouse_y / uph_song_timeline_data.zoom.y);

    uint32_t track_count = (uint32_t)naui_list_len(uph_state.project.tracks);
    if (hovered_track < 0 || (uint32_t)hovered_track >= track_count)
        return;

    if ((uint32_t)hovered_track == drag->track_index)
        return;

    Uph_Track *old_track = &uph_state.project.tracks[drag->track_index];
    Uph_Track *new_track = &uph_state.project.tracks[(uint32_t)hovered_track];

    if (new_track->type != UPH_RESOURCE_NONE && new_track->type != old_track->type)
        return;

    Uph_TimelineBlock moved = old_track->blocks[drag->block_index];
    naui_list_uremove(old_track->blocks, drag->block_index);
    naui_list_push(new_track->blocks, moved);

    if (new_track->type == UPH_RESOURCE_NONE)
        new_track->type = moved.type;
    if (naui_list_len(old_track->blocks) == 0)
        old_track->type = UPH_RESOURCE_NONE;

    drag->track_index = (uint32_t)hovered_track;
    drag->block_index = (uint32_t)naui_list_len(new_track->blocks) - 1;
}

static Uph_BlockInteractionMode uph_song_timeline_classify_hover(Naui_Vec4 hover_box, float mouse_x)
{
    if (mouse_x <= hover_box.x + UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_LEFT;
    if (mouse_x >= hover_box.x + hover_box.z - UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_RIGHT;
    return UPH_BLOCK_INTERACTION_MOVE;
}

static inline bool uph_song_timeline_block_is_visible(double start_beat, double length_beats, float zoom_x, float scroll_x, float viewport_width)
{
    const double left = start_beat * zoom_x;
    const double right = (start_beat + length_beats) * zoom_x;

    if (right < scroll_x)
        return false;
    if (left > scroll_x + viewport_width)
        return false;

    return true;
}

static void uph_song_timeline_update_track_timeline_drag(Leaf_BoundingBox bbox, uint32_t track_index)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    Naui_List(Uph_TimelineBlock) blocks = uph_state.project.tracks[track_index].blocks;
    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
    {
        if (!uph_song_timeline_block_is_visible(blocks[i].start_beat, blocks[i].length_beats, zoom_x, scroll_x, bbox.width))
            continue;

        bool is_dragging_this_block =
            drag->active &&
            drag->track_index == track_index &&
            drag->block_index == i;

        if (is_dragging_this_block)
        {
            double mouse_beat = ((double)naui_mouse_x() - bbox.x + scroll_x) / zoom_x;

            if (drag->mode == UPH_BLOCK_INTERACTION_MOVE)
            {
                blocks[i].start_beat =
                    fmax(0.0, floor(mouse_beat + drag->initial_drag_beat_offset));

                naui_set_cursor(NAUI_CURSOR_HAND);
            }
            else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
            {
                double new_start = fmax(0.0, floor(mouse_beat));
                double end_beat = drag->initial_start_beat + drag->initial_length_beats;

                const double earliest_start_beat = drag->initial_start_beat - drag->initial_start_offset_beats;
                new_start = fmax(new_start, earliest_start_beat);

                new_start = fmin(new_start, end_beat - 1.0);

                const double delta_beats = new_start - drag->initial_start_beat;

                blocks[i].start_beat = new_start;
                blocks[i].length_beats = end_beat - new_start;
                blocks[i].start_offset_beats = drag->initial_start_offset_beats + delta_beats;

                uph_state.shared.song_timeline_current_block_length = blocks[i].length_beats;
                uph_state.shared.song_timeline_current_block_start_offset = blocks[i].start_offset_beats;

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }
            else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
            {
                double new_length = ceil(mouse_beat) - blocks[i].start_beat;
                blocks[i].length_beats = fmax(1.0, new_length);

                uph_state.shared.song_timeline_current_block_length = blocks[i].length_beats;
                uph_state.shared.song_timeline_current_block_start_offset = blocks[i].start_offset_beats;

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }

            if (naui_mouse_released(NAUI_MOUSE_LEFT))
            {
                drag->active = false;
                drag->mode = UPH_BLOCK_INTERACTION_NONE;
            }

            return;
        }

        if (!uph_song_timeline_data.tracks_hovered)
            return;

        if (!drag->active)
        {
            const float block_left = bbox.x + zoom_x * blocks[i].start_beat - scroll_x;
            const float block_right = block_left + zoom_x * blocks[i].length_beats;

            const float clamped_left = fmaxf(bbox.x, block_left);
            const float clamped_right = fminf(bbox.x + bbox.width, block_right);

            if (clamped_right <= clamped_left)
                continue;

            Naui_Vec4 hover_box = (Naui_Vec4){
                clamped_left,
                bbox.y,
                clamped_right - clamped_left,
                bbox.height
            };

            if (upb_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
            {
                Uph_BlockInteractionMode hover_mode =
                    uph_song_timeline_classify_hover(hover_box, (float)naui_mouse_x());

                if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
                {
                    drag->active = true;
                    drag->track_index = track_index;
                    drag->block_index = i;
                    drag->mode = hover_mode;
                    drag->initial_start_beat = blocks[i].start_beat;
                    drag->initial_length_beats = blocks[i].length_beats;
                    drag->initial_start_offset_beats = blocks[i].start_offset_beats;

                    double mouse_beat = ((double)naui_mouse_x() - bbox.x + scroll_x) / zoom_x;
                    drag->initial_drag_beat_offset = blocks[i].start_beat - mouse_beat;

                    uph_state.shared.selected_resource.type = blocks[i].type;
                    uph_state.shared.selected_resource.index = blocks[i].resource_index;

                    uph_state.shared.song_timeline_current_block_length = blocks[i].length_beats;
                    uph_state.shared.song_timeline_current_block_start_offset = blocks[i].start_offset_beats;
                }

                naui_set_cursor(
                    hover_mode == UPH_BLOCK_INTERACTION_MOVE
                        ? NAUI_CURSOR_HAND
                        : NAUI_CURSOR_RESIZE_EW
                );

                uph_song_timeline_data.hovered_block.block_index = i;
                uph_song_timeline_data.hovered_block.track_index = track_index;
                uph_song_timeline_data.hovered_block.active = true;
            }
        }
    }
}

static void uph_song_timeline_render_track_timeline_blocks(Leaf_BoundingBox bbox, uint32_t track_index)
{
    const Uph_Track *track = &uph_state.project.tracks[track_index];
    Naui_List(Uph_TimelineBlock) blocks = track->blocks;

    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;

    const float opacity = ((track->state & UPH_TRACK_MUTED) || (track->state & UPH_TRACK_SILENCED)) ? 0.25f : 1.0f;

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
    {
        if (!uph_song_timeline_block_is_visible(blocks[i].start_beat, blocks[i].length_beats, zoom_x, scroll_x, bbox.width))
            continue;

        blocks[i].visual_lifetime = NAUI_MIN(blocks[i].visual_lifetime + naui_delta_time(), 1.0f);
        uph_song_timeline_render_timeline_block(
            (Naui_Vec2) { bbox.x + zoom_x * blocks[i].start_beat - scroll_x, bbox.y },
            (Naui_Vec2) { zoom_x * blocks[i].length_beats, bbox.height },
            track->color,
            opacity,
            &blocks[i],
            bbox
        );
    }
}

static void uph_song_timeline_update_track_action_input(Leaf_BoundingBox bbox, uint32_t track_index)
{
    if (!uph_song_timeline_data.tracks_hovered)
        return;
    
    Uph_Track *track = &uph_state.project.tracks[track_index];

    if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track_index == track_index)
    {
        naui_list_uremove(track->blocks, uph_song_timeline_data.hovered_block.block_index);
        if (naui_list_len(track->blocks) == 0)
            track->type = UPH_RESOURCE_NONE;
    }

    if (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT)
    {

    }
    else if (uph_song_timeline_data.current_action_mode == UPH_ACTION_DRAW)
    {
        if (uph_state.shared.selected_resource.type == UPH_RESOURCE_NONE)
            return;
        else if (uph_state.shared.selected_resource.type == UPH_RESOURCE_SAMPLE && !uph_state.project.samples)
            return;
        else if (uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN && !uph_state.project.midi_patterns)
            return;

        Uph_Track *track = &uph_state.project.tracks[track_index];
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && !uph_song_timeline_data.hovered_block.active &&
            (track->type == UPH_RESOURCE_NONE || track->type == uph_state.shared.selected_resource.type))
        {
            if (upb_song_timeline_vec4_contains_vec2(
                (Naui_Vec4) {bbox.x, bbox.y, bbox.width, bbox.height},
                (Naui_Vec2) {naui_mouse_x(), naui_mouse_y()}
            ))
            {
                const float beat = (naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
                uph_song_timeline_data.drag.active = true;
                uph_song_timeline_data.drag.block_index = naui_list_len(track->blocks);
                uph_song_timeline_data.drag.track_index = track_index;
                uph_song_timeline_data.drag.mode = UPH_BLOCK_INTERACTION_MOVE;
                uph_song_timeline_data.drag.initial_drag_beat_offset = 0.0;
                naui_list_push(track->blocks, uph_song_timeline_init_block(floor(beat), uph_state.shared.selected_resource.index, uph_state.shared.selected_resource.type));
                if (track->type == UPH_RESOURCE_NONE)
                    track->type = uph_state.shared.selected_resource.type;
            }
        }
    }
    else if (uph_song_timeline_data.current_action_mode == UPH_ACTION_CUT)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) &&
            uph_song_timeline_data.hovered_block.active &&
            uph_song_timeline_data.hovered_block.track_index == track_index)
        {
            const uint32_t block_index = uph_song_timeline_data.hovered_block.block_index;
            Uph_TimelineBlock *block = &track->blocks[block_index];

            const double mouse_beat = ((double)naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
            const double cut_beat = floor(mouse_beat);

            const double left_length = cut_beat - block->start_beat;
            const double right_length = block->length_beats - left_length;

            if (left_length >= 1.0 && right_length >= 1.0)
            {
                Uph_TimelineBlock right_half = *block;
                right_half.start_beat = cut_beat;
                right_half.length_beats = right_length;
                right_half.start_offset_beats = block->start_offset_beats + left_length;

                block->length_beats = left_length;

                naui_list_push(track->blocks, right_half);
            }
        }
    }
}

static void uph_song_timeline_render_track_timeline_overlay(Leaf_BoundingBox bbox, uint32_t *track_index_ptr)
{
    const uint32_t track_index = *track_index_ptr;

    naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);

    uph_song_timeline_render_ruler(bbox, uph_song_timeline_data.zoom.x, uph_song_timeline_data.scroll.x);

    uph_song_timeline_update_track_timeline_drag(bbox, track_index);
    uph_song_timeline_update_track_action_input(bbox, track_index);
    uph_song_timeline_render_track_timeline_blocks(bbox, track_index);

    naui_pop_clip_rect();
}

static void uph_song_timeline_render_track_timeline(uint32_t track_index)
{
    const Leaf_Color bg_color = track_index & 1 ? naui_theme_color("uph_track_bg1_color") : naui_theme_color("uph_track_bg2_color");
    const Leaf_Color border_color = naui_theme_color("uph_track_border_color");

    const float row_height = NAUI_DPI(uph_song_timeline_data.zoom.y);
    const float row_y = uph_song_timeline_data.panel_bounding_box.y
        - uph_song_timeline_data.scroll.y
        + (float)track_index * row_height;

    const bool row_visible =
        row_y + row_height >= uph_song_timeline_data.panel_bounding_box.y &&
        row_y <= uph_song_timeline_data.panel_bounding_box.y + uph_song_timeline_data.panel_bounding_box.height;

    leaf({
        .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
        .color = bg_color,
        .border = {
            .width = 1.0f,
            .color = border_color,
            .sides = LEAF_SIDE_TOP | LEAF_SIDE_BOTTOM
        },
        .custom_draw_data = row_visible ? LEAF_DATA_SLICE(track_index) : (Leaf_DataSlice){ 0 },
        .custom_draw = row_visible ? (Leaf_CustomDrawFn)uph_song_timeline_render_track_timeline_overlay : NULL
    });
}

static void uph_song_timeline_solo_track(uint32_t track_index)
{
    Uph_Track *track = &uph_state.project.tracks[track_index];
    track->state ^= UPH_TRACK_SOLOED;

    bool is_soloed = track->state & UPH_TRACK_SOLOED;

    if (is_soloed)
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
        {
            Uph_Track *t = &uph_state.project.tracks[i];
            if (i == track_index)
            {
                t->state &= ~UPH_TRACK_SILENCED;
                continue;
            }
            t->state &= ~UPH_TRACK_SOLOED;
            t->state |= UPH_TRACK_SILENCED;
        }
    }
    else
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
        {
            uph_state.project.tracks[i].state &= ~UPH_TRACK_SILENCED;
        }
    }
}

static void uph_song_timeline_render_track_header(uint32_t track_index, Uph_UIMenuID options_menu)
{
    Uph_Track *track = &uph_state.project.tracks[track_index];

    const Leaf_Color bg_color = naui_theme_color("uph_track_header_color");
    const Leaf_Color text_color = naui_theme_color("uph_track_text_color");
    const Leaf_Color border_color = naui_theme_color("uph_track_header_border_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_track_header_padding");
    const float header_width = naui_theme_float("uph_track_header_width");
    const float font_size = naui_theme_float("uph_track_font_size");

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width)), LEAF_SIZE_FULL},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = bg_color,
        .child_gap = NAUI_DPI(10.0f),
        .border = {
            .width = 1.0f,
            .color = border_color,
            .sides = LEAF_SIDE_ALL
        }
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(5.0f)), LEAF_SIZE_FULL},
            .color = track->color,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });

        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_gap = NAUI_DPI(8.0f)
        })
        {
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(3.0f)
            })
            {
                if (track->type != UPH_RESOURCE_NONE)
                {
                    Naui_Image *icon;
                    switch (track->type)
                    {
                    case UPH_RESOURCE_SAMPLE:
                        icon = naui_asset_image("uph_icon_wave");
                        break;
                    case UPH_RESOURCE_PATTERN:
                        icon = naui_asset_image("uph_icon_piano");
                        break;
                    }
                    const float icon_size = NAUI_DPI(naui_theme_float("uph_track_icon_size"));
                    leaf({
                        .size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
                        .image = icon,
                        .color = text_color
                    });
                }

                static int32_t current_rename_index = -1;
                Leaf_ID name_id = leaf_id_indexed("uph_track_name", track_index);

                if (current_rename_index == track_index)
                {
                    if (uph_ui_textfield(&track->name, name_id, UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, NAUI_TR("song_timeline.track.title")))
                    {
                        if (!track->name.length)
                            track->name = naui_string_from_cstr(NAUI_TR("song_timeline.track.title"));
                        uph_song_timeline_data.disable_space_to_play = false;
                        current_rename_index = -1;
                    }
                }
                else
                {
                    if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph_ui_widget_hovered(name_id))
                    {
                        uph_song_timeline_data.disable_space_to_play = true;
                        current_rename_index = track_index;
                    }

                    leaf({
                        .id = name_id
                    })
                    {
                        leaf_text(track->name.data, {
                            .font_size = NAUI_DPI(14.0f),
                            .color = text_color
                        });
                    }
                }
            }


            leaf({
                .direction = LEAF_DIRECTION_HORIZONTAL,
                .child_gap = NAUI_DPI(2.0f)
            })
            {
                if (uph_ui_text_toggle_button("M", leaf_id_indexed("uph_track_mute_toggle", track_index), track->state & UPH_TRACK_MUTED))
                    track->state ^= UPH_TRACK_MUTED;
                if (uph_ui_text_toggle_button("S", leaf_id_indexed("uph_track_solo_toggle", track_index), track->state & UPH_TRACK_SOLOED))
                    uph_song_timeline_solo_track(track_index);

                const float size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
                if (uph_ui_image_toggle_button(
                    naui_asset_image("uph_icon_mic"),
                    leaf_id_indexed("uph_track_arm_toggle", track_index),
                    (Naui_Vec2) { size, size },
                    text_color,
                    track->state & UPH_TRACK_ARMED
                )) track->state ^= UPH_TRACK_ARMED;
            }

            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
                .child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_TOP}
            })
            {
                if (uph_ui_text_button("...", leaf_id_indexed("uph_track_options", track_index)))
                {
                    uph_song_timeline_data.current_options_track_index = track_index;
                    uph_ui_open_context_menu(options_menu);
                }
            }
        }
    }
}

static void uph_song_timeline_render_track(uint32_t track_index, Uph_UIMenuID options_menu)
{
    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_data.zoom.y))}
    })
    {
        uph_song_timeline_render_track_header(track_index, options_menu);
        uph_song_timeline_render_track_timeline(track_index);
    }
}

static void uph_song_timeline_render_toolbox(void)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32.0f))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
        .color = naui_theme_color("uph_ui_toolbox_bg_color"),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .child_gap = NAUI_DPI(16.0f),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        const float button_size = NAUI_DPI(14.0f);
        const float button_gap = NAUI_DPI(2.0f);
        const Naui_Color bg_color = naui_theme_color("uph_ui_frame_secondary_bg_color");
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
        })
        {
            const Naui_Color icon_color = naui_theme_color("uph_tool_icon_color");
            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_select"),
                leaf_id("uph_song_timeline_select"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_TL | NAUI_CORNER_BL,
                uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT
            )) uph_song_timeline_data.current_action_mode = UPH_ACTION_SELECT;

            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_draw"),
                leaf_id("uph_song_timeline_draw"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_NONE,
                uph_song_timeline_data.current_action_mode == UPH_ACTION_DRAW
            )) uph_song_timeline_data.current_action_mode = UPH_ACTION_DRAW;

            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_cut"),
                leaf_id("uph_song_timeline_cut"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_TR | NAUI_CORNER_BR,
                uph_song_timeline_data.current_action_mode == UPH_ACTION_CUT
            )) uph_song_timeline_data.current_action_mode = UPH_ACTION_CUT;
        }
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        })
        {
            if (uph_ui_image_button_ex(
                naui_asset_image(uph_state.shared.song_timeline_playing ? "uph_icon_pause" : "uph_icon_play"),
                leaf_id("uph_song_timeline_play"),
                (Naui_Vec2){button_size, button_size},
                naui_theme_color(uph_state.shared.song_timeline_playing ? "uph_pause_icon_color" : "uph_play_icon_color"),
                bg_color,
                NAUI_CORNER_TL | NAUI_CORNER_BL
            ))
            {
                uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
            }

            if (uph_ui_image_button_ex(
                naui_asset_image("uph_icon_stop"),
                leaf_id("uph_song_timeline_stop"),
                (Naui_Vec2){button_size, button_size},
                naui_theme_color("uph_stop_icon_color"),
                bg_color,
                NAUI_CORNER_TR | NAUI_CORNER_BR
            ))
            {
                uph_state.shared.song_timeline_playing = false;
                uph_state.shared.song_timeline_playhead_position = 0.0;
            }
        }
    }
}

static void uph_song_timeline_render_top_bar(void)
{
    const float header_width = naui_theme_float("uph_track_header_width");
    const Naui_Vec2 header_padding = naui_theme_vec2("uph_track_header_padding");

    const float height = NAUI_DPI(32.0f);
    const float half_height = height * 0.5f;

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(height)}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width + header_padding.x * 2.0f)), LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        });
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .custom_draw = (Leaf_CustomDrawFn)uph_song_timeline_render_top_ruler
        });
    }
}

static float uph_song_timeline_max_scroll_y(void)
{
    const uint32_t track_count = (uint32_t)naui_list_len(uph_state.project.tracks);

    if (track_count == 0)
        return 0.0f;

    return (float)(track_count - 1) * NAUI_DPI(uph_song_timeline_data.zoom.y);
}

static void uph_song_timeline_update_input(void)
{
    const float wheel_y = (float)naui_mouse_scroll_delta();
    const bool ctrl_held = naui_key_down(NAUI_KEY_LCONTROL);
    const float max_scroll_y = uph_song_timeline_max_scroll_y();

    if (uph_song_timeline_data.panel_hovered)
    {
        if (ctrl_held && wheel_y != 0.0f)
        {
            const float old_zoom_x = uph_song_timeline_data.zoom.x;

            const double mouse_beat_before =
                ((double)naui_mouse_x() + uph_song_timeline_data.scroll.x) / old_zoom_x;

            float new_zoom_x = old_zoom_x * (1.0f + wheel_y * UPH_SONG_TIMELINE_ZOOM_SPEED);
            new_zoom_x = NAUI_CLAMP(new_zoom_x, UPH_SONG_TIMELINE_ZOOM_X_MIN, UPH_SONG_TIMELINE_ZOOM_X_MAX);

            uph_song_timeline_data.zoom.x = new_zoom_x;

            uph_song_timeline_data.scroll.x =
                (float)(mouse_beat_before * new_zoom_x) - (float)naui_mouse_x();
            uph_song_timeline_data.scroll.x = fmaxf(0.0f, uph_song_timeline_data.scroll.x);
        }
        else if (wheel_y != 0.0f)
        {
            uph_song_timeline_data.scroll.y -= wheel_y * UPH_SONG_TIMELINE_SCROLL_Y_SPEED;
            uph_song_timeline_data.scroll.y = NAUI_CLAMP(uph_song_timeline_data.scroll.y, 0.0f, max_scroll_y);
        }
    }

    static Naui_Vec2 pan_last_mouse;
    static bool panning = false;

    if (uph_song_timeline_data.panel_hovered && naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
    {
        panning = true;
        pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
    }

    if (panning)
    {
        Naui_Vec2 current = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
        Naui_Vec2 delta = (Naui_Vec2) { current.x - pan_last_mouse.x, current.y - pan_last_mouse.y };

        uph_song_timeline_data.scroll.x -= delta.x * UPH_SONG_TIMELINE_PAN_SPEED;
        uph_song_timeline_data.scroll.x = fmaxf(0.0f, uph_song_timeline_data.scroll.x);

        uph_song_timeline_data.scroll.y -= delta.y * UPH_SONG_TIMELINE_PAN_SPEED;
        uph_song_timeline_data.scroll.y = NAUI_CLAMP(uph_song_timeline_data.scroll.y, 0.0f, max_scroll_y);

        pan_last_mouse = current;

        if (naui_mouse_released(NAUI_MOUSE_MIDDLE))
            panning = false;
    }
}

static void uph_song_timeline_render_playhead_overlay(Leaf_BoundingBox bbox, void *data)
{
    const float x_offset = NAUI_DPI(naui_theme_float("uph_track_header_width") + naui_theme_vec2("uph_track_header_padding").x * 2.0f);
    
    const Naui_Image *playhead_image = naui_asset_image("uph_icon_playhead");
    const Naui_Color color = naui_theme_color("uph_playhead_color");

    const float playhead_size = NAUI_DPI(16.0f);
    const float playhead_half_size = playhead_size * 0.5f;

    naui_push_clip_rect(bbox.x + x_offset, bbox.y, bbox.width, bbox.height);
    if (uph_song_timeline_data.current_action_mode == UPH_ACTION_CUT)
    {
        const double mouse_beat = ((double)naui_mouse_x() - (bbox.x + x_offset) + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
        const double cut_beat = floor(mouse_beat);

        const float x = bbox.x + x_offset + (float)(cut_beat * uph_song_timeline_data.zoom.x) - uph_song_timeline_data.scroll.x;

        naui_draw_line(
            (Naui_Vec2) { x, bbox.y + playhead_size * 2.0f },
            (Naui_Vec2) { x, bbox.y + bbox.height },
            color,
            NAUI_DPI(1.0f)
        );
    }

    if (uph_state.shared.song_timeline_playhead_position > 0.0f)
    {
        const float x = bbox.x + x_offset + uph_state.shared.song_timeline_playhead_position * uph_song_timeline_data.zoom.x - uph_song_timeline_data.scroll.x;
        naui_draw_line(
            (Naui_Vec2) { x, bbox.y + playhead_size },
            (Naui_Vec2) { x, bbox.y + bbox.height },
            color,
            NAUI_DPI(1.0f)
        );

        naui_draw_image(playhead_image, (Naui_Vec2){x - playhead_half_size, bbox.y + playhead_size}, (Naui_Vec2){playhead_size, playhead_size}, color, 0.0f, NAUI_CORNER_NONE);
    }

    naui_pop_clip_rect();
}

static void uph_song_timeline_render_track_options_menu(Uph_SongTimelineData *data, Uph_UIMenuID track_options_context_menu)
{
    if (naui_list_len(uph_state.project.tracks) == 0)
        return;

    Uph_Track *track = &uph_state.project.tracks[data->current_options_track_index];

    {
        Uph_UIMenuID color_menu = uph_ui_submenu(track_options_context_menu, "Color", leaf_id("uph_track_options_color"));
        if (uph_ui_menu_item(color_menu, "Color 1", leaf_id("uph_track_options_color_1")))
            track->color = naui_theme_color("uph_palette_color_1");
        if (uph_ui_menu_item(color_menu, "Color 2", leaf_id("uph_track_options_color_2")))
            track->color = naui_theme_color("uph_palette_color_2");
        if (uph_ui_menu_item(color_menu, "Color 3", leaf_id("uph_track_options_color_3")))
            track->color = naui_theme_color("uph_palette_color_3");
        if (uph_ui_menu_item(color_menu, "Color 4", leaf_id("uph_track_options_color_4")))
            track->color = naui_theme_color("uph_palette_color_4");
        if (uph_ui_menu_item(color_menu, "Color 5", leaf_id("uph_track_options_color_5")))
            track->color = naui_theme_color("uph_palette_color_5");
        if (uph_ui_menu_item(color_menu, "Color 6", leaf_id("uph_track_options_color_6")))
            track->color = naui_theme_color("uph_palette_color_6");
        if (uph_ui_menu_item(color_menu, "Color 7", leaf_id("uph_track_options_color_7")))
            track->color = naui_theme_color("uph_palette_color_7");
        if (uph_ui_menu_item(color_menu, "Color 8", leaf_id("uph_track_options_color_8")))
            track->color = naui_theme_color("uph_palette_color_8");
    }

    if (uph_ui_menu_item(track_options_context_menu, "Remove", leaf_id("uph_track_options_remove"))) 
    {
        naui_list_free(track->blocks);
        naui_list_remove(uph_state.project.tracks, data->current_options_track_index);
    }
}

static void uph_song_timeline_on_update(void)
{
    const Leaf_ID track_section_id = leaf_id("uph_track_section");

    Uph_SongTimelineData *data = &uph_song_timeline_data;

    data->panel_bounding_box = leaf_get_bounding_box(track_section_id);
    data->panel_hovered = naui_panel_hovered(naui_current_panel());
    data->tracks_hovered = leaf_hovered(track_section_id) && data->panel_hovered;
    data->hovered_block.active = false;

    if (naui_key_pressed(NAUI_KEY_SPACE) && !data->disable_space_to_play && data->panel_hovered)
        uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
    
    uph_song_timeline_update_input();
    uph_song_timeline_update_drag_track_switch();

    uph_song_timeline_render_toolbox();

    Uph_UIMenuID track_options_context_menu = uph_ui_context_menu();

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW}
    })
    {
        uph_song_timeline_render_top_bar();
        leaf({
            .id = track_section_id,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .child_offset = {0.0f, data->scroll.y},
            .clip_children = true
        })
        {
            for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
                uph_song_timeline_render_track(i, track_options_context_menu);
        }
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .custom_draw = uph_song_timeline_render_playhead_overlay
        });
    }

    uph_song_timeline_render_track_options_menu(data, track_options_context_menu);
}