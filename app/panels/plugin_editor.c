typedef struct
{
    Uph_PluginInternalHandle *handle;
    int32_t last_mouse_x, last_mouse_y;
    uint8_t buttons_down;
    Naui_Vec2 chrome;
    Naui_Vec2 size_history[2];
    uint8_t size_history_count;
}
Uph_PluginEditorData;

typedef struct
{
    Naui_Texture texture;
    uint32_t width, height;
}
Uph_PluginEditorDrawData;

NAUI_PANEL_WITH_DATA(uph_plugin_editor, Uph_PluginEditorData)

#define UPH_PLUGIN_EDITOR_TITLEBAR_GUESS 32

static void _uph_plugin_editor_reset_fit(Uph_PluginEditorData *data)
{
    data->chrome = (Naui_Vec2){ FLT_MAX, FLT_MAX };
    data->size_history_count = 0;
}

static void uph_plugin_editor_on_attach(void)
{
    Uph_PluginEditorData *data = (Uph_PluginEditorData*)naui_panel_current_data();
    memset(data, 0, sizeof(Uph_PluginEditorData));
    _uph_plugin_editor_reset_fit(data);
}

static void uph_plugin_editor_on_detach(void) {}
static void uph_plugin_editor_on_open(void) {}

static void uph_plugin_editor_on_close(void)
{
    Uph_PluginEditorData *data = (Uph_PluginEditorData*)naui_panel_current_data();
    data->buttons_down = 0;
    _uph_plugin_editor_reset_fit(data);
    if (data->handle)
        uph_plugin_gui_closed(data->handle);
}

static void _uph_plugin_editor_draw(Leaf_BoundingBox box, Uph_PluginEditorDrawData *draw)
{
    if (!draw->texture)
        return;
    naui_draw_texture(draw->texture, (Naui_Vec2){ floorf(box.x), floorf(box.y) }, (Naui_Vec2){ (float)draw->width, (float)draw->height }, (Leaf_Color){ 255, 255, 255, 255 });
}

static void _uph_plugin_editor_fit(Uph_PluginEditorData *data, Naui_PanelID panel, Leaf_BoundingBox body)
{
    if (data->size_history_count < 2)
        return;

    const Naui_Vec2 laid_out = data->size_history[1];
    data->chrome.x = fminf(data->chrome.x, laid_out.x - (float)body.width);
    data->chrome.y = fminf(data->chrome.y, laid_out.y - (float)body.height);

    uint32_t width, height;
    uph_plugin_gui_size(data->handle, &width, &height);

    const Naui_Vec2 target = {
        roundf((float)width + data->chrome.x),
        roundf((float)height + data->chrome.y)
    };
    const Naui_Vec2 size = naui_panel_size(panel);
    if (target.x != size.x || target.y != size.y)
        naui_panel_set_size(panel, target);
}

static void _uph_plugin_editor_track_size(Uph_PluginEditorData *data, Naui_PanelID panel)
{
    data->size_history[1] = data->size_history[0];
    data->size_history[0] = naui_panel_size(panel);
    if (data->size_history_count < 2)
        data->size_history_count++;
}

static void _uph_plugin_editor_forward_input(Uph_PluginEditorData *data, Naui_PanelID panel, Leaf_BoundingBox body)
{
    const int32_t x = naui_mouse_x() - (int32_t)floorf(body.x);
    const int32_t y = naui_mouse_y() - (int32_t)floorf(body.y);
    const bool inside = x >= 0 && y >= 0 && x < (int32_t)body.width && y < (int32_t)body.height;
    const bool hovered = inside && naui_panel_is_hovered(panel);

#if NAUI_WINDOWS
    bool any_down = false;
    for (Naui_MouseButton button = 0; button < NAUI_MOUSE_MAX; button++)
        any_down |= naui_mouse_down(button);
    uph_plugin_gui_native_input(data->handle, hovered && !any_down);
#else
    if (!hovered && !data->buttons_down)
        return;

    if (x != data->last_mouse_x || y != data->last_mouse_y)
    {
        uph_plugin_gui_mouse_move(data->handle, x, y);
        data->last_mouse_x = x;
        data->last_mouse_y = y;
    }

    for (Naui_MouseButton button = 0; button < NAUI_MOUSE_MAX; button++)
    {
        const uint8_t bit = (uint8_t)(1 << button);
        if (hovered && naui_mouse_pressed(button))
        {
            data->buttons_down |= bit;
            uph_plugin_gui_mouse_button(data->handle, button, true, x, y);
        }
        else if ((data->buttons_down & bit) && !naui_mouse_down(button))
        {
            data->buttons_down &= (uint8_t)~bit;
            uph_plugin_gui_mouse_button(data->handle, button, false, x, y);
        }
    }

    if (hovered)
        uph_plugin_gui_mouse_scroll(data->handle, naui_mouse_scroll_delta(), x, y);
#endif
}

static void uph_plugin_editor_on_update(void)
{
    Uph_PluginEditorData *data = (Uph_PluginEditorData*)naui_panel_current_data();
    const Naui_PanelID panel = naui_panel_current();

    if (!data->handle)
        return;

    const Leaf_BoundingBox body = naui_panel_bounding_box(panel);
    Uph_PluginEditorDrawData draw = { 0 };
    draw.texture = uph_plugin_gui_texture(data->handle, &draw.width, &draw.height);

    leaf({
        .custom_draw = (Leaf_CustomDrawFn)_uph_plugin_editor_draw,
        .custom_draw_data = LEAF_DATA_SLICE(draw),
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL }
    }) {}

    if (body.width <= 0.0f || body.height <= 0.0f)
    {
        _uph_plugin_editor_track_size(data, panel);
        return;
    }

    _uph_plugin_editor_fit(data, panel, body);
    _uph_plugin_editor_track_size(data, panel);
    uph_plugin_gui_set_origin(data->handle, (int32_t)floorf(body.x), (int32_t)floorf(body.y));
    _uph_plugin_editor_forward_input(data, panel, body);
}

Naui_PanelID uph_plugin_editor_create(Uph_PluginInternalHandle *handle)
{
    const Naui_PanelID panel = NAUI_ATTACH_PANEL(uph_plugin_editor);
    Uph_PluginEditorData *data = (Uph_PluginEditorData*)naui_panel_data(panel);

    uint32_t width, height;
    uph_plugin_gui_size(handle, &width, &height);

    naui_panel_set_title(panel, uph_plugin_gui_title(handle));
    naui_panel_set_min_size(panel, (Naui_Vec2){ NAUI_DPI(64), NAUI_DPI(64) });
    naui_panel_set_size(panel, (Naui_Vec2){ (float)width, (float)height + NAUI_DPI(UPH_PLUGIN_EDITOR_TITLEBAR_GUESS) });
    naui_panel_enable_flags(panel, NAUI_PANEL_FLAG_NO_RESIZE | NAUI_PANEL_FLAG_NO_DOCK);

    naui_panel_close(panel);
    data->handle = handle;

    return panel;
}

void uph_plugin_editor_destroy(Naui_PanelID panel)
{
    Uph_PluginEditorData *data = (Uph_PluginEditorData*)naui_panel_data(panel);
    data->handle = NULL;
    naui_panel_close(panel);
    naui_panel_detach(panel);
}
