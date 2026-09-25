bool naui_key_down(Naui_Key key)
{
    return mgapp_key_down((mg_key)key);
}

bool naui_key_pressed(Naui_Key key)
{
    return mgapp_key_pressed((mg_key)key);
}

uint32_t naui_app_char_pressed(void)
{
    //return mgapp_char_pressed();
    return 0;
}

bool naui_mouse_down(Naui_MouseButton button)
{
    return mgapp_mouse_down((mg_mouse_button)button);
}

bool naui_mouse_pressed(Naui_MouseButton button)
{
    return mgapp_mouse_pressed((mg_mouse_button)button);
}

bool naui_mouse_released(Naui_MouseButton button)
{
    return mgapp_mouse_released((mg_mouse_button)button);
}

bool naui_mouse_clicked(Naui_MouseButton button)
{
    return mgapp_mouse_clicked((mg_mouse_button)button);
}

bool naui_mouse_double_clicked(Naui_MouseButton button)
{
    return mgapp_mouse_double_clicked((mg_mouse_button)button);
}

int8_t naui_mouse_scroll_delta(void)
{
    return mgapp_mouse_scroll_delta();
}

int32_t naui_mouse_x(void)
{
    return mgapp_mouse_x();
}

int32_t naui_mouse_y(void)
{
    return mgapp_mouse_y();
}

void naui_set_cursor(Naui_Cursor cursor)
{
    mgapp_set_cursor((mg_cursor)cursor);
}

static bool s_dragging[MG_MOUSE_BUTTON_MAX];
void naui_input_update(void)
{
    static int32_t s_drag_start_x[MG_MOUSE_BUTTON_MAX];
    static int32_t s_drag_start_y[MG_MOUSE_BUTTON_MAX];

    #define DRAG_THRESHOLD 6

    for (int i = 0; i < MG_MOUSE_BUTTON_MAX; i++)
    {
        if (mgapp_mouse_pressed((mg_mouse_button)i))
        {
            s_drag_start_x[i] = mgapp_mouse_x();
            s_drag_start_y[i] = mgapp_mouse_y();
            s_dragging[i] = false;
        }
        else if (mgapp_mouse_down((mg_mouse_button)i))
        {
            int32_t dx = mgapp_mouse_x() - s_drag_start_x[i];
            int32_t dy = mgapp_mouse_y() - s_drag_start_y[i];
            if (!s_dragging[i] && (dx * dx + dy * dy) > (DRAG_THRESHOLD * DRAG_THRESHOLD))
                s_dragging[i] = true;
        }
        else
        {
            s_dragging[i] = false;
        }
    }
}

bool naui_mouse_dragging(Naui_MouseButton button)
{
    return s_dragging[(int)button];
}
