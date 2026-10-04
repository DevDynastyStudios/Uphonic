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
    Uph_ActionTrackRef source_track;
    uint32_t source_index;
    Naui_List(Uph_ActionBlockSnapshot) group_blocks;
    bool active;
    bool creating;
    bool group;
    bool additive;
    Uph_BlockInteractionMode mode;
}
Uph_DraggingBlockState;

typedef struct
{
    uint32_t block_index;
    Uph_Track *track;
    int32_t dragging_point_index;
    Uph_ResourceIndex automation_index;
    Uph_AutomationPoint initial_point;
    bool creating;
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
    double start_beat;
    float start_content_y;
    bool active;
}
Uph_MarqueeState;

typedef struct
{
    Naui_Vec2 scroll;
    Naui_Vec2 zoom;
    Uph_DraggingBlockState drag;
    Uph_HoveredBlockState hovered_block;
    Uph_AutomationEditState automation_edit;
    Uph_MarqueeState marquee;
    Leaf_BoundingBox panel_bounding_box;
    Uph_Track *current_options_track;
    Uph_Track *current_hovered_track;
    Uph_Track *rename_track;
    Naui_String rename_old_name;
    uint32_t visual_row_counter;
    Uph_SnapResolution snap_resolution;
	Uph_ActionMode current_action_mode;
    bool panel_hovered;
    bool tracks_hovered;
    bool disable_space_to_play;
}
Uph_SongTimelineData;

static Uph_SongTimelineData uph_song_timeline_data;

NAUI_PANEL(uph_song_timeline)

#pragma region Temp Helpers
static inline bool uph_ui_ctrl_down(void)	// Simply because Naui is broken
{
    return naui_key_down(NAUI_KEY_CONTROL) || naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL);
}

static inline bool uph_ui_shift_down(void)
{
    return naui_key_down(NAUI_KEY_SHIFT) || naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT);
}
#pragma endregion

void uph_song_timeline_invalidate_tracks(void)
{
    Uph_SongTimelineData *data = &uph_song_timeline_data;
    naui_list_free(data->drag.group_blocks);
    data->drag.group_blocks = NULL;
    data->drag.active = false;
    data->drag.creating = false;
    data->drag.group = false;
    data->drag.track = NULL;
    data->drag.mode = UPH_BLOCK_INTERACTION_NONE;

    data->automation_edit.dragging_point_index = -1;
    data->automation_edit.creating = false;
    data->automation_edit.track = NULL;

    data->hovered_block.active = false;
    data->hovered_block.track = NULL;
    data->current_options_track = NULL;
    data->current_hovered_track = NULL;
    data->marquee.active = false;

    if (data->rename_track)
    {
        data->rename_track = NULL;
        data->disable_space_to_play = false;
    }
}

static Uph_TimelineBlock uph_song_timeline_init_block(double start_beat, uint32_t resource_index, Uph_ResourceType block_type)
{
    if (uph_state.shared.current_pattern_updated && block_type == UPH_RESOURCE_PATTERN)
    {
        uph_state.shared.song_timeline_current_block_start_offset = 0.0;
        uph_state.shared.song_timeline_current_block_length = uph_calculate_pattern_length(&uph_state.project.midi_patterns[resource_index]);
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
    uph_song_timeline_data.zoom = (Naui_Vec2) { NAUI_DPI(64.0f), 90.0f };
    uph_song_timeline_data.snap_resolution = UPH_SNAP_QUARTER;
    uph_song_timeline_data.current_action_mode = UPH_ACTION_DRAW;
    uph_song_timeline_data.automation_edit.dragging_point_index = -1;
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
    const Leaf_Color beat_color = naui_theme_color("uph_timeline_grid_beat_color");
    const Leaf_Color bar_color = naui_theme_color("uph_timeline_grid_bar_color");
    const Leaf_Color sub_color = naui_theme_color("uph_timeline_grid_subbeat_color");

    const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
    const float division_px = (float)(division * zoom_x);
    const bool draw_subdivisions = division < 1.0 && division_px >= 3.0f;

    const int64_t first_div = (int64_t)floor((double)scroll_x / division_px);
    const uint32_t div_count = (uint32_t)(bbox.width / division_px) + 2;

    for (uint32_t i = 0; i < div_count; i++)
    {
        const int64_t div_index = first_div + (int64_t)i;
        if (div_index < 0)
            continue;

        const float x = bbox.x + (float)div_index * division_px - scroll_x;
        if (x < bbox.x || x > bbox.x + bbox.width)
            continue;

        const double divs_per_beat = 1.0 / division;
        const bool is_beat = fmod((double)div_index, divs_per_beat) < 0.5;

        if (!is_beat)
        {
            if (draw_subdivisions)
                naui_draw_line((Naui_Vec2){x, bbox.y}, (Naui_Vec2){x, bbox.y + bbox.height}, sub_color, 1.0f);
            continue;
        }

        const int64_t beat_index = (int64_t)llround((double)div_index / divs_per_beat);
        const bool is_downbeat = (beat_index % 4) == 0;
        const Leaf_Color line_color = is_downbeat ? bar_color : beat_color;

        naui_draw_line((Naui_Vec2){x, bbox.y}, (Naui_Vec2){x, bbox.y + bbox.height}, line_color, 1.0f);
    }
}

static inline bool uph_song_timeline_vec4_contains_vec2(const Naui_Vec4 rect, const Naui_Vec2 point)
{
    return point.x >= rect.x && point.x < rect.x + rect.z && point.y >= rect.y && point.y < rect.y + rect.w;
}

static void uph_song_timeline_update_playhead_drag(Leaf_BoundingBox bbox)
{
    Uph_SongTimelineData *data = &uph_song_timeline_data;

    static bool dragging_playhead;

    const bool mouse_over_ruler = uph_song_timeline_vec4_contains_vec2(
        (Naui_Vec4) { bbox.x, bbox.y, bbox.width, bbox.height },
        (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }
    );

    if (!dragging_playhead && mouse_over_ruler && naui_mouse_pressed(NAUI_MOUSE_LEFT) && naui_panel_hovered(uph_state.panels.song_timeline))
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

static void uph_song_timeline_render_top_ruler(Leaf_BoundingBox bbox, void *user_data)
{
    const Leaf_Color beat_color = naui_theme_color("uph_timeline_top_ruler_grid_beat_color");
    const Leaf_Color bar_color = naui_theme_color("uph_timeline_top_ruler_grid_bar_color");
    const Leaf_Color sub_color = naui_theme_color("uph_timeline_top_ruler_grid_subbeat_color");
    const Leaf_Color number_color = naui_theme_color("uph_timeline_top_ruler_grid_text_color");

    const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
    const float division_px = (float)(division * uph_song_timeline_data.zoom.x);
    const bool draw_subdivisions = division < 1.0 && division_px >= 3.0f;
    const double divs_per_beat = 1.0 / division;

    const float scroll_x = uph_song_timeline_data.scroll.x;

    const int64_t first_div = (int64_t)floor((double)scroll_x / division_px);
    const uint32_t div_count = (uint32_t)(bbox.width / division_px) + 2;

    uph_song_timeline_update_playhead_drag(bbox);

    naui_push_clip_rect(bbox.x - 0.5f, bbox.y, bbox.width, bbox.height);
    for (uint32_t i = 0; i < div_count; i++)
    {
        const int64_t div_index = first_div + (int64_t)i;
        if (div_index < 0)
            continue;

        const float x = bbox.x + (float)div_index * division_px - scroll_x;

        if (x < bbox.x - division_px || x > bbox.x + bbox.width)
            continue;

        const bool is_beat = fmod((double)div_index, divs_per_beat) < 0.5;

        if (!is_beat)
        {
            if (draw_subdivisions)
            {
                naui_draw_line(
                    (Naui_Vec2) { x, bbox.y + bbox.height * 0.6f },
                    (Naui_Vec2) { x, bbox.y + bbox.height },
                    sub_color,
                    1.0f
                );
            }
            continue;
        }

        const int64_t beat_index = (int64_t)llround((double)div_index / divs_per_beat);
        const bool is_downbeat = (beat_index % 4) == 0;
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
            snprintf(label, sizeof(label), "%d", (int)(beat_index / 4));
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


static void uph_song_timeline_render_timeline_block(Naui_Vec2 position, Naui_Vec2 size, Naui_Color color, float opacity, bool selected, const Uph_TimelineBlock *block, Leaf_BoundingBox visible_bbox)
{
    const float anim_scale = NAUI_DPI(30.0f);
    position.x += size.x * 0.5f;
    position.y += size.y * 0.5f;
    size.x = size.x + (easeOutElastic(block->visual_lifetime) - 1.0f) * anim_scale + 0.5f;
    size.y = size.y + (easeOutElastic(block->visual_lifetime) - 1.0f) * anim_scale;
    position.x -= size.x * 0.5f;
    position.y -= size.y * 0.5f;

    const float title_padding = NAUI_DPI(2.0f);
    const float font_size = NAUI_DPI(13.0f);
    const float title_height = font_size + title_padding * 2.0f;
    const float rounding = NAUI_DPI(4.0f);

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

    if (selected)
    {
        const float line_width = NAUI_DPI(2);
        position.x += line_width;
        position.y += line_width;
        size.x -= line_width * 2;
        size.y -= line_width * 2;
        naui_draw_rect(position, size, leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity)), line_width, rounding, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
    }
}

static void uph_song_timeline_update_drag_track_switch(void)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    if (!drag->active)
        return;

    if (drag->mode != UPH_BLOCK_INTERACTION_MOVE || drag->group)
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
    naui_list_remove(old_track->blocks, drag->block_index);
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

#pragma region Block Drag Actions

static bool uph_song_timeline_block_geometry_equal(const Uph_TimelineBlock *a, const Uph_TimelineBlock *b)
{
    return a->start_beat == b->start_beat && a->length_beats == b->length_beats && a->start_offset_beats == b->start_offset_beats;
}

static void uph_song_timeline_begin_block_drag(Uph_Track *track, uint32_t block_index, Uph_BlockInteractionMode mode, double mouse_beat)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    Uph_TimelineBlock *block = &track->blocks[block_index];

    naui_list_free(drag->group_blocks);
    drag->group_blocks = NULL;

    drag->active = true;
    drag->creating = false;
    drag->group = false;
    drag->additive = false;
    drag->track = track;
    drag->block_index = block_index;
    drag->source_track = uph_action_track_ref(track);
    drag->source_index = block_index;
    drag->mode = mode;
    drag->initial_start_beat = block->start_beat;
    drag->initial_length_beats = block->length_beats;
    drag->initial_start_offset_beats = block->start_offset_beats;
    drag->initial_drag_beat_offset = block->start_beat - mouse_beat;

    if (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT)
    {
        drag->additive = uph_ui_ctrl_down() || uph_ui_shift_down();
        if (drag->additive)
        {
            block->selected = !block->selected;
            if (!block->selected)
            {
                drag->active = false;
                drag->mode = UPH_BLOCK_INTERACTION_NONE;
                return;
            }
        }
        else if (!block->selected)
        {
            uph_block_select_all(false);
            block->selected = true;
        }

        drag->group = true;
        uph_block_collect_all(UPH_RESOURCE_NONE, true, false, 0, &drag->group_blocks);
    }

    uph_state.shared.selected_resource.type = block->type;
    uph_state.shared.selected_resource.index = block->resource_index;

    uph_state.shared.song_timeline_current_block_length = block->length_beats;
    uph_state.shared.song_timeline_current_block_start_offset = block->start_offset_beats;
    uph_state.shared.current_pattern_updated = false;
}

static void uph_song_timeline_apply_group_drag(const Uph_TimelineBlock *grabbed)
{
    const Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);

    double delta_start = grabbed->start_beat - drag->initial_start_beat;
    double delta_length = grabbed->length_beats - drag->initial_length_beats;

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_blocks); i++)
    {
        const Uph_TimelineBlock *initial = &drag->group_blocks[i].block;
        if (drag->mode == UPH_BLOCK_INTERACTION_MOVE)
        {
            delta_start = fmax(delta_start, -initial->start_beat);
        }
        else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
        {
            delta_start = fmax(delta_start, -initial->start_offset_beats);
            delta_start = fmin(delta_start, initial->length_beats - division);
        }
        else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
        {
            delta_length = fmax(delta_length, division - initial->length_beats);
        }
    }

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_blocks); i++)
    {
        const Uph_ActionBlockSnapshot *snapshot = &drag->group_blocks[i];
        Uph_Track *track = uph_action_track_resolve(snapshot->track);
        if (!track || snapshot->index >= (uint32_t)naui_list_len(track->blocks))
            continue;

        Uph_TimelineBlock *block = &track->blocks[snapshot->index];
        if (drag->mode == UPH_BLOCK_INTERACTION_MOVE)
        {
            block->start_beat = snapshot->block.start_beat + delta_start;
        }
        else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
        {
            block->start_beat = snapshot->block.start_beat + delta_start;
            block->length_beats = snapshot->block.length_beats - delta_start;
            block->start_offset_beats = snapshot->block.start_offset_beats + delta_start;
        }
        else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
        {
            block->length_beats = snapshot->block.length_beats + delta_length;
        }
    }
}

static void uph_song_timeline_finish_block_drag(void)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    const char *transform_action = drag->mode == UPH_BLOCK_INTERACTION_MOVE ? UPH_ACTION_BLOCK_MOVE : UPH_ACTION_BLOCK_RESIZE;

    if (drag->creating)
    {
        if (drag->block_index < (uint32_t)naui_list_len(drag->track->blocks))
        {
            Uph_ActionBlockCreate data = {
                .track = uph_action_track_ref(drag->track),
                .block_index = drag->block_index,
                .block = drag->track->blocks[drag->block_index],
                .applied_live = true
            };
            naui_action_execute_stack(UPH_ACTION_BLOCK_CREATE, data);
        }
    }
    else if (drag->group)
    {
        bool changed = false;
        naui_action_group_start(drag->mode == UPH_BLOCK_INTERACTION_MOVE ? UPH_ACTION_BLOCKS_MOVE_GROUP : UPH_ACTION_BLOCKS_RESIZE_GROUP);

        for (uint32_t i = 0; i < (uint32_t)naui_list_len(drag->group_blocks); i++)
        {
            const Uph_ActionBlockSnapshot *snapshot = &drag->group_blocks[i];
            Uph_Track *track = uph_action_track_resolve(snapshot->track);
            if (!track || snapshot->index >= (uint32_t)naui_list_len(track->blocks))
                continue;

            const Uph_TimelineBlock *block = &track->blocks[snapshot->index];
            if (uph_song_timeline_block_geometry_equal(block, &snapshot->block))
                continue;

            Uph_ActionBlockTransform data = {
                .src_track = snapshot->track, .src_index = snapshot->index,
                .dst_track = snapshot->track, .dst_index = snapshot->index,
                .old_start = snapshot->block.start_beat, .old_length = snapshot->block.length_beats, .old_offset = snapshot->block.start_offset_beats,
                .new_start = block->start_beat, .new_length = block->length_beats, .new_offset = block->start_offset_beats,
                .applied_live = true
            };
            naui_action_execute_stack(transform_action, data);
            changed = true;
        }

        naui_action_group_end();

        if (!changed && !drag->additive)
        {
            Uph_Track *source = uph_action_track_resolve(drag->source_track);
            if (source && drag->source_index < (uint32_t)naui_list_len(source->blocks))
            {
                uph_block_select_all(false);
                source->blocks[drag->source_index].selected = true;
            }
        }
    }
    else if (drag->block_index < (uint32_t)naui_list_len(drag->track->blocks))
    {
        const Uph_TimelineBlock *block = &drag->track->blocks[drag->block_index];
        const Uph_ActionTrackRef destination = uph_action_track_ref(drag->track);
        const bool moved_track = destination.parent != drag->source_track.parent || destination.index != drag->source_track.index;

        const Uph_TimelineBlock initial = {
            .start_beat = drag->initial_start_beat,
            .length_beats = drag->initial_length_beats,
            .start_offset_beats = drag->initial_start_offset_beats
        };

        if (moved_track || !uph_song_timeline_block_geometry_equal(block, &initial))
        {
            Uph_ActionBlockTransform data = {
                .src_track = drag->source_track, .src_index = drag->source_index,
                .dst_track = destination, .dst_index = drag->block_index,
                .old_start = initial.start_beat, .old_length = initial.length_beats, .old_offset = initial.start_offset_beats,
                .new_start = block->start_beat, .new_length = block->length_beats, .new_offset = block->start_offset_beats,
                .applied_live = true
            };
            naui_action_execute_stack(transform_action, data);
        }
    }

    naui_list_free(drag->group_blocks);
    drag->group_blocks = NULL;
    drag->active = false;
    drag->creating = false;
    drag->group = false;
    drag->mode = UPH_BLOCK_INTERACTION_NONE;
}

#pragma endregion

#pragma region Automation Point Editing

static double uph_song_timeline_automation_mouse_beat(Naui_Vec2 position, double start_offset)
{
    const double raw = ((double)naui_mouse_x() - position.x) / uph_song_timeline_data.zoom.x + start_offset;
    return uph_snap_beat_round(raw, uph_song_timeline_data.snap_resolution);
}

static void uph_song_timeline_finish_point_edit(const Uph_Automation *automation)
{
    Uph_AutomationEditState *edit = &uph_song_timeline_data.automation_edit;
    const int32_t index = edit->dragging_point_index;
    edit->dragging_point_index = -1;

    if (index < 0 || (uint32_t)index >= (uint32_t)naui_list_len(automation->points))
    {
        edit->creating = false;
        return;
    }

    const Uph_AutomationPoint point = automation->points[index];
    if (edit->creating)
    {
        Uph_ActionAutomationPointCreate data = {
            .automation_index = edit->automation_index,
            .point_index = (uint32_t)index,
            .point = point,
            .applied_live = true
        };
        naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_CREATE, data);
    }
    else if (point.beat != edit->initial_point.beat || point.value != edit->initial_point.value)
    {
        Uph_ActionAutomationPointMove data = {
            .automation_index = edit->automation_index,
            .point_index = (uint32_t)index,
            .old_point = edit->initial_point,
            .new_point = point,
            .applied_live = true
        };
        naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_MOVE, data);
    }

    edit->creating = false;
}

static void uph_song_timeline_update_automation_point_drag(
    Naui_Vec2 position,
    Naui_Vec2 size,
    Uph_Track *track,
    uint32_t block_index,
    double start_offset,
    Uph_ResourceIndex automation_index
)
{
    Uph_AutomationEditState *edit = &uph_song_timeline_data.automation_edit;
    Uph_Automation *automation = &uph_state.project.automations[automation_index];
    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;
    const int32_t point_size = NAUI_DPI(6);
    const int32_t hit_radius = point_size;

    const bool is_target_block = (edit->track == track && edit->block_index == block_index);

    if (is_target_block && edit->dragging_point_index >= 0)
    {
        const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
        const int32_t idx = edit->dragging_point_index;
        if ((uint32_t)idx >= point_count)
        {
            edit->dragging_point_index = -1;
            edit->creating = false;
            return;
        }

        Uph_AutomationPoint *point = &automation->points[idx];

        const float mouse_y = (float)naui_mouse_y();
        const float clamped_y = fminf(fmaxf(mouse_y, position.y), position.y + size.y);
        point->value = 1.0f - (double)((clamped_y - position.y) / size.y);
        point->value = fmax(0.0, fmin(1.0, point->value));

        if (idx != 0)
        {
            double snapped_beat = uph_song_timeline_automation_mouse_beat(position, start_offset);

            const double prev_beat = automation->points[idx - 1].beat;
            snapped_beat = fmax(snapped_beat, prev_beat);

            if ((uint32_t)idx + 1 < point_count)
                snapped_beat = fmin(snapped_beat, automation->points[idx + 1].beat);

            point->beat = snapped_beat;
        }

        naui_set_cursor(NAUI_CURSOR_HAND);

        if (naui_mouse_released(NAUI_MOUSE_LEFT))
            uph_song_timeline_finish_point_edit(automation);

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
                edit->automation_index = automation_index;
                edit->initial_point = *point;
                edit->creating = false;
                edit->dragging_point_index = (int32_t)i;
            }
            else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
            {
                Uph_ActionAutomationPointDelete data = { .automation_index = automation_index, .point_index = i };
                naui_action_execute_stack(UPH_ACTION_AUTOMATION_POINT_DELETE, data);
                break;
            }

            break;
        }
    }

    if (!hovering_existing_point &&
        naui_mouse_pressed(NAUI_MOUSE_LEFT) &&
        uph_song_timeline_vec4_contains_vec2(
            (Naui_Vec4) { position.x, position.y, size.x, size.y },
            (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }
        ))
    {
        double new_beat = uph_song_timeline_automation_mouse_beat(position, start_offset);
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
        edit->automation_index = automation_index;
        edit->initial_point = new_point;
        edit->creating = true;
        edit->dragging_point_index = (int32_t)insert_index;
    }
}

#pragma endregion

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
        if (i >= (uint32_t)naui_list_len(blocks))
        {
            automation_edit->dragging_point_index = -1;
            automation_edit->creating = false;
            return;
        }

        const float block_left = bbox.x + zoom_x * blocks[i].start_beat - scroll_x;

        Naui_Vec2 automation_pos = { block_left, bbox.y + title_height };
        Naui_Vec2 automation_size = { zoom_x * blocks[i].length_beats, bbox.height - title_height };

        uph_song_timeline_update_automation_point_drag(
            automation_pos,
            automation_size,
            track,
            i,
            blocks[i].start_offset_beats,
            blocks[i].resource_index
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
            	blocks[i].start_beat = fmax(0.0, uph_snap_beat_round(mouse_beat + drag->initial_drag_beat_offset, uph_song_timeline_data.snap_resolution));
                naui_set_cursor(NAUI_CURSOR_HAND);
            }
            else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_LEFT)
            {
                const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);
                double new_start = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution);
                double end_beat = drag->initial_start_beat + drag->initial_length_beats;

                const double earliest_start_beat = drag->initial_start_beat - drag->initial_start_offset_beats;
                new_start = fmax(new_start, earliest_start_beat);
                new_start = fmin(new_start, end_beat - division);
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
                const double division = uph_snap_division(uph_song_timeline_data.snap_resolution);

                double raw_length = mouse_beat - blocks[i].start_beat;
                double snapped_end = uph_snap_beat_round(blocks[i].start_beat + raw_length, uph_song_timeline_data.snap_resolution);
                double new_length = snapped_end - blocks[i].start_beat;
                new_length = fmax(division, new_length);
                blocks[i].length_beats = new_length;

                uph_state.shared.song_timeline_current_block_length = blocks[i].length_beats;
                uph_state.shared.song_timeline_current_block_start_offset = blocks[i].start_offset_beats;
                uph_state.shared.current_pattern_updated = false;

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }

            if (drag->group)
                uph_song_timeline_apply_group_drag(&blocks[i]);

            if (naui_mouse_released(NAUI_MOUSE_LEFT))
                uph_song_timeline_finish_block_drag();

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

            if (uph_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
            {
                Uph_BlockInteractionMode hover_mode = uph_song_timeline_classify_hover(hover_box, (float)naui_mouse_x());
                if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT || uph_song_timeline_data.current_action_mode == UPH_ACTION_DRAW))
                {
                    const double grab_beat = ((double)naui_mouse_x() - bbox.x + scroll_x) / zoom_x;
                    uph_song_timeline_begin_block_drag(track, i, hover_mode, grab_beat);
                }

                naui_set_cursor(hover_mode == UPH_BLOCK_INTERACTION_MOVE ? NAUI_CURSOR_HAND : NAUI_CURSOR_RESIZE_EW);
                uph_song_timeline_data.hovered_block.block_index = i;
                uph_song_timeline_data.hovered_block.track = track;
                uph_song_timeline_data.hovered_block.active = true;
            }
            else if (blocks[i].type == UPH_RESOURCE_AUTOMATION)
            {
                hover_box.y += title_height;
                hover_box.w = bbox.height - title_height;

                if (uph_song_timeline_vec4_contains_vec2(hover_box, (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }))
                {
                    Naui_Vec2 automation_pos = { block_left, bbox.y + title_height };
                    Naui_Vec2 automation_size = { zoom_x * blocks[i].length_beats, bbox.height - title_height };

                    uph_song_timeline_update_automation_point_drag(
                        automation_pos,
                        automation_size,
                        track,
                        i,
                        blocks[i].start_offset_beats,
                        blocks[i].resource_index
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

	Naui_Color color = uph_resources_track_color(track->color_index);
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
    {
        if (!uph_song_timeline_block_is_visible(blocks[i].start_beat, blocks[i].length_beats, zoom_x, scroll_x, bbox.width))
            continue;

        blocks[i].visual_lifetime = NAUI_MIN(blocks[i].visual_lifetime + naui_delta_time(), 1.0f);
        uph_song_timeline_render_timeline_block(
            (Naui_Vec2) { bbox.x + zoom_x * blocks[i].start_beat - scroll_x, bbox.y },
            (Naui_Vec2) { zoom_x * blocks[i].length_beats, bbox.height },
            color,
            opacity,
            blocks[i].selected,
            &blocks[i],
            bbox
        );
    }
}

static void uph_song_timeline_delete_hovered_block(Uph_Track *track)
{
    const uint32_t block_index = uph_song_timeline_data.hovered_block.block_index;
    if (block_index >= (uint32_t)naui_list_len(track->blocks))
        return;

    if (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT && track->blocks[block_index].selected && uph_block_selected_count() > 1)
    {
        uph_block_delete_selected();
        return;
    }

    Uph_ActionBlockDelete data = { .track = uph_action_track_ref(track), .block_index = block_index };
    naui_action_execute_stack(UPH_ACTION_BLOCK_DELETE, data);
}

static void uph_song_timeline_update_track_action_input(Leaf_BoundingBox bbox, Uph_Track *track)
{
    if (!uph_song_timeline_data.tracks_hovered)
        return;

    if (uph_song_timeline_data.automation_edit.active)
        return;

    if (naui_mouse_pressed(NAUI_MOUSE_RIGHT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track == track)
        uph_song_timeline_delete_hovered_block(track);

    const bool mouse_over_track = uph_song_timeline_vec4_contains_vec2(
        (Naui_Vec4) { bbox.x, bbox.y, bbox.width, bbox.height },
        (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }
    );

    if (uph_song_timeline_data.current_action_mode == UPH_ACTION_SELECT)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && mouse_over_track && !uph_song_timeline_data.hovered_block.active && !uph_song_timeline_data.drag.active)
        {
            Uph_MarqueeState *marquee = &uph_song_timeline_data.marquee;
            marquee->active = true;
            marquee->start_beat = ((double)naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
            marquee->start_content_y = (float)naui_mouse_y() - uph_song_timeline_data.panel_bounding_box.y + uph_song_timeline_data.scroll.y;
            uph_block_select_all(false);
        }
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
            (uph_state.shared.selected_resource.type == UPH_RESOURCE_AUTOMATION ? track->type == UPH_RESOURCE_AUTOMATION : (track->type == UPH_RESOURCE_NONE || track->type == uph_state.shared.selected_resource.type)))
        {
            if (mouse_over_track)
            {
                const float beat = (naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
                naui_list_free(uph_song_timeline_data.drag.group_blocks);
                uph_song_timeline_data.drag.group_blocks = NULL;
                uph_song_timeline_data.drag.active = true;
                uph_song_timeline_data.drag.creating = true;
                uph_song_timeline_data.drag.group = false;
                uph_song_timeline_data.drag.additive = false;
                uph_song_timeline_data.drag.block_index = naui_list_len(track->blocks);
                uph_song_timeline_data.drag.track = track;
                uph_song_timeline_data.drag.mode = UPH_BLOCK_INTERACTION_MOVE;
                uph_song_timeline_data.drag.initial_drag_beat_offset = 0.0;
                naui_list_push(track->blocks, uph_song_timeline_init_block(uph_snap_beat_floor(beat, uph_song_timeline_data.snap_resolution), uph_state.shared.selected_resource.index, uph_state.shared.selected_resource.type));
                if (track->type == UPH_RESOURCE_NONE)
                    track->type = uph_state.shared.selected_resource.type;
            }
        }
    }
    else if (uph_song_timeline_data.current_action_mode == UPH_ACTION_CUT)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph_song_timeline_data.hovered_block.active && uph_song_timeline_data.hovered_block.track == track)
        {
            const uint32_t block_index = uph_song_timeline_data.hovered_block.block_index;
            const double mouse_beat = ((double)naui_mouse_x() - bbox.x + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
            const double cut_beat = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution);
            uph_block_cut(uph_action_track_ref(track), block_index, cut_beat, uph_snap_division(uph_song_timeline_data.snap_resolution));
        }
    }
}

static void uph_song_timeline_update_marquee(Leaf_BoundingBox bbox, Uph_Track *track)
{
    const Uph_MarqueeState *marquee = &uph_song_timeline_data.marquee;
    if (!marquee->active)
        return;

    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;

    const float corner_x = bbox.x + (float)(marquee->start_beat * zoom_x) - scroll_x;
    const float corner_y = uph_song_timeline_data.panel_bounding_box.y - uph_song_timeline_data.scroll.y + marquee->start_content_y;

    const float left = fminf(corner_x, (float)naui_mouse_x());
    const float right = fmaxf(corner_x, (float)naui_mouse_x());
    const float top = fminf(corner_y, (float)naui_mouse_y());
    const float bottom = fmaxf(corner_y, (float)naui_mouse_y());

    const bool row_touched = bbox.y < bottom && bbox.y + bbox.height > top;

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->blocks); i++)
    {
        Uph_TimelineBlock *block = &track->blocks[i];
        const float block_left = bbox.x + (float)(block->start_beat * zoom_x) - scroll_x;
        const float block_right = block_left + (float)(block->length_beats * zoom_x);

        block->selected = row_touched && block_left < right && block_right > left;
    }
}

static void uph_song_timeline_render_track_timeline_overlay(Leaf_BoundingBox bbox, Uph_Track **track_ptr)
{
    Uph_Track *track = *track_ptr;

    const bool mouse_over_track = uph_song_timeline_vec4_contains_vec2(
        (Naui_Vec4) { bbox.x, bbox.y, bbox.width, bbox.height },
        (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() }
    );

    if (mouse_over_track)
        uph_song_timeline_data.current_hovered_track = track;

    naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);

    uph_song_timeline_render_ruler(bbox, uph_song_timeline_data.zoom.x, uph_song_timeline_data.scroll.x);

    uph_song_timeline_update_track_timeline_drag(bbox, track);
    uph_song_timeline_update_track_action_input(bbox, track);
    uph_song_timeline_update_marquee(bbox, track);
    uph_song_timeline_render_track_timeline_blocks(bbox, track);

    naui_pop_clip_rect();
}

static void uph_song_timeline_render_track_timeline(Uph_Track *track)
{
    const uint32_t row_counter = uph_song_timeline_data.visual_row_counter;

    const Leaf_Color bg_color = row_counter & 1 ? naui_theme_color("uph_timeline_row_bg1_color") : naui_theme_color("uph_timeline_row_bg2_color");
    const Leaf_Color border_color = naui_theme_color("uph_song_timeline_border_color");

    const float row_height = NAUI_DPI(uph_song_timeline_data.zoom.y);
    const float row_y = uph_song_timeline_data.panel_bounding_box.y - uph_song_timeline_data.scroll.y + (float)row_counter * row_height;

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

static void uph_song_timeline_toggle_track_state(Uph_Track *track, Uph_TrackState flag, const char *action)
{
    Uph_ActionTrackState data = {
        .track = uph_action_track_ref(track),
        .old_state = track->state,
        .new_state = track->state ^ flag
    };
    naui_action_execute_stack(action, data);
}

static void uph_song_timeline_finish_rename(Uph_Track *track)
{
    Uph_SongTimelineData *timeline = &uph_song_timeline_data;
    if (!track->name.length)
        track->name = naui_string_from_cstr(NAUI_TR("song_timeline.track.title"));

    if (!naui_strings_equal(track->name, timeline->rename_old_name, true))
    {
        Uph_ActionTrackRename data = {
            .track = uph_action_track_ref(track),
            .old_name = timeline->rename_old_name,
            .new_name = track->name
        };
        naui_action_execute_stack(UPH_ACTION_TRACK_RENAME, data);
    }

    timeline->disable_space_to_play = false;
    timeline->rename_track = NULL;
}

static void uph_song_timeline_render_track_header(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu)
{
    const int32_t depth_offset = depth * 13;
    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
    const Leaf_Color bg_color = naui_theme_color("uph_song_timeline_header_color");
    const Leaf_Color border_color = naui_theme_color("uph_song_timeline_header_border_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_song_timeline_header_padding");
    const float header_width = naui_theme_float("uph_song_timeline_header_width");

    uint64_t track_id = (uint64_t)track;
	Naui_Color color = uph_resources_track_color(track->color_index);

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
                .color = color,
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
                            const float icon_size = NAUI_DPI(naui_theme_float("uph_song_timeline_icon_size"));
                            leaf({
                                .size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
                                .image = icon,
                                .color = {text_color}
                            });
                        }

                        Leaf_ID name_id = leaf_id_indexed("uph_song_timeline_name", track_id);

                        if (uph_song_timeline_data.rename_track == track)
                        {
                            if (uph_ui_textfield(&track->name, name_id, UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, NAUI_TR("song_timeline.track.title")))
                                uph_song_timeline_finish_rename(track);
                        }
                        else
                        {
                            if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph_ui_widget_hovered(name_id))
                            {
                                uph_song_timeline_data.disable_space_to_play = true;
                                uph_song_timeline_data.rename_track = track;
                                uph_song_timeline_data.rename_old_name = track->name;
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
                        leaf_id_indexed("uph_song_timeline_options", track_id),
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
                    if (uph_ui_text_toggle_button("M", leaf_id_indexed("uph_song_timeline_mute_toggle", track_id), track->state & UPH_TRACK_MUTED))
                        uph_song_timeline_toggle_track_state(track, UPH_TRACK_MUTED, UPH_ACTION_TRACK_MUTE);
                    if (uph_ui_text_toggle_button("S", leaf_id_indexed("uph_song_timeline_solo_toggle", track_id), track->state & UPH_TRACK_SOLOED))
                    {
                        Uph_ActionTrackSolo data = { .track = uph_action_track_ref(track) };
                        naui_action_execute_stack(UPH_ACTION_TRACK_SOLO, data);
                    }

                    if (track->type != UPH_RESOURCE_AUTOMATION && uph_ui_image_toggle_button(
                        naui_asset_image("uph_icon_mic"),
                        leaf_id_indexed("uph_song_timeline_arm_toggle", track_id),
                        (Naui_Vec2) { button_size, button_size },
                        text_color,
                        track->state & UPH_TRACK_ARMED
                    )) uph_song_timeline_toggle_track_state(track, UPH_TRACK_ARMED, UPH_ACTION_TRACK_ARM);
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
    Uph_SongTimelineData *data = &uph_song_timeline_data;

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(40))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .color = {naui_theme_color("uph_toolbox_bg_color")},
        .child_gap = NAUI_DPI(16),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        const int32_t button_size = NAUI_DPI(naui_theme_float("uph_song_timeline_icon_size"));
        const Naui_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
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
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
        })
        {
            leaf_text("Snap: ", {
                .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
                .color = naui_theme_color("uph_ui_text_color")
            });
            static const char *snap_options[] = {
                "Beat",
                "1/2",
                "1/4",
                "1/8",
                "1/16"
            };

            leaf({
                .size = {LEAF_SIZE_FIXED(NAUI_DPI(60)), LEAF_SIZE_FULL},
                .child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
            }) uph_ui_dropdown(snap_options, 5, (uint32_t*)&data->snap_resolution, leaf_id("uph_song_timeline_snap"));
        }

        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            leaf_text("BPM: ", {.font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")), .color = naui_theme_color("uph_ui_text_color")});
            leaf({
                .size = { LEAF_SIZE_FIXED(NAUI_DPI(56)), LEAF_SIZE_FULL },
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                const Naui_String bpm = naui_string_format("BPM: %.1f", uph_state.project.bpm);
                uph_ui_drag_float(&uph_state.project.bpm, leaf_id("uph_bpm_drag"), 1.0f, 1.0f, 10000.0f, "%.2f", UPH_UI_DRAG_CLAMPED);
            }
        }
    }
}

static void uph_song_timeline_render_top_bar(void)
{
    const float header_width = naui_theme_float("uph_song_timeline_header_width");
    const Naui_Vec2 header_padding = naui_theme_vec2("uph_song_timeline_header_padding");

    const uint32_t height = NAUI_DPI(24);
    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(height)}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width)), LEAF_SIZE_FULL},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(header_padding.x), 0.0f),
            .child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
        });

        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL}
        })
        {
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(height - 2)},
                .custom_draw = (Leaf_CustomDrawFn)uph_song_timeline_render_top_ruler,
                .border = {
                    .width = 1,
                    .sides = LEAF_SIDE_ALL,
                    .color = naui_theme_color("uph_timeline_top_ruler_border_color")
                },
                .color = naui_theme_color("uph_timeline_top_ruler_bg_color")
            });
        }
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
    const float x_offset = NAUI_DPI(naui_theme_float("uph_song_timeline_header_width") + naui_theme_vec2("uph_song_timeline_header_padding").x * 2.0f);
    
    const Naui_Image *playhead_image = naui_asset_image("uph_icon_playhead");
    const Naui_Color color = naui_theme_color("uph_playhead_color");

    const float playhead_size = NAUI_DPI(16.0f);
    const float playhead_half_size = playhead_size * 0.5f;

    naui_push_clip_rect(bbox.x + x_offset - NAUI_DPI(1), bbox.y, bbox.width, bbox.height);
    if (uph_song_timeline_data.current_action_mode == UPH_ACTION_CUT)
    {
        const double mouse_beat = ((double)naui_mouse_x() - (bbox.x + x_offset) + uph_song_timeline_data.scroll.x) / uph_song_timeline_data.zoom.x;
        const double cut_beat = uph_snap_beat_round(mouse_beat, uph_song_timeline_data.snap_resolution);

        const float x = bbox.x + x_offset + (float)(cut_beat * uph_song_timeline_data.zoom.x) - uph_song_timeline_data.scroll.x;

        naui_draw_line(
            (Naui_Vec2) { x, bbox.y + playhead_half_size },
            (Naui_Vec2) { x, bbox.y + bbox.height },
            color,
            NAUI_DPI(1.0f)
        );
    }

    {
        const float x = bbox.x + x_offset + uph_state.shared.song_timeline_playhead_position * uph_song_timeline_data.zoom.x - uph_song_timeline_data.scroll.x;
        naui_draw_line(
            (Naui_Vec2) { x, bbox.y + playhead_half_size },
            (Naui_Vec2) { x, bbox.y + bbox.height },
            color,
            NAUI_DPI(1.0f)
        );

        naui_draw_image(playhead_image, (Naui_Vec2){x - playhead_half_size, bbox.y + playhead_half_size}, (Naui_Vec2){playhead_size, playhead_size}, color, 0.0f, NAUI_CORNER_NONE);
    }

    const Uph_MarqueeState *marquee = &uph_song_timeline_data.marquee;
    if (marquee->active)
    {
        const float corner_x = bbox.x + x_offset + (float)(marquee->start_beat * uph_song_timeline_data.zoom.x) - uph_song_timeline_data.scroll.x;
        const float corner_y = uph_song_timeline_data.panel_bounding_box.y - uph_song_timeline_data.scroll.y + marquee->start_content_y;

        const Naui_Vec2 top_left = { fminf(corner_x, (float)naui_mouse_x()), fminf(corner_y, (float)naui_mouse_y()) };
        const Naui_Vec2 size = { fabsf(corner_x - (float)naui_mouse_x()), fabsf(corner_y - (float)naui_mouse_y()) };

        naui_fill_rect(top_left, size, leaf_rgba(255, 255, 255, 30), 0.0f, NAUI_CORNER_NONE);
        naui_draw_rect(top_left, size, leaf_rgba(255, 255, 255, 160), NAUI_DPI(1.0f), 0.0f, NAUI_CORNER_NONE, NAUI_SIDE_ALL);
    }

    naui_pop_clip_rect();
}

static void uph_song_timeline_render_track_options_menu(Uph_SongTimelineData *data, Uph_UIMenuID track_options_context_menu)
{
    if (naui_list_len(uph_state.project.tracks) == 0 || !data->current_options_track)
        return;

    Uph_Track *track = data->current_options_track;

    {
        Uph_UIMenuID color_menu = uph_ui_submenu(track_options_context_menu, "Color", leaf_id("uph_song_timeline_options_color"));
		const uint32_t color_count = (uint32_t)naui_list_len(naui_theme_color_list("uph_track_palette"));
        for (uint32_t i = 0; i < color_count; i++)
		{
			if (uph_ui_menu_item(color_menu, naui_string_format("Color %i", i).data, leaf_id_indexed("uph_song_timeline_options_color_item", i)) && track->color_index != (int32_t)i)
            {
                Uph_ActionTrackColor color_data = {
                    .track = uph_action_track_ref(track),
                    .old_color = track->color_index,
                    .new_color = (int32_t)i
                };
                naui_action_execute_stack(UPH_ACTION_TRACK_COLOR, color_data);
            }
		}
       
    }

    if (track->type == UPH_RESOURCE_NONE || track->type == UPH_RESOURCE_PATTERN)
    {
        if (track->instrument.loaded)
        {
            const bool visible = uph_plugin_window_visible(&track->instrument);
            if (uph_ui_menu_item(track_options_context_menu, visible ? "Hide Instrument" : "Show Instrument", leaf_id("uph_song_timeline_options_show_instrument"))) 
            {
                if (visible)
                    uph_hide_plugin_window(&track->instrument);
                else uph_show_plugin_window(&track->instrument);
            }

            if (uph_ui_menu_item(track_options_context_menu, "Remove Instrument", leaf_id("uph_song_timeline_options_remove_instrument"))) 
            {
                uph_unload_plugin(&track->instrument);
                if (naui_list_len(track->blocks) == 0)
                    track->type = UPH_RESOURCE_NONE;
            }
        }
        else
        {
            if (uph_ui_menu_item(track_options_context_menu, "Load Instrument", leaf_id("uph_song_timeline_options_load_instrument")))
            {
                uph_state.shared.plugin_list_for_track_instrument = true;
                uph_state.shared.current_plugin_list_track = track;
                naui_open_panel(uph_state.panels.plugin_list);
            }
        }

        if (track->instrument.params)
        {
            Uph_UIMenuID automate_menu = uph_ui_submenu(track_options_context_menu, "Automate", leaf_id("uph_song_timeline_options_automate"));

            // TODO: make this a separate menu with filtering and stuff
            for (uint32_t i = 0; i < 100u && i < (uint32_t)naui_list_len(track->instrument.params); i++)
            {
                if (uph_ui_menu_item(automate_menu, track->instrument.params[i].name.data, leaf_id_indexed("uph_song_timeline_options_automate_param", i)))
                {
                    Uph_ActionTrackAutomationCreate lane = {
                        .parent = uph_action_track_ref(track),
                        .name = track->instrument.params[i].name,
                        .effect_index = -1,
                        .param_id = track->instrument.params[i].id
                    };
                    naui_action_execute_stack(UPH_ACTION_TRACK_AUTOMATION_CREATE, lane);
                }
            }
        }
    }

    if (uph_ui_menu_item(track_options_context_menu, "Remove", leaf_id("uph_song_timeline_options_remove"))) 
    {
        Uph_ActionTrackDelete delete_data = { .track = uph_action_track_ref(track) };
        naui_action_execute_stack(UPH_ACTION_TRACK_DELETE, delete_data);
    }
}

static void uph_song_timeline_render_track_plus(void)
{
    const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
    const Leaf_Color bg_color = naui_theme_color("uph_song_timeline_header_color");
    const Leaf_Color border_color = naui_theme_color("uph_song_timeline_header_border_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_song_timeline_header_padding");
    const float header_width = NAUI_DPI(naui_theme_float("uph_song_timeline_header_width"));

    Leaf_ID id = leaf_id("uph_song_timeline_plus");
    if (uph_ui_widget_hovered(id))
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
            naui_action_execute_stack(UPH_ACTION_TRACK_CREATE, (Uph_ActionTrackCreate){0});
        naui_set_cursor(NAUI_CURSOR_HAND);
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(header_width), LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(6), LEAF_CORNER_BL | LEAF_CORNER_BR),
        .border = {
            .width = 1,
            .sides = LEAF_SIDE_ALL,
            .color = border_color
        },
        .color = bg_color
    })
    {
        Naui_Image *icon = naui_asset_image("uph_icon_plus");

        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(13)), LEAF_SIZE_DERIVED},
            .color = text_color,
            .image = icon,
            .aspect_ratio = 1.0f
        });
    }
}

static void uph_song_timeline_update_selection_keys(void)
{
    Uph_SongTimelineData *data = &uph_song_timeline_data;

    if (!data->panel_hovered || data->disable_space_to_play || data->drag.active || data->marquee.active || data->automation_edit.dragging_point_index >= 0)
        return;

    const bool ctrl = uph_ui_ctrl_down();

    if (naui_key_pressed(NAUI_KEY_DELETE))
        uph_block_delete_selected();
    else if (ctrl && naui_key_pressed(NAUI_KEY_A))
        uph_block_select_all(true);
    else if (ctrl && naui_key_pressed(NAUI_KEY_D))
        uph_block_duplicate_selected();
    else if (naui_key_pressed(NAUI_KEY_ESCAPE))
        uph_block_select_all(false);
}

static void uph_song_timeline_on_update(void)
{
    Uph_SongTimelineData *data = &uph_song_timeline_data;
    const Leaf_ID track_section_id = leaf_id("uph_song_timeline_section");

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

    if (data->marquee.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
        data->marquee.active = false;

    if (naui_key_pressed(NAUI_KEY_SPACE) && !data->disable_space_to_play && data->panel_hovered)
        uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
    
    uph_song_timeline_update_input();
    uph_song_timeline_update_selection_keys();
    uph_song_timeline_update_drag_track_switch();
    uph_song_timeline_render_toolbox();

    Uph_UIMenuID track_options_context_menu = uph_ui_context_menu();

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
    })
    {
        uph_song_timeline_render_top_bar();
        leaf({
            .id = track_section_id,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .child_offset = {0.0f, data->scroll.y},
            .clip_children = true
        })
        {
            for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
            {
                uph_song_timeline_render_track(&uph_state.project.tracks[i], 0, track_options_context_menu);
                data->visual_row_counter++;
            }

            uph_song_timeline_render_track_plus();
        }
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .custom_draw = uph_song_timeline_render_playhead_overlay
        });
    }

    uph_song_timeline_render_track_options_menu(data, track_options_context_menu);
}
