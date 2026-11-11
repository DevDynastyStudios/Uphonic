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

bool naui_key_released(Naui_Key key)
{
    return mgapp_key_released((mg_key)key);
}

const char *naui_key_name(Naui_Key key)
{
    switch (key)
    {
        case NAUI_KEY_BACKSPACE: return "Backspace";
        case NAUI_KEY_TAB: return "Tab";
        case NAUI_KEY_ENTER: return "Enter";
        case NAUI_KEY_SHIFT: return "Shift";
        case NAUI_KEY_CONTROL: return "Ctrl";
        case NAUI_KEY_ALT: return NAUI_KEY_ALT_NAME;
        case NAUI_KEY_PAUSE: return "Pause";
        case NAUI_KEY_CAPITAL: return "CapsLock";
        case NAUI_KEY_ESCAPE: return "Escape";
        case NAUI_KEY_SPACE: return "Space";
        case NAUI_KEY_PAGE_UP: return "PageUp";
        case NAUI_KEY_PAGE_DOWN: return "PageDown";
        case NAUI_KEY_END: return "End";
        case NAUI_KEY_HOME: return "Home";
        case NAUI_KEY_LEFT: return "Left";
        case NAUI_KEY_UP: return "Up";
        case NAUI_KEY_RIGHT: return "Right";
        case NAUI_KEY_DOWN: return "Down";
        case NAUI_KEY_PRINT_SCREEN: return "PrintScreen";
        case NAUI_KEY_INSERT: return "Insert";
        case NAUI_KEY_DELETE: return "Delete";

        case NAUI_KEY_0: return "0"; case NAUI_KEY_1: return "1"; case NAUI_KEY_2: return "2";
        case NAUI_KEY_3: return "3"; case NAUI_KEY_4: return "4"; case NAUI_KEY_5: return "5";
        case NAUI_KEY_6: return "6"; case NAUI_KEY_7: return "7"; case NAUI_KEY_8: return "8";
        case NAUI_KEY_9: return "9";

        case NAUI_KEY_A: return "A"; case NAUI_KEY_B: return "B"; case NAUI_KEY_C: return "C";
        case NAUI_KEY_D: return "D"; case NAUI_KEY_E: return "E"; case NAUI_KEY_F: return "F";
        case NAUI_KEY_G: return "G"; case NAUI_KEY_H: return "H"; case NAUI_KEY_I: return "I";
        case NAUI_KEY_J: return "J"; case NAUI_KEY_K: return "K"; case NAUI_KEY_L: return "L";
        case NAUI_KEY_M: return "M"; case NAUI_KEY_N: return "N"; case NAUI_KEY_O: return "O";
        case NAUI_KEY_P: return "P"; case NAUI_KEY_Q: return "Q"; case NAUI_KEY_R: return "R";
        case NAUI_KEY_S: return "S"; case NAUI_KEY_T: return "T"; case NAUI_KEY_U: return "U";
        case NAUI_KEY_V: return "V"; case NAUI_KEY_W: return "W"; case NAUI_KEY_X: return "X";
        case NAUI_KEY_Y: return "Y"; case NAUI_KEY_Z: return "Z";

        case NAUI_KEY_LSUPER: return "L" NAUI_KEY_SUPER_NAME;
        case NAUI_KEY_RSUPER: return "R" NAUI_KEY_SUPER_NAME;
        case NAUI_KEY_APPS: return "Menu";

        case NAUI_KEY_NUMPAD0: return "Num0"; case NAUI_KEY_NUMPAD1: return "Num1";
        case NAUI_KEY_NUMPAD2: return "Num2"; case NAUI_KEY_NUMPAD3: return "Num3";
        case NAUI_KEY_NUMPAD4: return "Num4"; case NAUI_KEY_NUMPAD5: return "Num5";
        case NAUI_KEY_NUMPAD6: return "Num6"; case NAUI_KEY_NUMPAD7: return "Num7";
        case NAUI_KEY_NUMPAD8: return "Num8"; case NAUI_KEY_NUMPAD9: return "Num9";
        case NAUI_KEY_MULTIPLY: return "Num*";
        case NAUI_KEY_ADD: return "Num+";
        case NAUI_KEY_SUBTRACT: return "Num-";
        case NAUI_KEY_DECIMAL: return "Num.";
        case NAUI_KEY_DIVIDE: return "Num/";
        case NAUI_KEY_NUMPAD_EQUAL: return "Num=";

        case NAUI_KEY_F1: return "F1"; case NAUI_KEY_F2: return "F2"; case NAUI_KEY_F3: return "F3";
        case NAUI_KEY_F4: return "F4"; case NAUI_KEY_F5: return "F5"; case NAUI_KEY_F6: return "F6";
        case NAUI_KEY_F7: return "F7"; case NAUI_KEY_F8: return "F8"; case NAUI_KEY_F9: return "F9";
        case NAUI_KEY_F10: return "F10"; case NAUI_KEY_F11: return "F11"; case NAUI_KEY_F12: return "F12";
        case NAUI_KEY_F13: return "F13"; case NAUI_KEY_F14: return "F14"; case NAUI_KEY_F15: return "F15";
        case NAUI_KEY_F16: return "F16"; case NAUI_KEY_F17: return "F17"; case NAUI_KEY_F18: return "F18";
        case NAUI_KEY_F19: return "F19"; case NAUI_KEY_F20: return "F20"; case NAUI_KEY_F21: return "F21";
        case NAUI_KEY_F22: return "F22"; case NAUI_KEY_F23: return "F23"; case NAUI_KEY_F24: return "F24";

        case NAUI_KEY_NUMLOCK: return "NumLock";
        case NAUI_KEY_SCROLL: return "ScrollLock";

        case NAUI_KEY_LSHIFT: return "LShift";
        case NAUI_KEY_RSHIFT: return "RShift";
        case NAUI_KEY_LCONTROL: return "LCtrl";
        case NAUI_KEY_RCONTROL: return "RCtrl";
        case NAUI_KEY_LALT: return "L" NAUI_KEY_ALT_NAME;
        case NAUI_KEY_RALT: return "R" NAUI_KEY_ALT_NAME;

        case NAUI_KEY_SEMICOLON: return ";";
        case NAUI_KEY_APOSTROPHE: return "'";
        case NAUI_KEY_EQUAL: return "=";
        case NAUI_KEY_COMMA: return ",";
        case NAUI_KEY_MINUS: return "-";
        case NAUI_KEY_PERIOD: return ".";
        case NAUI_KEY_SLASH: return "/";
        case NAUI_KEY_GRAVE: return "`";
        case NAUI_KEY_LBRACKET: return "[";
        case NAUI_KEY_BACKSLASH: return "\\";
        case NAUI_KEY_RBRACKET: return "]";

        default: return NULL;
    }
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
    int32_t drag_start_x[MG_MOUSE_BUTTON_MAX];
    int32_t drag_start_y[MG_MOUSE_BUTTON_MAX];
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
    #define DRAG_THRESHOLD 6

    for (int i = 0; i < MG_MOUSE_BUTTON_MAX; i++)
    {
        if (mgapp_mouse_pressed((mg_mouse_button)i))
        {
            naui_input_state.drag_start_x[i] = mgapp_mouse_x();
            naui_input_state.drag_start_y[i] = mgapp_mouse_y();
            naui_input_state.dragging[i] = false;
        }
        else if (mgapp_mouse_down((mg_mouse_button)i))
        {
            int32_t dx = mgapp_mouse_x() - naui_input_state.drag_start_x[i];
            int32_t dy = mgapp_mouse_y() - naui_input_state.drag_start_y[i];
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
