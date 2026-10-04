static int32_t _uph_ui_thumb_length(float max_size, float content_size)
{
    int32_t length = (int32_t)(max_size * (max_size / content_size));
    const int32_t min_thumb = NAUI_DPI(16);
    return length < min_thumb ? min_thumb : length;
}

Uph_UIScrollContainer uph_ui_begin_scroll_container(Uph_UIScrollDirection direction, float *out_scroll, Leaf_ID scroll_id)
{
    static bool drag_active = false;
    static Leaf_ID drag_id;
    static int32_t drag_start_mouse;
    static float drag_start_scroll;

    Uph_UIScrollContainer container;
    container.direction = direction;
    container.out_scroll = out_scroll;
    container.handle_id = scroll_id;

    leaf_begin_element((Leaf_ElementConfig){
        .direction = direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            LEAF_DIRECTION_HORIZONTAL : LEAF_DIRECTION_VERTICAL,
        .size = {LEAF_SIZE_GROW, LEAF_SIZE_GROW},
        .clip_children = true
    });

    Leaf_ID current_area_id = {.value = scroll_id.value + 1};
    Leaf_ID max_area_id = {.value = scroll_id.value + 2};

    Leaf_BoundingBox current_area_box = leaf_get_bounding_box(current_area_id);
    Leaf_BoundingBox max_area_box = leaf_get_bounding_box(max_area_id);

    container.container_current_size = direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
        current_area_box.height : current_area_box.width;

    container.container_max_size = direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
        max_area_box.height : max_area_box.width;

    const bool vertical = direction == UPH_UI_SCROLL_DIRECTION_VERTICAL;
    const float max_size = (float)container.container_max_size;
    const float content_size = (float)container.container_current_size;
    const float max_scroll = content_size - max_size;
    const bool this_drag = drag_active && drag_id.value == scroll_id.value;

    extern Uph_GlobalWidgetData uph_global_widget_data;

    if (max_scroll > 0.0f)
    {
        const float thumb_travel = max_size - (float)_uph_ui_thumb_length(max_size, content_size);
        const int32_t mouse_pos = vertical ? naui_mouse_y() : naui_mouse_x();

        if (this_drag)
        {
            if (naui_mouse_down(NAUI_MOUSE_LEFT))
            {
                if (thumb_travel > 0.0f)
                {
                    *out_scroll = drag_start_scroll +
                        (float)(mouse_pos - drag_start_mouse) * (max_scroll / thumb_travel);
                }
            }
            else
            {
                drag_active = false;
            }
            naui_set_cursor(NAUI_CURSOR_HAND);
        }
        else if (naui_panel_hovered(naui_current_panel()))
        {
            if (leaf_hovered(max_area_id) || leaf_hovered(scroll_id))
                *out_scroll -= (float)naui_mouse_scroll_delta() * 20.0f;

            if (leaf_hovered(scroll_id))
            {
                uph_global_widget_data.any_widget_hovered = true;
                naui_set_cursor(NAUI_CURSOR_HAND);
                if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
                {
                    drag_active = true;
                    drag_id = scroll_id;
                    drag_start_mouse = mouse_pos;
                    drag_start_scroll = *out_scroll;
                }
            }
        }
    }
    else if (this_drag)
    {
        drag_active = false;
    }

    leaf_begin_element((Leaf_ElementConfig){
        .id = max_area_id,
        .direction = (Leaf_LayoutDirection)direction,
        .size = (direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_FULL} :
            (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_GROW}
        )
    });

    *out_scroll = NAUI_CLAMP(*out_scroll, 0.0f, NAUI_MAX(0.0f, max_scroll));

    leaf_begin_element((Leaf_ElementConfig){
        .id = current_area_id,
        .child_offset = (direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            (Leaf_Vec2){0.0f, *out_scroll} :
            (Leaf_Vec2){*out_scroll, 0.0f}
        ),
        .direction = (Leaf_LayoutDirection)direction,
        .size = (direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_FIT} :
            (Leaf_Size){LEAF_SIZE_FIT, LEAF_SIZE_FULL}
        )
    });

    return container;
}

static void _uph_ui_scrollbar_custom_draw(void)
{
    
}

void uph_ui_end_scroll_container(Uph_UIScrollContainer *container)
{
    static int frame_counter = 0;
    frame_counter++;

    leaf_end_element();
    leaf_end_element();

    const float max_size = (float)container->container_max_size;
    const float content_size = (float)container->container_current_size;

    const bool needs_scrollbar = content_size > max_size;

    if (needs_scrollbar)
    {
        const int32_t scrollbar_width = NAUI_DPI(16);
        const int32_t scrollbar_length = _uph_ui_thumb_length(max_size, content_size);

        leaf({
            .size = (container->direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
                (Leaf_Size){LEAF_SIZE_FIXED(scrollbar_width), LEAF_SIZE_FULL} :
                (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_FIXED(scrollbar_width)}
            ),
            .color = naui_theme_color("uph_ui_scrollbar_bg_color")
        })
        {
            const float max_scroll = content_size - max_size;
            const float track_length = max_size;
            const float thumb_travel = track_length - (float)scrollbar_length;

            const float t = NAUI_CLAMP(*container->out_scroll / max_scroll, 0.0f, 1.0f);
            const float handle_offset = t * thumb_travel;

            leaf({
                .id = container->handle_id,
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                .floating = {
                    .offset = (container->direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
                        (Leaf_Vec2){0, handle_offset} :
                        (Leaf_Vec2){handle_offset, 0}
                    )
                },
                .size = (container->direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
                    (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_FIXED(scrollbar_length)} :
                    (Leaf_Size){LEAF_SIZE_FIXED(scrollbar_length), LEAF_SIZE_FULL}
                ),
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_scrollbar_handle_rounding")), LEAF_CORNER_ALL),
                .color = naui_theme_color("uph_ui_scrollbar_handle_color")
            });
        }
    }

    leaf_end_element();
}