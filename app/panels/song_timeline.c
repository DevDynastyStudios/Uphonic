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
    uint32_t block_index;
    uint32_t track_index;
    bool active;
    Uph_BlockInteractionMode mode;
}
Uph_DraggingBlockState;

typedef struct
{
    Naui_Vec2 scroll;
    Naui_Vec2 zoom;
    Uph_DraggingBlockState drag;
}
Uph_SongTimelineData;

static Uph_SongTimelineData uph_song_timeline_data;

NAUI_PANEL(uph_song_timeline)

static void uph_song_timeline_on_attach(void)
{
    Naui_PanelID panel = naui_current_panel();

    naui_panel_set_title(panel, "Song Timeline");

    for (uint32_t i = 0; i < 10; i++)
    {
        Uph_Track track = {
            .name = naui_string_from_cstr("Test"),
            .color = LEAF_COLOR_WHITE
        };
        naui_list_push(uph_state.project.tracks, track);
    }

    Uph_TimelineBlock block = {
        .start_beat = 3.0f,
        .length_beats = 5.0f
    };
    naui_list_push(uph_state.project.tracks[0].blocks, block);

    uph_song_timeline_data.scroll = naui_vec2(0.0f, 0.0f);
    uph_song_timeline_data.zoom = naui_vec2(32.0f, 90.0f);
}

static void uph_song_timeline_on_detach(void)
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
            naui_vec2(x, bbox.y),
            naui_vec2(x, bbox.y + bbox.height),
            line_color,
            1.0f
        );
    }
}

static void uph_song_timeline_render_top_ruler(Leaf_BoundingBox bbox)
{
    const Leaf_Color beat_color = naui_theme_color("uph_track_grid_beat_color");
    const Leaf_Color bar_color = naui_theme_color("uph_track_grid_bar_color");
    const Leaf_Color number_color = naui_theme_color("uph_track_grid_text_color");

    const int32_t first_line = (int32_t)(uph_song_timeline_data.scroll.x / uph_song_timeline_data.zoom.x);
    const uint32_t line_count = (uint32_t)(bbox.width / uph_song_timeline_data.zoom.x) + 2;

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
            naui_vec2(x, bbox.y + bbox.height * (is_downbeat ? 0.4f : 0.6f)),
            naui_vec2(x, bbox.y + bbox.height),
            line_color,
            1.0f
        );

        if (is_downbeat)
        {
            char label[16];
            snprintf(label, sizeof(label), "%d", line_index / 4);
            naui_draw_text(naui_vec2(x + 4.0f, bbox.y + bbox.height * 0.4f), label, NAUI_DPI(13.0f), 0, number_color);
        }
    }
    naui_pop_clip_rect();
}

static void uph_song_timeline_render_timeline_block(Naui_Vec2 position, Naui_Vec2 size, const Uph_TimelineBlock *block)
{
    const float title_padding = NAUI_DPI(2.0f);
    const float font_size = NAUI_DPI(13.0f);
    const float title_height = font_size + title_padding * 2.0f;
    const float rounding = NAUI_DPI(8.0f);

    naui_push_clip_rect(position.x, position.y, size.x, size.y);
    naui_fill_rect(position, naui_vec2(size.x, title_height), leaf_rgba(0, 0, 0, 200), rounding, NAUI_CORNER_TL | NAUI_CORNER_TR);
    naui_fill_rect(naui_vec2(position.x, position.y + title_height), naui_vec2(size.x, size.y - title_height), leaf_rgba(0, 0, 0, 128), rounding, NAUI_CORNER_BL | NAUI_CORNER_BR);
    naui_draw_text(naui_vec2(position.x + title_padding, position.y + title_padding), "Test Sample", font_size, 0, LEAF_COLOR_WHITE);
    naui_pop_clip_rect();
}

static inline bool upb_song_timeline_vec4_contains_vec2(const Naui_Vec4 rect, const Naui_Vec2 point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z &&
           point.y >= rect.y && point.y <= rect.y + rect.w;
}

static void uph_song_timeline_update_drag_track_switch(float tracks_top_y)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    if (!drag->active)
        return;

    if (drag->mode != UPH_BLOCK_INTERACTION_MOVE)
        return;

    float mouse_y = (float)naui_mouse_y() + uph_song_timeline_data.scroll.y - tracks_top_y;
    int32_t hovered_track = (int32_t)(mouse_y / uph_song_timeline_data.zoom.y);

    uint32_t track_count = (uint32_t)naui_list_len(uph_state.project.tracks);
    if (hovered_track < 0 || (uint32_t)hovered_track >= track_count)
        return;

    if ((uint32_t)hovered_track == drag->track_index)
        return;

    Uph_Track *old_track = &uph_state.project.tracks[drag->track_index];
    Uph_Track *new_track = &uph_state.project.tracks[(uint32_t)hovered_track];

    Uph_TimelineBlock moved = old_track->blocks[drag->block_index];
    naui_list_uremove(old_track->blocks, drag->block_index);
    naui_list_push(new_track->blocks, moved);

    drag->track_index = (uint32_t)hovered_track;
    drag->block_index = (uint32_t)naui_list_len(new_track->blocks) - 1;
}

#define UPH_RESIZE_HANDLE_WIDTH 6.0f

static Uph_BlockInteractionMode uph_song_timeline_classify_hover(Naui_Vec4 hover_box, float mouse_x)
{
    if (mouse_x <= hover_box.x + UPH_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_LEFT;
    if (mouse_x >= hover_box.x + hover_box.z - UPH_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_RIGHT;
    return UPH_BLOCK_INTERACTION_MOVE;
}

static void uph_song_timeline_update_track_timeline_drag(Leaf_BoundingBox bbox, uint32_t track_index)
{
    Uph_DraggingBlockState *drag = &uph_song_timeline_data.drag;
    Naui_List(Uph_TimelineBlock) blocks = uph_state.project.tracks[track_index].blocks;
    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
    {
        if ((blocks[i].start_beat + blocks[i].length_beats) * zoom_x < scroll_x)
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

                new_start = fmin(new_start, end_beat - 1.0);

                blocks[i].start_beat = new_start;
                blocks[i].length_beats = end_beat - new_start;

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }
            else if (drag->mode == UPH_BLOCK_INTERACTION_RESIZE_RIGHT)
            {
                double new_length = ceil(mouse_beat) - blocks[i].start_beat;
                blocks[i].length_beats = fmax(1.0, new_length);

                naui_set_cursor(NAUI_CURSOR_RESIZE_EW);
            }

            if (naui_mouse_released(NAUI_MOUSE_LEFT))
            {
                drag->active = false;
                drag->mode = UPH_BLOCK_INTERACTION_NONE;
            }

            return;
        }

        if (!drag->active)
        {
            Naui_Vec4 hover_box = naui_vec4(
                fmaxf(bbox.x, bbox.x + zoom_x * blocks[i].start_beat - scroll_x),
                bbox.y,
                zoom_x * blocks[i].length_beats,
                bbox.height
            );

            if (upb_song_timeline_vec4_contains_vec2(hover_box, naui_vec2((float)naui_mouse_x(), (float)naui_mouse_y())))
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

                    double mouse_beat = ((double)naui_mouse_x() - bbox.x + scroll_x) / zoom_x;
                    drag->initial_drag_beat_offset = blocks[i].start_beat - mouse_beat;
                }

                naui_set_cursor(
                    hover_mode == UPH_BLOCK_INTERACTION_MOVE
                        ? NAUI_CURSOR_HAND
                        : NAUI_CURSOR_RESIZE_EW
                );
            }
        }
    }
}

static void uph_song_timeline_render_track_timeline_blocks(Leaf_BoundingBox bbox, uint32_t track_index)
{
    Naui_List(Uph_TimelineBlock) blocks = uph_state.project.tracks[track_index].blocks;
    const float zoom_x = uph_song_timeline_data.zoom.x;
    const float scroll_x = uph_song_timeline_data.scroll.x;

    for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
    {
        if ((blocks[i].start_beat + blocks[i].length_beats) * zoom_x < scroll_x)
            continue;

        uph_song_timeline_render_timeline_block(
            naui_vec2(bbox.x + zoom_x * blocks[i].start_beat - scroll_x, bbox.y),
            naui_vec2(zoom_x * blocks[i].length_beats, bbox.height),
            &blocks[i]
        );
    }
}

static void uph_song_timeline_render_track_timeline_overlay(Leaf_BoundingBox bbox, uint32_t *track_index_ptr)
{
    const uint32_t track_index = *track_index_ptr;

    naui_push_clip_rect(bbox.x, bbox.y, bbox.width, bbox.height);

    uph_song_timeline_render_ruler(bbox, uph_song_timeline_data.zoom.x, uph_song_timeline_data.scroll.x);
    uph_song_timeline_update_track_timeline_drag(bbox, track_index);
    uph_song_timeline_render_track_timeline_blocks(bbox, track_index);

    naui_pop_clip_rect();
}

static void uph_song_timeline_render_track_timeline(uint32_t track_index)
{
    const Leaf_Color bg_color = track_index & 1 ? naui_theme_color("uph_track_bg1_color") : naui_theme_color("uph_track_bg2_color");
    const Leaf_Color border_color = naui_theme_color("uph_track_border_color");

    leaf({
        .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
        .color = bg_color,
        .border = {
            .width = 1.0f,
            .color = border_color,
            .sides = LEAF_SIDE_TOP | LEAF_SIDE_BOTTOM
        },
        .custom_draw_data = LEAF_DATA_SLICE(track_index),
        .custom_draw = (Leaf_CustomDrawFn)uph_song_timeline_render_track_timeline_overlay
    });
}

static void uph_song_timeline_render_track_header(const Uph_Track *track)
{
    const Leaf_Color bg_color = naui_theme_color("uph_track_header_color");
    const Leaf_Color text_color = naui_theme_color("uph_track_text_color");
    const Leaf_Color border_color = naui_theme_color("uph_track_header_border_color");

    const Naui_Vec2 padding = naui_theme_vec2("uph_track_header_padding");
    const float header_width = naui_theme_float("uph_track_header_width");

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
        leaf_text(track->name.data, {
            .font_size = NAUI_DPI(14.0f),
            .color = text_color
        });
    }
}

static void uph_song_timeline_render_track(uint32_t track_index)
{
    const Uph_Track *track = &uph_state.project.tracks[track_index];

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_data.zoom.y))}
    })
    {
        uph_song_timeline_render_track_header(track);
        uph_song_timeline_render_track_timeline(track_index);
    }
}

static void uph_song_timeline_render_top_bar(void)
{
    const float header_width = naui_theme_float("uph_track_header_width");
    const Naui_Vec2 header_padding = naui_theme_vec2("uph_track_header_padding");

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32.0f))}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width + header_padding.x * 2.0f)), LEAF_SIZE_FULL}
        });
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .custom_draw = (Leaf_CustomDrawFn)uph_song_timeline_render_top_ruler
        });
    }
}

#define UPH_ZOOM_X_MIN 8.0f
#define UPH_ZOOM_X_MAX 256.0f
#define UPH_PAN_SPEED 1.0f
#define UPH_SCROLL_Y_SPEED 40.0f
#define UPH_ZOOM_SPEED 0.1f

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

    if (ctrl_held && wheel_y != 0.0f)
    {
        const float old_zoom_x = uph_song_timeline_data.zoom.x;

        const double mouse_beat_before =
            ((double)naui_mouse_x() + uph_song_timeline_data.scroll.x) / old_zoom_x;

        float new_zoom_x = old_zoom_x * (1.0f + wheel_y * UPH_ZOOM_SPEED);
        new_zoom_x = naui_clamp(new_zoom_x, UPH_ZOOM_X_MIN, UPH_ZOOM_X_MAX);

        uph_song_timeline_data.zoom.x = new_zoom_x;

        uph_song_timeline_data.scroll.x =
            (float)(mouse_beat_before * new_zoom_x) - (float)naui_mouse_x();
        uph_song_timeline_data.scroll.x = fmaxf(0.0f, uph_song_timeline_data.scroll.x);
    }
    else if (wheel_y != 0.0f)
    {
        uph_song_timeline_data.scroll.y -= wheel_y * UPH_SCROLL_Y_SPEED;
        uph_song_timeline_data.scroll.y = naui_clamp(uph_song_timeline_data.scroll.y, 0.0f, max_scroll_y);
    }

    static Naui_Vec2 pan_last_mouse;
    static bool panning = false;

    if (naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
    {
        panning = true;
        pan_last_mouse = naui_vec2((float)naui_mouse_x(), (float)naui_mouse_y());
    }

    if (panning)
    {
        Naui_Vec2 current = naui_vec2((float)naui_mouse_x(), (float)naui_mouse_y());
        Naui_Vec2 delta = naui_vec2(current.x - pan_last_mouse.x, current.y - pan_last_mouse.y);

        uph_song_timeline_data.scroll.x -= delta.x * UPH_PAN_SPEED;
        uph_song_timeline_data.scroll.x = fmaxf(0.0f, uph_song_timeline_data.scroll.x);

        uph_song_timeline_data.scroll.y -= delta.y * UPH_PAN_SPEED;
        uph_song_timeline_data.scroll.y = naui_clamp(uph_song_timeline_data.scroll.y, 0.0f, max_scroll_y);

        pan_last_mouse = current;

        if (naui_mouse_released(NAUI_MOUSE_MIDDLE))
            panning = false;
    }
}

static void uph_song_timeline_on_update(void)
{
    const Leaf_ID track_section_id = leaf_id("uph_track_section");
    const Leaf_BoundingBox track_section_bbox = leaf_get_bounding_box(track_section_id);

    uph_song_timeline_update_input();
    uph_song_timeline_update_drag_track_switch(track_section_bbox.y);

    uph_song_timeline_render_top_bar();
    leaf({
        .id = track_section_id,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .child_offset = {0.0f, uph_song_timeline_data.scroll.y},
        .clip_children = true
    })
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.project.tracks); i++)
            uph_song_timeline_render_track(i);
    }
}