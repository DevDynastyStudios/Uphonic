static void uph__ui_render_menu_dropdown_child(Uph_UIMenuNode *node)
{
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const Leaf_Color text_color = uph_state.colors.ui_text;

    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);

    leaf({
        .id = node->element_id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = leaf_hovered(node->element_id) ?
            uph_state.colors.ui_frame_hovered_bg :
            LEAF_COLOR_TRANSPARENT,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_X_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
        .child_gap = NAUI_DPI(8),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        if (node->check != UPH_UI_MENU_CHECK_NONE)
        {
            const float box_size = font_size * 0.85f;
            leaf({
                .size = { .width = LEAF_SIZE_FIXED(box_size), .height = LEAF_SIZE_FIXED(box_size) },
                .border = uph__ui_frame_border(),
                .color = uph_state.colors.ui_frame_bg,
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING) * 0.75f, NAUI_CORNER_ALL)
            })
            {
                if (node->check == UPH_UI_MENU_CHECK_ON)
                {
                    leaf({
                        .size = { .width = LEAF_SIZE_PERCENT(0.6f), .height = LEAF_SIZE_PERCENT(0.6f) },
                        .color = uph_state.colors.ui_slider_fill,
                        .floating = {
                            .parent_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
                            .self_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
                        },
                        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING) * 0.4f, NAUI_CORNER_ALL),
                        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
                    });
                }
            }
        }

        leaf_text(node->text, {
            .color = text_color,
            .font_size = LEAF_SIZE_FIXED(font_size)
        });
        if (node->shortcut)
        {
            leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT } });
            leaf_text(node->shortcut, {
                .color = uph_state.colors.panel_text,
                .font_size = LEAF_SIZE_FIXED(font_size)
            });
        }
        if (node->first_child)
        {
            leaf({
                .image = uph_state.icons.dropdown,
                .size = { .width = LEAF_SIZE_FIXED(font_size), .height = LEAF_SIZE_FIXED(font_size) },
                .color = text_color,
                .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
            });
        }
    }
}

static bool uph__ui_menu_dropdown_hovered(Uph_UIMenuNode *node)
{
    if (leaf_hovered(node->element_id))
        return true;
    else
    {
        Uph_UIMenuNode *current_child = node->first_child;
        while (current_child)
        {
            if (uph__ui_menu_dropdown_hovered(current_child))
                return true;
            current_child = current_child->next_sibling;
        }
    }
    return false;
}

static bool uph__ui_menu_dropdown_children_hovered(Uph_UIMenuNode *menu)
{
    for (Uph_UIMenuNode *child = menu->first_child; child; child = child->next_sibling)
        if (uph__ui_menu_dropdown_hovered(child))
            return true;
    return false;
}

static void uph__ui_render_menu_dropdown_recursive(Uph_UIMenuNode *node, Naui_Vec2 position_offset, float reveal)
{
    position_offset.y += (1.0f - reveal) * NAUI_DPI(-4);

    leaf({
        .color = uph_state.colors.ui_dropdown_bg,
        .floating = {
            .offset = { .x = position_offset.x, .y = position_offset.y }
        },
        .transparency = 1.0f - reveal,
        .shadow = {
            .blur_radius = NAUI_DPI(16),
            .color = uph_state.colors.ui_dropdown_shadow
        },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
        .uniform_children = LEAF_UNIFORM_SIZING_WIDTH,
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
    })
    {
        while (node)
        {
            if (uph__ui_menu_dropdown_hovered(node))
            {
                Leaf_BoundingBox bbox = leaf_get_bounding_box(node->element_id);
                uph__ui_render_menu_dropdown_recursive(node->first_child, (Naui_Vec2){bbox.x + bbox.width, bbox.y}, 1.0f);
            }
            uph__ui_render_menu_dropdown_child(node);
            node = node->next_sibling;
        }
    }
}

static Uph_UIMenuNode *uph__ui_menu_root(Uph_GlobalWidgetState *data, const Leaf_ID element_id)
{
    Uph_UIMenuNode *menu = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));
    menu->element_id = element_id;
    if (data->open_menu_key != 0 && element_id.value == data->open_menu_key)
        data->current_open_menu = menu;
    return menu;
}

static void uph__ui_open_menu(Uph_GlobalWidgetState *data, Uph_UIMenuNode *menu, bool context)
{
    data->current_open_menu = menu;
    data->open_menu_key = menu->element_id.value;
    data->is_current_menu_context = context;
    data->menu_opened_last_frame = false;
    data->visual.menu_lifetime = 0.0f;
}

static void uph__ui_close_menu(Uph_GlobalWidgetState *data)
{
    data->current_open_menu = NULL;
    data->open_menu_key = 0;
}

static bool uph__ui_menu_is_open(const Uph_GlobalWidgetState *data, const Uph_UIMenuNode *menu)
{
    return data->open_menu_key != 0 && menu->element_id.value == data->open_menu_key;
}

static void uph__ui_render_menu_dropdown(Uph_GlobalWidgetState *data)
{
    data->menu_dropdown_hovered = false;
    if (data->current_open_menu)
    {
        data->visual.menu_lifetime = naui_app_animations_enabled() ? NAUI_CLAMP01(data->visual.menu_lifetime + naui_delta_time() * NAUI_UI_ANIM_SPEED) : 1.0f;

        if (data->current_open_menu->deferred_render)
        {
            naui_panel_occlude_all();
            Leaf_ID id = leaf_id("uph_widget_current_custom_context_menu");
            Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
            const float reveal = naui_ease_out_cubic(data->visual.menu_lifetime);
            leaf({
                .id = id,
                .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x + 2)),
                .border = uph__ui_frame_border(),
                .color = uph_state.colors.ui_dropdown_bg,
                .floating = {
                    .offset = { .x = data->current_context_menu_position.x, .y = data->current_context_menu_position.y + (1.0f - reveal) * NAUI_DPI(-4) }
                },
                .transparency = 1.0f - reveal,
                .shadow = {
                    .blur_radius = NAUI_DPI(16),
                    .color = uph_state.colors.ui_dropdown_shadow
                },
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING) * 1.5f, LEAF_CORNER_ALL),
                .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
            }) data->current_open_menu->deferred_render((Uph_UIMenuID)data->current_open_menu);

            if (data->menu_opened_last_frame && naui_any_mouse_pressed() && !leaf_hovered(id))
            {
                uph__ui_close_menu(data);
                return;
            }

            data->menu_opened_last_frame = true;
            return;
        }

        Uph_UIMenuNode *current_node = data->current_open_menu->first_child;
        if (!current_node)
        {
            uph__ui_close_menu(data);
            return;
        }

        naui_panel_occlude_all();

        data->menu_dropdown_hovered = uph__ui_menu_dropdown_children_hovered(data->current_open_menu);

        if (data->menu_opened_last_frame && naui_any_mouse_pressed() && !uph__ui_menu_dropdown_hovered(data->current_open_menu))
        {
            uph__ui_close_menu(data);
            return;
        }

        if (!data->is_current_menu_context)
        {
            Leaf_BoundingBox bbox = leaf_get_bounding_box(data->current_open_menu->element_id);
            uph__ui_render_menu_dropdown_recursive(current_node, (Naui_Vec2){bbox.x, bbox.y + bbox.height}, naui_ease_out_cubic(data->visual.menu_lifetime));
        }
        else
        {
            uph__ui_render_menu_dropdown_recursive(current_node, data->current_context_menu_position, naui_ease_out_cubic(data->visual.menu_lifetime));
        }

        data->menu_opened_last_frame = true;
    }
}

Uph_UIMenuID uph_ui_menu(const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;

    Uph_UIMenuNode *menu = uph__ui_menu_root(data, element_id);

    const bool blocked_by_dropdown = data->open_menu_key != 0 && data->menu_dropdown_hovered;
    bool hovered = !blocked_by_dropdown && uph_ui_widget_hovered(element_id);
    if (!hovered && !blocked_by_dropdown && data->open_menu_key != 0 && !data->is_current_menu_context && !uph_dialog_blocks_input())
        hovered = leaf_hovered_clipped(element_id);
    if (hovered)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
        {
            if (data->open_menu_key != 0)
                uph__ui_close_menu(data);
            else
                uph__ui_open_menu(data, menu, false);
        }
        else if (!data->is_current_menu_context && data->open_menu_key != 0)
        {
            if (data->open_menu_key != element_id.value)
                data->visual.menu_lifetime = 1.0f;
            data->current_open_menu = menu;
            data->open_menu_key = element_id.value;
        }
    }

    const Naui_Vec2 padding = UPH_UI_MENU_BAR_ITEM_PADDING;
    leaf({
        .id = element_id,
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = (uph__ui_menu_is_open(data, menu) || hovered) ?
            uph_state.colors.ui_frame_hovered_bg : LEAF_COLOR_TRANSPARENT,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL)
    })
    {
        leaf_text(name, {
            .color = uph_state.colors.ui_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
        });
    }

    return (Uph_UIMenuID)menu;
}

Uph_UIMenuID uph_ui_context_menu(const Leaf_ID id)
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;
    Uph_UIMenuNode *menu = uph__ui_menu_root(data, id);
    return (Uph_UIMenuID)menu;
}

Uph_UIMenuID uph_ui_context_menu_custom(const Leaf_ID id, void (*custom_render)(Uph_UIMenuID menu_id))
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;
    Uph_UIMenuNode *menu = uph__ui_menu_root(data, id);
    menu->deferred_render = custom_render;
    return (Uph_UIMenuID)menu;
}

void uph_ui_open_context_menu(Uph_UIMenuID menu)
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;
    uph__ui_open_menu(data, (Uph_UIMenuNode*)menu, true);
    data->current_context_menu_position = (Naui_Vec2){naui_mouse_x(), naui_mouse_y()};
}

bool uph_ui_any_menu_open(void)
{
    return uph_global_widget_state.open_menu_key != 0;
}

static void uph__ui_append_menu_child(Uph_UIMenuNode *parent, Uph_UIMenuNode *child)
{
    if (!parent->first_child)
    {
        parent->first_child = child;
        parent->last_child = child;
    }
    else
    {
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }
}

Uph_UIMenuID uph_ui_submenu(Uph_UIMenuID parent_id, const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;

    Uph_UIMenuNode *parent = (Uph_UIMenuNode*)parent_id;
    Uph_UIMenuNode *menu = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));
    menu->element_id = element_id;

    const uint32_t name_length = strlen(name);
    menu->text = naui_arena_alloc(&data->menu_arena, name_length + 1);
    memcpy(menu->text, name, name_length);
    menu->text[name_length] = '\0';

    uph__ui_append_menu_child(parent, menu);

    return (Uph_UIMenuID)menu;
}

bool uph_ui_menu_item(Uph_UIMenuID menu_id, const char *name, const Leaf_ID element_id)
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;

    Uph_UIMenuNode *parent = (Uph_UIMenuNode*)menu_id;
    Uph_UIMenuNode *item = naui_arena_alloc(&data->menu_arena, sizeof(Uph_UIMenuNode));

    item->element_id = element_id;
    const uint32_t name_length = strlen(name);
    item->text = naui_arena_alloc(&data->menu_arena, name_length + 1);
    memcpy(item->text, name, name_length);
    item->text[name_length] = '\0';
    uph__ui_append_menu_child(parent, item);

    bool hovered = leaf_hovered(element_id);

    if (hovered)
        naui_set_cursor(NAUI_CURSOR_HAND);

    bool result;
    if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        uph__ui_close_menu(&uph_global_widget_state);
        result = true;
    }
    else result = false;

    return result;
}

bool uph_ui_menu_shortcut_item(Uph_UIMenuID menu_id, const char *name, const Leaf_ID element_id, Naui_Keybind chord)
{
    const bool clicked = uph_ui_menu_item(menu_id, name, element_id);

    const Naui_String text = naui_keybind_to_string(chord);
    if (text.length > 0)
    {
        Uph_UIMenuNode *parent = (Uph_UIMenuNode*)menu_id;
        char *shortcut = naui_arena_alloc(&uph_global_widget_state.menu_arena, text.length + 1);
        memcpy(shortcut, text.data, text.length + 1);
        parent->last_child->shortcut = shortcut;
    }

    return clicked;
}

bool uph_ui_menu_keybind_item(Uph_UIMenuID menu_id, const char *name, const Leaf_ID element_id, Naui_Keybind chord)
{
    const bool clicked = uph_ui_menu_shortcut_item(menu_id, name, element_id, chord);
    return clicked || (!uph_dialog_blocks_input() && naui_keybind_pressed(chord));
}

bool uph_ui_menu_check_item(Uph_UIMenuID menu_id, const char *name, const Leaf_ID element_id, bool checked)
{
    const bool result = uph_ui_menu_item(menu_id, name, element_id);
    Uph_UIMenuNode *parent = (Uph_UIMenuNode*)menu_id;
    parent->last_child->check = checked ? UPH_UI_MENU_CHECK_ON : UPH_UI_MENU_CHECK_OFF;

    const bool right_clicked = leaf_hovered(element_id) && naui_mouse_pressed(NAUI_MOUSE_RIGHT);
    return result || right_clicked;
}
