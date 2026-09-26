bool naui_key_down(Naui_Key key)
{
    return mgapp_key_down((mg_key)key);
}

bool naui_key_pressed(Naui_Key key)
{
    return mgapp_key_pressed((mg_key)key);
}

bool naui_key_pressed_repeat(Naui_Key key)
{
    return mgapp_key_pressed_repeat((mg_key)key);
}

uint32_t naui_app_codepoint(void)
{
    return mgapp_codepoint();
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

typedef struct
{
    bool dragging[MG_MOUSE_BUTTON_MAX];
    Naui_Cursor current_cursor;
}
Naui_GlobalInputState;
static Naui_GlobalInputState naui_input_state;

void naui_set_cursor(Naui_Cursor cursor)
{
    naui_input_state.current_cursor = cursor;
}

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
            naui_input_state.dragging[i] = false;
        }
        else if (mgapp_mouse_down((mg_mouse_button)i))
        {
            int32_t dx = mgapp_mouse_x() - s_drag_start_x[i];
            int32_t dy = mgapp_mouse_y() - s_drag_start_y[i];
            if (!naui_input_state.dragging[i] && (dx * dx + dy * dy) > (DRAG_THRESHOLD * DRAG_THRESHOLD))
                naui_input_state.dragging[i] = true;
        }
        else
        {
            naui_input_state.dragging[i] = false;
        }
    }

    mgapp_set_cursor((mg_cursor)naui_input_state.current_cursor);
    naui_input_state.current_cursor = NAUI_CURSOR_ARROW;
}

bool naui_mouse_dragging(Naui_MouseButton button)
{
    return naui_input_state.dragging[(int)button];
}
