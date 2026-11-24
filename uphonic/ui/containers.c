typedef uint8_t Uph_UIScrollDirection;
enum
{
    UPH_UI_SCROLL_DIRECTION_VERTICAL,
    UPH_UI_SCROLL_DIRECTION_HORIZONTAL
};

typedef struct
{
    Uph_UIScrollDirection direction;
    Leaf_ID handle_id;
    float *out_scroll;
    float container_max_size;
    float container_current_size;
    bool handle_hovered;
}
Uph_UIScrollContainer;

typedef struct
{
    Leaf_ID handle_x_id, handle_y_id;
    float *scroll_x, *scroll_y;
    Leaf_Vec2 viewport_size;
    Leaf_Vec2 content_size;
}
Uph_UIScrollArea;

static int32_t uph__ui_thumb_length(float max_size, float content_size)
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
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
        .direction = direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            LEAF_DIRECTION_HORIZONTAL : LEAF_DIRECTION_VERTICAL,
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

    if (max_area_box.width <= 0.0f && max_area_box.height <= 0.0f)
        naui_app_stale_frame();

    const bool vertical = direction == UPH_UI_SCROLL_DIRECTION_VERTICAL;
    const float max_size = (float)container.container_max_size;
    const float content_size = (float)container.container_current_size;
    const float max_scroll = content_size - max_size;
    const bool this_drag = drag_active && drag_id.value == scroll_id.value;

    if (max_scroll > 0.0f)
    {
        const float thumb_travel = max_size - (float)uph__ui_thumb_length(max_size, content_size);
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
        else if (naui_panel_is_hovered(naui_panel_current()))
        {
            if (leaf_hovered(max_area_id) || leaf_hovered(scroll_id))
                *out_scroll += (float)naui_mouse_scroll_delta() * (vertical ? -20.0f : 20.0f);

            if (leaf_hovered(scroll_id))
            {
                uph_global_widget_state.any_widget_hovered = true;
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
        .size = (direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_FULL} :
            (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_GROW}
        ),
        .direction = (Leaf_LayoutDirection)direction
    });

    *out_scroll = NAUI_CLAMP(*out_scroll, 0.0f, NAUI_MAX(0.0f, max_scroll));

    leaf_begin_element((Leaf_ElementConfig){
        .id = current_area_id,
        .size = (direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_FIT} :
            (Leaf_Size){LEAF_SIZE_FIT, LEAF_SIZE_FULL}
        ),
        .child_offset = (direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
            (Leaf_Vec2){0.0f, *out_scroll} :
            (Leaf_Vec2){*out_scroll, 0.0f}
        ),
        .direction = (Leaf_LayoutDirection)direction
    });

    return container;
}

void uph_ui_end_scroll_container(Uph_UIScrollContainer *container)
{
    leaf_end_element();
    leaf_end_element();

    const float max_size = (float)container->container_max_size;
    const float content_size = (float)container->container_current_size;

    const bool needs_scrollbar = content_size > max_size;

    if (needs_scrollbar)
    {
        const int32_t scrollbar_width = NAUI_DPI(16);
        const int32_t scrollbar_length = uph__ui_thumb_length(max_size, content_size);

        leaf({
            .size = (container->direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
                (Leaf_Size){LEAF_SIZE_FIXED(scrollbar_width), LEAF_SIZE_FULL} :
                (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_FIXED(scrollbar_width)}
            ),
            .color = uph_state.colors.ui_scrollbar_bg
        })
        {
            const float max_scroll = content_size - max_size;
            const float track_length = max_size;
            const float thumb_travel = track_length - (float)scrollbar_length;

            const float t = NAUI_CLAMP(*container->out_scroll / max_scroll, 0.0f, 1.0f);
            const float handle_offset = t * thumb_travel;

            leaf({
                .id = container->handle_id,
                .size = (container->direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
                    (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_FIXED(scrollbar_length)} :
                    (Leaf_Size){LEAF_SIZE_FIXED(scrollbar_length), LEAF_SIZE_FULL}
                ),
                .color = uph_state.colors.ui_scrollbar_handle,
                .floating = {
                    .offset = (container->direction == UPH_UI_SCROLL_DIRECTION_VERTICAL ?
                        (Leaf_Vec2){0, handle_offset} :
                        (Leaf_Vec2){handle_offset, 0}
                    )
                },
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_SCROLLBAR_HANDLE_ROUNDING), LEAF_CORNER_ALL),
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
            });
        }
    }

    leaf_end_element();
}

typedef struct
{
    struct
    {
        bool active;
        Leaf_ID id;
        int32_t start_mouse;
        float start_scroll;
    }
    drag;

    struct
    {
        bool active;
        Leaf_ID id;
        int32_t last_mouse_x, last_mouse_y;
    }
    pan;
}
Uph_UIScrollAreaState;

static Uph_UIScrollAreaState uph__ui_scroll_area_state;

static bool uph__ui_scroll_area_axis(bool vertical, float *scroll, Leaf_ID handle_id, float max_size, float content_size)
{
    Uph_UIScrollAreaState *state = &uph__ui_scroll_area_state;

    const float max_scroll = content_size - max_size;
    const bool this_drag = state->drag.active && state->drag.id.value == handle_id.value;
    if (max_scroll <= 0.0f)
    {
        if (this_drag)
            state->drag.active = false;
        return false;
    }

    const int32_t mouse_pos = vertical ? naui_mouse_y() : naui_mouse_x();
    if (this_drag)
    {
        if (naui_mouse_down(NAUI_MOUSE_LEFT))
        {
            const float thumb_travel = max_size - (float)uph__ui_thumb_length(max_size, content_size);
            if (thumb_travel > 0.0f)
                *scroll = state->drag.start_scroll + (float)(mouse_pos - state->drag.start_mouse) * (max_scroll / thumb_travel);
        }
        else
            state->drag.active = false;
        naui_set_cursor(NAUI_CURSOR_HAND);
        return true;
    }

    if (naui_panel_is_hovered(naui_panel_current()) && leaf_hovered(handle_id))
    {
        uph_global_widget_state.any_widget_hovered = true;
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            state->drag.active = true;
            state->drag.id = handle_id;
            state->drag.start_mouse = mouse_pos;
            state->drag.start_scroll = *scroll;
        }
        return true;
    }
    return false;
}

static void uph__ui_scroll_area_bar(bool vertical, Leaf_ID handle_id, float scroll, float max_size, float content_size)
{
    const int32_t thumb_length = uph__ui_thumb_length(max_size, content_size);
    const float t = NAUI_CLAMP(scroll / (content_size - max_size), 0.0f, 1.0f);
    const float handle_offset = t * (max_size - (float)thumb_length);
    const int32_t scrollbar_width = NAUI_DPI(16);

    leaf({
        .size = vertical ?
            (Leaf_Size){LEAF_SIZE_FIXED(scrollbar_width), LEAF_SIZE_FULL} :
            (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_FIXED(scrollbar_width)},
        .color = uph_state.colors.ui_scrollbar_bg
    })
    {
        leaf({
            .id = handle_id,
            .size = vertical ?
                (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_FIXED(thumb_length)} :
                (Leaf_Size){LEAF_SIZE_FIXED(thumb_length), LEAF_SIZE_FULL},
            .color = uph_state.colors.ui_scrollbar_handle,
            .floating = {
                .offset = vertical ? (Leaf_Vec2){0, handle_offset} : (Leaf_Vec2){handle_offset, 0}
            },
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_SCROLLBAR_HANDLE_ROUNDING), LEAF_CORNER_ALL),
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
        });
    }
}

Uph_UIScrollArea uph_ui_begin_scroll_area(float *scroll_x, float *scroll_y, float min_content_height, Leaf_ID scroll_id)
{
    Uph_UIScrollAreaState *state = &uph__ui_scroll_area_state;
    Uph_UIScrollArea area = {
        .handle_x_id = scroll_id,
        .handle_y_id = {.value = scroll_id.value + 3},
        .scroll_x = scroll_x,
        .scroll_y = scroll_y
    };

    const Leaf_ID content_id = {.value = scroll_id.value + 1};
    const Leaf_ID viewport_id = {.value = scroll_id.value + 2};

    const Leaf_BoundingBox content_box = leaf_get_bounding_box(content_id);
    const Leaf_BoundingBox viewport_box = leaf_get_bounding_box(viewport_id);
    area.content_size = (Leaf_Vec2){content_box.width, content_box.height};
    area.viewport_size = (Leaf_Vec2){viewport_box.width, viewport_box.height};

    const float max_scroll_x = NAUI_MAX(0.0f, area.content_size.x - area.viewport_size.x);
    const float max_scroll_y = NAUI_MAX(0.0f, area.content_size.y - area.viewport_size.y);

    const bool handle_x_active = uph__ui_scroll_area_axis(false, scroll_x, area.handle_x_id, area.viewport_size.x, area.content_size.x);
    const bool handle_y_active = uph__ui_scroll_area_axis(true, scroll_y, area.handle_y_id, area.viewport_size.y, area.content_size.y);

    const bool this_pan = state->pan.active && state->pan.id.value == scroll_id.value;
    if (this_pan)
    {
        if (naui_mouse_down(NAUI_MOUSE_MIDDLE))
        {
            *scroll_x -= (float)(naui_mouse_x() - state->pan.last_mouse_x);
            *scroll_y -= (float)(naui_mouse_y() - state->pan.last_mouse_y);
            state->pan.last_mouse_x = naui_mouse_x();
            state->pan.last_mouse_y = naui_mouse_y();
            naui_set_cursor(NAUI_CURSOR_RESIZE_ALL);
        }
        else
            state->pan.active = false;
    }
    else if (!state->drag.active && (max_scroll_x > 0.0f || max_scroll_y > 0.0f) &&
        naui_mouse_pressed(NAUI_MOUSE_MIDDLE) && naui_panel_is_hovered(naui_panel_current()) && leaf_hovered(viewport_id))
    {
        state->pan.active = true;
        state->pan.id = scroll_id;
        state->pan.last_mouse_x = naui_mouse_x();
        state->pan.last_mouse_y = naui_mouse_y();
        naui_set_cursor(NAUI_CURSOR_RESIZE_ALL);
    }

    const int8_t wheel = naui_mouse_scroll_delta();
    if (wheel != 0 && !state->drag.active && naui_panel_is_hovered(naui_panel_current()))
    {
        float *target = NULL;
        if (handle_x_active)
            target = scroll_x;
        else if (handle_y_active)
            target = scroll_y;
        else if (leaf_hovered(viewport_id))
        {
            const bool prefer_x = uph_ui_shift_down();
            const bool can_x = max_scroll_x > 0.0f;
            const bool can_y = max_scroll_y > 0.0f;
            if (prefer_x)
                target = can_x ? scroll_x : can_y ? scroll_y : NULL;
            else
                target = can_y ? scroll_y : can_x ? scroll_x : NULL;
        }
        if (target)
            *target += (float)wheel * (target == scroll_x ? 20.0f : -20.0f);
    }

    *scroll_x = NAUI_CLAMP(*scroll_x, 0.0f, max_scroll_x);
    *scroll_y = NAUI_CLAMP(*scroll_y, 0.0f, max_scroll_y);

    leaf_begin_element((Leaf_ElementConfig){
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
        .direction = LEAF_DIRECTION_VERTICAL,
        .clip_children = true
    });
    leaf_begin_element((Leaf_ElementConfig){
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
        .direction = LEAF_DIRECTION_HORIZONTAL
    });
    leaf_begin_element((Leaf_ElementConfig){
        .id = viewport_id,
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .clip_children = true
    });
    leaf_begin_element((Leaf_ElementConfig){
        .id = content_id,
        .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_PERCENT_MIN(1.0f, NAUI_MAX(0.0f, min_content_height)) },
        .child_offset = { .x = *scroll_x, .y = *scroll_y },
        .direction = LEAF_DIRECTION_HORIZONTAL
    });

    return area;
}

void uph_ui_end_scroll_area(Uph_UIScrollArea *area)
{
    leaf_end_element();
    leaf_end_element();

    const bool needs_x = area->content_size.x > area->viewport_size.x && area->viewport_size.x > 0.0f;
    const bool needs_y = area->content_size.y > area->viewport_size.y && area->viewport_size.y > 0.0f;

    if (needs_y)
        uph__ui_scroll_area_bar(true, area->handle_y_id, *area->scroll_y, area->viewport_size.y, area->content_size.y);
    leaf_end_element();

    if (needs_x)
    {
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            uph__ui_scroll_area_bar(false, area->handle_x_id, *area->scroll_x, area->viewport_size.x, area->content_size.x);
            if (needs_y)
            {
                leaf({
                    .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(16)), .height = LEAF_SIZE_FIXED(NAUI_DPI(16)) },
                    .color = uph_state.colors.ui_scrollbar_bg
                });
            }
        }
    }

    leaf_end_element();
}

typedef struct
{
    float start_mouse_x;
    float start_width;
    bool hovered;
    bool resizing;
}
Uph_UISplitter;

bool uph_ui_splitter_update(Uph_UISplitter *splitter, Leaf_BoundingBox left, float gap, float *width, float min_width, float max_width, bool enabled)
{
    if (!enabled)
    {
        splitter->hovered = false;
        splitter->resizing = false;
        return false;
    }

    const float edge = left.x + left.width;
    const float mouse_x = (float)naui_mouse_x();
    const float mouse_y = (float)naui_mouse_y();
    splitter->hovered = naui_panel_is_hovered(naui_panel_current()) && mouse_y >= left.y && mouse_y < left.y + left.height && mouse_x >= edge - 1.0f && mouse_x <= edge + gap;

    if (splitter->hovered && !splitter->resizing && naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        splitter->resizing = true;
        splitter->start_mouse_x = mouse_x;
        splitter->start_width = *width;
    }

    if (splitter->resizing)
    {
        *width = splitter->start_width + (mouse_x - splitter->start_mouse_x) / naui_app_dpi_scale();
        if (!naui_mouse_down(NAUI_MOUSE_LEFT))
            splitter->resizing = false;
    }

    *width = NAUI_CLAMP(*width, min_width, NAUI_MAX(min_width, max_width));

    const bool active = splitter->hovered || splitter->resizing;
    if (active)
        naui_set_cursor(NAUI_CURSOR_RESIZE_EW);

    return active;
}
