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
    Uph_Track *track;
    bool active;
    Uph_BlockInteractionMode mode;
}
Uph_DraggingBlockState;

typedef struct
{
    uint32_t block_index;
    Uph_Track *track;
    int32_t dragging_point_index;
    bool active;
}
Uph_AutomationEditState;

typedef struct
{
    uint32_t block_index;
    Uph_Track *track;
    bool active;
}
Uph_HoveredBlockState;

typedef struct
{
    Naui_Vec2 scroll;
    Naui_Vec2 zoom;
    Uph_DraggingBlockState drag;
    Uph_HoveredBlockState hovered_block;
    Uph_AutomationEditState automation_edit;
    Leaf_BoundingBox panel_bounding_box;
    Uph_Track *current_options_track;
    Uph_Track *current_hovered_track;
    uint32_t visual_row_counter;
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
    if (uph_state.shared.current_pattern_updated && block_type == UPH_RESOURCE_PATTERN)
    {
        uph_state.shared.song_timeline_current_block_start_offset = 0.0;
        uph_state.shared.song_timeline_current_block_length =
            uph_calculate_pattern_length(&uph_state.project.midi_patterns[resource_index]);
        uph_state.shared.current_pattern_updated = false;
    }

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
    uph_song_timeline_data.zoom = (Naui_Vec2) { 32.0f, 100.0f };
    uph_song_timeline_data.current_action_mode = UPH_ACTION_DRAW;
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

static void uph_song_timeline_render_automation(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Naui_Color color,
    double start_offset,
    Leaf_BoundingBox visible_bbox,
    Uph_Automation* automation
)
{
    const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
    const float zoom_x = uph_song_timeline_data.zoom.x;

    if (point_count == 0)
        return;

    const int32_t point_size = NAUI_DPI(6);
    const int32_t half_point_size = point_size / 2;

    const Naui_Color fill_color = {
        color.r, color.g, color.b, (uint8_t)(color.a * 0.25f)
    };

    const float baseline_y = position.y + size.y;

    {
        const Uph_AutomationPoint *first_point = &automation->points[0];
        const Naui_Vec2 first_point_position = {
            position.x + (float)(first_point->beat - start_offset) * zoom_x,
            position.y + size.y - first_point->value * size.y
        };

        naui_fill_rect(
            (Naui_Vec2) { first_point_position.x - half_point_size, first_point_position.y - half_point_size },
            (Naui_Vec2) { point_size, point_size },
            color,
            FLT_MAX,
            NAUI_CORNER_ALL
        );
    }

    for (uint32_t i = 0; i < point_count - 1; i++)
    {
        const Uph_AutomationPoint *prev_point = &automation->points[i];
        const Uph_AutomationPoint *point = &automation->points[i + 1];

        const Naui_Vec2 prev_point_position = { position.x + (float)(prev_point->beat - start_offset) * zoom_x, position.y + size.y - prev_point->value * size.y };
        const Naui_Vec2 point_position = { position.x + (float)(point->beat - start_offset) * zoom_x, position.y + size.y - point->value * size.y };

        const Naui_Vec2 fill_poly[4] = {
            prev_point_position,
            point_position,
            (Naui_Vec2) { point_position.x, baseline_y },
            (Naui_Vec2) { prev_point_position.x, baseline_y },
        };
        naui_fill_polygon(fill_poly, 4, fill_color);

        naui_draw_line(
            prev_point_position,
            point_position,
            color,
            1.0f
        );
        naui_fill_rect(
            (Naui_Vec2) { point_position.x - half_point_size, point_position.y - half_point_size },
            (Naui_Vec2) { point_size, point_size },
            color,
            FLT_MAX,
            NAUI_CORNER_ALL
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
    else if (block->type == UPH_RESOURCE_AUTOMATION)
    {
        naui_draw_text(
            (Naui_Vec2) { position.x + title_padding, position.y + title_padding },
            uph_state.project.automations[block->resource_index].name.data,
            font_size,
            0,
            leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity))
        );

        uph_song_timeline_render_automation(
            (Naui_Vec2) { position.x, position.y + title_height },
            (Naui_Vec2) { size.x, size.y - title_height },
            leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity)),
            block->start_offset_beats,
            visible_bbox,
            &uph_state.project.automations[block->resource_index]
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

    Uph_Track *new_track = uph_song_timeline_data.current_hovered_track;

    if (!new_track || new_track == drag->track)
        return;

    Uph_Track *old_track = drag->track;

    if (new_track->type != UPH_RESOURCE_NONE && new_track->type != old_track->type)
        return;
    
    if (old_track->type == UPH_RESOURCE_AUTOMATION && new_track->type != UPH_RESOURCE_AUTOMATION)
        return;

    Uph_TimelineBlock moved = old_track->blocks[drag->block_index];
    naui_list_uremove(old_track->blocks, drag->block_index);
    naui_list_push(new_track->blocks, moved);

    if (old_track->type != UPH_RESOURCE_AUTOMATION)
    {
        if (new_track->type == UPH_RESOURCE_NONE)
            new_track->type = moved.type;
        if (naui_list_len(old_track->blocks) == 0 && !old_track->instrument.loaded)
            old_track->type = UPH_RESOURCE_NONE;
    }

    drag->track = new_track;
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

static void uph_song_timeline_update_automation_point_drag(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Uph_Track *track,
    uint32_t block_index,
    double start_offset,
    Uph_Automation *automation
)
{
    Uph_AutomationEditState *edit = &uph_song_timeline_data.automation_edit;
    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;
    const int32_t point_size = NAUI_DPI(6);
    const int32_t hit_radius = point_size;

    const bool is_target_block = (edit->track == track && edit->block_index == block_index);

    if (is_target_block && edit->dragging_point_index >= 0)
    {
        const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
        const int32_t idx = edit->dragging_point_index;
        Uph_AutomationPoint *point = &automation->points[idx];

        const float mouse_y = (float)naui_mouse_y();
        const float clamped_y = fminf(fmaxf(mouse_y, position.y), position.y + size.y);
        point->value = 1.0f - (double)((clamped_y - position.y) / size.y);
        point->value = fmax(0.0, fmin(1.0, point->value));

        if (idx != 0)
        {
            const double mouse_beat_raw =
                ((double)naui_mouse_x() - position.x + scroll_x) / zoom_x + start_offset;
            double snapped_beat = round(mouse_beat_raw);

            const double prev_beat = automation->points[idx - 1].beat;
            snapped_beat = fmax(snapped_beat, prev_beat);

            if ((uint32_t)idx + 1 < point_count)
            {
                const double next_beat = automation->points[idx + 1].beat;
                snapped_beat = fmin(snapped_beat, next_beat);
            }

            point->beat = snapped_beat;
        }

        naui_set_cursor(NAUI_CURSOR_HAND);

        if (naui_mouse_released(NAUI_MOUSE_LEFT))
            edit->dragging_point_index = -1;

        return;
    }

    const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
    bool hovering_existing_point = false;

    for (uint32_t i = 0; i < point_count; i++)
    {
        Uph_AutomationPoint *point = &automation->points[i];

        const float px = position.x + (float)((point->beat - start_offset) * zoom_x);
        const float py = position.y + size.y - (float)(point->value * size.y);

        const float dx = (float)naui_mouse_x() - px;
        const float dy = (float)naui_mouse_y() - py;

        if (dx * dx + dy * dy <= (float)(hit_radius * hit_radius))
        {
            hovering_existing_point = true;

            naui_set_cursor(NAUI_CURSOR_HAND);

            if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
            {
                edit->track = track;
                edit->block_index = block_index;
                edit->dragging_point_index = (int32_t)i;
            }

            break;
        }
    }

    if (!hovering_existing_point &&
        naui_mouse_pressed(NAUI_MOUSE_LEFT) &&
        upb_song_timeline_vec4_contains_vec2(
            (Naui_Vec4) { position.x, position.y, size.x, size.y },
            (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }
        ))
    {
        const double mouse_beat_raw =
            ((double)naui_mouse_x() - position.x + scroll_x) / zoom_x + start_offset;
        double new_beat = round(mouse_beat_raw);
        new_beat = fmax(new_beat, 0.0);

        const float mouse_y = (float)naui_mouse_y();
        const float clamped_y = fminf(fmaxf(mouse_y, position.y), position.y + size.y);
        double new_value = 1.0f - (double)((clamped_y - position.y) / size.y);
        new_value = fmax(0.0, fmin(1.0, new_value));

        uint32_t insert_index = point_count;
        for (uint32_t i = 0; i < point_count; i++)
        {
            if (new_beat < automation->points[i].beat)
            {
                insert_index = i;
                break;
            }
        }

        Uph_AutomationPoint new_point = { .beat = new_beat, .value = new_value };
        naui_list_insert(automation->points, new_point, insert_index);

        edit->track = track;
        edit->block_index = block_index;
        edit->dragging_point_index = (int32_t)insert_index;
    }
}

static void uph_song_timeline_update_track_timeline_drag(Leaf_BoundingBox bbox, Uph_Track *track)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    Uph_AutomationEditState *automation_edit = &uph_song_timeline_data.automation_edit;
    Naui_List(Uph_TimelineBlock) blocks = track->blocks;
    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;

    const float title_padding = NAUI_DPI(2.0f);
    const float font_size = NAUI_DPI(13.0f);
    const float title_height = font_size + title_padding * 2.0f;

    if (automation_edit->dragging_point_index >= 0 && automation_edit->track == track)
    {
        const uint32_t i = automation_edit->block_index;
        const float block_left = bbox.x + zoom_x * blocks[i].start_beat - scroll_x;

        Naui_Vec2 automation_pos = { block_left, bbox.y + title_height };
        Naui_Vec2 automation_size = { zoom_x * blocks[i].length_beats, bbox.height - title_height };

        uph_song_timeline_update_automation_point_drag(
            automation_pos,
            automation_size,
            track,
            i,
            blocks[i].start_offset_beats,
            &uph_state.project.automations[blocks[i].resource_index]
        );
        automation_edit->active = true;
        return;
    }

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
    {
        bool is_dragging_this_block =
            drag->active &&
            drag->track == track &&
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

                uph_state.shared.current_pattern_updated = false;

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }
            else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
            {
                double new_length = ceil(mouse_beat) - blocks[i].start_beat;
                blocks[i].length_beats = fmax(1.0, new_length);

                uph_state.shared.song_timeline_current_block_length = blocks[i].length_beats;
                uph_state.shared.song_timeline_current_block_start_offset = blocks[i].start_offset_beats;

                uph_state.shared.current_pattern_updated = false;

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
                blocks[i].type == UPH_RESOURCE_AUTOMATION ? title_height : bbox.height
            };

            if (upb_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
            {
                Uph_BlockInteractionMode hover_mode =
                    uph_song_timeline_classify_hover(hover_box, (float)naui_mouse_x());

                if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
                {
                    drag->active = true;
                    drag->track = track;
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

                    uph_state.shared.current_pattern_updated = false;
                }

                naui_set_cursor(
                    hover_mode == UPH_BLOCK_INTERACTION_MOVE
                        ? NAUI_CURSOR_HAND
                        : NAUI_CURSOR_RESIZE_EW
                );

                uph_song_timeline_data.hovered_block.block_index = i;
                uph_song_timeline_data.hovered_block.track = track;
                uph_song_timeline_data.hovered_block.active = true;
            }

            if (blocks[i].type == UPH_RESOURCE_AUTOMATION)
            {
                hover_box.y += title_height;
                hover_box.w = bbox.height - title_height;

                if (upb_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
                {
                    Naui_Vec2 automation_pos = { clamped_left, bbox.y + title_height };
                    Naui_Vec2 automation_size = { clamped_right - clamped_left, bbox.height - title_height };

                    uph_song_timeline_update_automation_point_drag(
                        automation_pos,
                        automation_size,
                        track,
                        i,
                        blocks[i].start_offset_beats,
                        &uph_state.project.automations[blocks[i].resource_index]
                    );
                    automation_edit->active = true;
                }
            }
        }
    }
}

static void uph_song_timeline_render_track_timeline_blocks(Leaf_BoundingBox bbox, Uph_Track *track)
{
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

static void uph_song_timeline_update_track_action_input(Leaf_BoundingBox bbox, Uph_Track *track)
{
    if (!uph_song_timeline_data.tracks_hovered)
        return;

    if (uph_song_timeline_data.automation_edit.active)
        return;

    if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track == track)
    {
        naui_list_uremove(track->blocks, uph_song_timeline_data.hovered_block.block_index);
        if (naui_list_len(track->blocks) == 0 && !track->instrument.loaded && track->type != UPH_RESOURCE_AUTOMATION)
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

        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && !uph_song_timeline_data.hovered_block.active &&
            (uph_state.shared.selected_resource.type == UPH_RESOURCE_AUTOMATION
                ? track->type == UPH_RESOURCE_AUTOMATION
                : (track->type == UPH_RESOURCE_NONE || track->type == uph_state.shared.selected_resource.type)))
        {
            if (upb_song_timeline_vec4_contains_vec2(
                (Naui_Vec4) {bbox.x, bbox.y, bbox.width, bbox.height},
                (Naui_Vec2) {naui_mouse_x(), naui_mouse_y()}
            ))
            {
                const float beat = (naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
                uph_song_timeline_data.drag.active = true;
                uph_song_timeline_data.drag.block_index = naui_list_len(track->blocks);
                uph_song_timeline_data.drag.track = track;
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
            uph_song_timeline_data.hovered_block.track == track)
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

static void uph_song_timeline_render_track_timeline_overlay(Leaf_BoundingBox bbox, Uph_Track **track_ptr)
{
    Uph_Track *track = *track_ptr;

    const bool mouse_over_track = upb_song_timeline_vec4_contains_vec2(
        (Naui_Vec4) { bbox.x, bbox.y, bbox.width, bbox.height },
        (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }
    );

    if (mouse_over_track)
        uph_song_timeline_data.current_hovered_track = track;

    naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);

    uph_song_timeline_render_ruler(bbox, uph_song_timeline_data.zoom.x, uph_song_timeline_data.scroll.x);

    uph_song_timeline_update_track_timeline_drag(bbox, track);
    uph_song_timeline_update_track_action_input(bbox, track);
    uph_song_timeline_render_track_timeline_blocks(bbox, track);

    naui_pop_clip_rect();
}

static void uph_song_timeline_render_track_timeline(Uph_Track *track)
{
    const uint32_t row_counter = uph_song_timeline_data.visual_row_counter;

    const Leaf_Color bg_color = row_counter & 1 ? naui_theme_color("uph_track_bg1_color") : naui_theme_color("uph_track_bg2_color");
    const Leaf_Color border_color = naui_theme_color("uph_track_border_color");

    const float row_height = NAUI_DPI(uph_song_timeline_data.zoom.y);
    const float row_y = uph_song_timeline_data.panel_bounding_box.y
        - uph_song_timeline_data.scroll.y
        + (float)row_counter * row_height;

    const bool row_visible =
        row_y + row_height >= uph_song_timeline_data.panel_bounding_box.y &&
        row_y <= uph_song_timeline_data.panel_bounding_box.y + uph_song_timeline_data.panel_bounding_box.height;

    leaf({
        .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
        .color = bg_color,
        .border = {
            .width = 1,
            .color = {border_color},
            .sides = LEAF_SIDE_TOP | LEAF_SIDE_BOTTOM
        },
        .custom_draw_data = row_visible ? LEAF_DATA_SLICE(track) : (Leaf_DataSlice){ 0 },
        .custom_draw = row_visible ? (Leaf_CustomDrawFn)uph_song_timeline_render_track_timeline_overlay : NULL
    });
}

static void uph_song_timeline_solo_track(Uph_Track *track)
{
    track->state ^= UPH_TRACK_SOLOED;

    bool is_soloed = track->state & UPH_TRACK_SOLOED;

    if (is_soloed)
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
        {
            Uph_Track *t = &uph_state.project.tracks[i];
            if (t == track)
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

static void uph_song_timeline_render_track_header(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu)
{
    const int32_t depth_offset = depth * 13;

    const Leaf_Color bg_color = naui_theme_color("uph_track_header_color");
    const Leaf_Color text_color = naui_theme_color("uph_track_text_color");
    const Leaf_Color border_color = naui_theme_color("uph_track_header_border_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_track_header_padding");
    const float header_width = naui_theme_float("uph_track_header_width");

    uint64_t track_id = (uint64_t)track;

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width + padding.x * 2.0f)), LEAF_SIZE_FULL}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(depth_offset)), LEAF_SIZE_FULL}
        });
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .color = bg_color,
            .child_gap = NAUI_DPI(10),
            .border = {
                .width = 1,
                .color = {border_color},
                .sides = LEAF_SIDE_ALL
            }
        })
        {

            leaf({
                .size = {LEAF_SIZE_FIXED(NAUI_DPI(5)), LEAF_SIZE_FULL},
                .color = {track->color},
                .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
            });

            leaf({
                .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
                .child_gap = NAUI_DPI(4)
            })
            {
                const int32_t button_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));

                leaf({
                    .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                    .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                    .direction = LEAF_DIRECTION_HORIZONTAL
                })
                {
                    leaf({
                        .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
                        .direction = LEAF_DIRECTION_HORIZONTAL,
                        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                        .child_gap = NAUI_DPI(4)
                    })
                    {
                        if (track->type != UPH_RESOURCE_NONE)
                        {
                            Naui_Image *icon;
                            switch (track->type)
                            {
                            case UPH_RESOURCE_SAMPLE: icon = naui_asset_image("uph_icon_wave"); break;
                            case UPH_RESOURCE_PATTERN: icon = naui_asset_image("uph_icon_piano"); break;
                            case UPH_RESOURCE_AUTOMATION: icon = naui_asset_image("uph_icon_automation"); break;
                            }
                            const float icon_size = NAUI_DPI(naui_theme_float("uph_track_icon_size"));
                            leaf({
                                .size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
                                .image = icon,
                                .color = {text_color}
                            });
                        }

                        static Uph_Track *current_rename_track = NULL;
                        Leaf_ID name_id = leaf_id_indexed("uph_track_name", track_id);

                        if (current_rename_track == track)
                        {
                            if (uph_ui_textfield(&track->name, name_id, UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, NAUI_TR("song_timeline.track.title")))
                            {
                                if (!track->name.length)
                                    track->name = naui_string_from_cstr(NAUI_TR("song_timeline.track.title"));
                                uph_song_timeline_data.disable_space_to_play = false;
                                current_rename_track = NULL;
                            }
                        }
                        else
                        {
                            if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph_ui_widget_hovered(name_id))
                            {
                                uph_song_timeline_data.disable_space_to_play = true;
                                current_rename_track = track;
                            }

                            leaf({
                                .id = name_id
                            })
                            {
                                leaf_text(track->name.data, {
                                    .font_size = {NAUI_DPI(14)},
                                    .color = {text_color}
                                });
                            }
                        }
                    }

                    if (uph_ui_image_button_ex(
                        naui_asset_image("uph_icon_gear"),
                        leaf_id_indexed("uph_track_options", track_id),
                        (Naui_Vec2){button_size,button_size},
                        text_color,
                        LEAF_COLOR_TRANSPARENT,
                        NAUI_CORNER_ALL
                    ))
                    {
                        uph_song_timeline_data.current_options_track = track;
                        uph_ui_open_context_menu(options_menu);
                    }
                }

                leaf({
                    .direction = LEAF_DIRECTION_HORIZONTAL,
                    .child_gap = NAUI_DPI(2)
                })
                {
                    if (uph_ui_text_toggle_button("M", leaf_id_indexed("uph_track_mute_toggle", track_id), track->state & UPH_TRACK_MUTED))
                        track->state ^= UPH_TRACK_MUTED;
                    if (uph_ui_text_toggle_button("S", leaf_id_indexed("uph_track_solo_toggle", track_id), track->state & UPH_TRACK_SOLOED))
                        uph_song_timeline_solo_track(track);

                    if (track->type != UPH_RESOURCE_AUTOMATION && uph_ui_image_toggle_button(
                        naui_asset_image("uph_icon_mic"),
                        leaf_id_indexed("uph_track_arm_toggle", track_id),
                        (Naui_Vec2) { button_size, button_size },
                        text_color,
                        track->state & UPH_TRACK_ARMED
                    )) track->state ^= UPH_TRACK_ARMED;
                }
            }
        }
    }
    
}

static void uph_song_timeline_render_track(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu)
{
    Uph_SongTimelineData *data = &uph_song_timeline_data;
    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_data.zoom.y))}
    })
    {
        uph_song_timeline_render_track_header(track, depth, options_menu);
        uph_song_timeline_render_track_timeline(track);
    }

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
    {
        data->visual_row_counter++;
        uph_song_timeline_render_track(&track->subtracks[i], depth + 1, options_menu);
    }
}

static void uph_song_timeline_render_toolbox(void)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(40.0f))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
        .color = {naui_theme_color("uph_ui_toolbox_bg_color")},
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .child_gap = NAUI_DPI(16.0f),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        const int32_t button_size = NAUI_DPI(16);
        const Naui_Color bg_color = naui_theme_color("uph_ui_frame_secondary_bg_color");
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
        })
        {
            Uph_SongTimelineData *data = &uph_song_timeline_data;
            const Naui_Color icon_color = naui_theme_color("uph_tool_icon_color");
            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_select"),
                leaf_id("uph_song_timeline_select"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_TL | NAUI_CORNER_BL,
                data->current_action_mode == UPH_ACTION_SELECT
            )) data->current_action_mode = UPH_ACTION_SELECT;

            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_draw"),
                leaf_id("uph_song_timeline_draw"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_NONE,
                data->current_action_mode == UPH_ACTION_DRAW
            )) data->current_action_mode = UPH_ACTION_DRAW;

            if (uph_ui_image_toggle_button_ex(
                naui_asset_image("uph_icon_cut"),
                leaf_id("uph_song_timeline_cut"),
                (Naui_Vec2){button_size, button_size},
                icon_color,
                bg_color,
                NAUI_CORNER_TR | NAUI_CORNER_BR,
                data->current_action_mode == UPH_ACTION_CUT
            )) data->current_action_mode = UPH_ACTION_CUT;
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

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(40))}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width)), LEAF_SIZE_FULL},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(header_padding.x), 0.0f),
            .child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
        })
        {
            const Naui_Color bg_color = naui_theme_color("uph_ui_frame_secondary_bg_color");
            if (uph_ui_text_button_ex(NAUI_TR("song_timeline.add.track"), leaf_id("uph_song_timeline_add_track"), bg_color, NAUI_CORNER_ALL))
                uph_resources_add_track(naui_string_from_cstr("New Track"));
        }

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
    if (naui_list_len(uph_state.project.tracks) == 0 || !data->current_options_track)
        return;

    Uph_Track *track = data->current_options_track;

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

    if (track->type != UPH_RESOURCE_AUTOMATION)
    {
        if (track->instrument.loaded)
        {
            const bool visible = uph_plugin_window_visible(&track->instrument);
            if (uph_ui_menu_item(track_options_context_menu, visible ? "Hide Instrument" : "Show Instrument", leaf_id("uph_track_options_show_instrument"))) 
            {
                if (visible)
                    uph_hide_plugin_window(&track->instrument);
                else uph_show_plugin_window(&track->instrument);
            }

            if (uph_ui_menu_item(track_options_context_menu, "Remove Instrument", leaf_id("uph_track_options_remove_instrument"))) 
            {
                uph_unload_plugin_effect(&track->instrument);
                if (naui_list_len(track->blocks) == 0)
                    track->type = UPH_RESOURCE_NONE;
            }
        }
        else
        {
            Uph_UIMenuID instrument_menu = uph_ui_submenu(track_options_context_menu, "Load Instrument", leaf_id("uph_track_options_load_instrument"));
            for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.settings.plugin.plugin_paths); i++)
            {
                Naui_Path parent_path = uph_state.settings.plugin.plugin_paths[i];
                static Naui_List(Naui_DirEntry) entries = NULL;
                if (!entries)
                    entries = naui_directory_filter_recursive(parent_path, "*", NAUI_EXTENSIONS(".clap", ".vst3"));
                for (uint32_t j = 0; j < (uint32_t)naui_list_len(entries); j++)
                {
                    if (uph_ui_menu_item(instrument_menu, naui_view_to_string(naui_file_stem(&entries[j].path)).data, leaf_id_indexed("uph_track_options_instrument", j))) 
                    {
                        track->instrument = uph_load_plugin_effect(entries[j].path);
                        track->type = UPH_RESOURCE_PATTERN;
                    }
                }
            }
        }

        if (uph_ui_menu_item(track_options_context_menu, "Automate", leaf_id("uph_track_options_automate")))
            uph_resources_add_automation_track(track, naui_string_from_cstr("Property name"));
    }

    if (uph_ui_menu_item(track_options_context_menu, "Remove", leaf_id("uph_track_options_remove"))) 
        uph_resources_remove_track(track);
}

static void uph_song_timeline_on_update(void)
{
    Uph_SongTimelineData *data = &uph_song_timeline_data;

    const Leaf_ID track_section_id = leaf_id("uph_track_section");

    data->panel_bounding_box = leaf_get_bounding_box(track_section_id);
    data->panel_hovered = naui_panel_hovered(naui_current_panel());
    data->tracks_hovered = leaf_hovered(track_section_id) && data->panel_hovered;
    if (data->automation_edit.dragging_point_index < 0)
    {
        data->automation_edit.block_index = -1;
        data->automation_edit.track = NULL;
    }
    data->automation_edit.active = false;
    data->hovered_block.active = false;
    data->visual_row_counter = 0;

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
            {
                uph_song_timeline_render_track(&uph_state.project.tracks[i], 0, track_options_context_menu);
                data->visual_row_counter++;
            }
        }
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .custom_draw = uph_song_timeline_render_playhead_overlay
        });
    }

    uph_song_timeline_render_track_options_menu(data, track_options_context_menu);
}