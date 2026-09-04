typedef uint8_t Naui_MouseButton;
enum 
{
    NAUI_MOUSE_LEFT,
    NAUI_MOUSE_RIGHT,
    NAUI_MOUSE_MIDDLE,
    NAUI_MOUSE_MAX
};

typedef uint8_t Naui_Key;
enum
{
    NAUI_KEY_BACKSPACE = 0x08,
    NAUI_KEY_ENTER = 0x0D,
    NAUI_KEY_TAB = 0x09,
    NAUI_KEY_SHIFT = 0x10,
    NAUI_KEY_CONTROL = 0x11,

    NAUI_KEY_PAUSE = 0x13,
    NAUI_KEY_CAPITAL = 0x14,

    NAUI_KEY_ESCAPE = 0x1B,

    NAUI_KEY_CONVERT = 0x1C,
    NAUI_KEY_NONCONVERT = 0x1D,
    NAUI_KEY_ACCEPT = 0x1E,
    NAUI_KEY_MODECHANGE = 0x1F,

    NAUI_KEY_SPACE = 0x20,
    NAUI_KEY_PAGE_UP = 0x21,
    NAUI_KEY_PAGE_DOWN = 0x22,
    NAUI_KEY_END = 0x23,
    NAUI_KEY_HOME = 0x24,
    NAUI_KEY_LEFT = 0x25,
    NAUI_KEY_UP = 0x26,
    NAUI_KEY_RIGHT = 0x27,
    NAUI_KEY_DOWN = 0x28,
    NAUI_KEY_SELECT = 0x29,
    NAUI_KEY_PRINT = 0x2A,
    NAUI_KEY_EXECUTE = 0x2B,
    NAUI_KEY_PRINT_SCREEN = 0x2C,
    NAUI_KEY_INSERT = 0x2D,
    NAUI_KEY_DELETE = 0x2E,
    NAUI_KEY_HELP = 0x2F,

    NAUI_KEY_0 = 0x30,
    NAUI_KEY_1 = 0x31,
    NAUI_KEY_2 = 0x32,
    NAUI_KEY_3 = 0x33,
    NAUI_KEY_4 = 0x34,
    NAUI_KEY_5 = 0x35,
    NAUI_KEY_6 = 0x36,
    NAUI_KEY_7 = 0x37,
    NAUI_KEY_8 = 0x38,
    NAUI_KEY_9 = 0x39,

    NAUI_KEY_A = 0x41,
    NAUI_KEY_B = 0x42,
    NAUI_KEY_C = 0x43,
    NAUI_KEY_D = 0x44,
    NAUI_KEY_E = 0x45,
    NAUI_KEY_F = 0x46,
    NAUI_KEY_G = 0x47,
    NAUI_KEY_H = 0x48,
    NAUI_KEY_I = 0x49,
    NAUI_KEY_J = 0x4A,
    NAUI_KEY_K = 0x4B,
    NAUI_KEY_L = 0x4C,
    NAUI_KEY_M = 0x4D,
    NAUI_KEY_N = 0x4E,
    NAUI_KEY_O = 0x4F,
    NAUI_KEY_P = 0x50,
    NAUI_KEY_Q = 0x51,
    NAUI_KEY_R = 0x52,
    NAUI_KEY_S = 0x53,
    NAUI_KEY_T = 0x54,
    NAUI_KEY_U = 0x55,
    NAUI_KEY_V = 0x56,
    NAUI_KEY_W = 0x57,
    NAUI_KEY_X = 0x58,
    NAUI_KEY_Y = 0x59,
    NAUI_KEY_Z = 0x5A,

    NAUI_KEY_LSUPER = 0x5B,
    NAUI_KEY_RSUPER = 0x5C,
    NAUI_KEY_APPS = 0x5D,

    NAUI_KEY_SLEEP = 0x5F,

    NAUI_KEY_NUMPAD0 = 0x60,
    NAUI_KEY_NUMPAD1 = 0x61,
    NAUI_KEY_NUMPAD2 = 0x62,
    NAUI_KEY_NUMPAD3 = 0x63,
    NAUI_KEY_NUMPAD4 = 0x64,
    NAUI_KEY_NUMPAD5 = 0x65,
    NAUI_KEY_NUMPAD6 = 0x66,
    NAUI_KEY_NUMPAD7 = 0x67,
    NAUI_KEY_NUMPAD8 = 0x68,
    NAUI_KEY_NUMPAD9 = 0x69,
    NAUI_KEY_MULTIPLY = 0x6A,
    NAUI_KEY_ADD = 0x6B,
    NAUI_KEY_SEPARATOR = 0x6C,
    NAUI_KEY_SUBTRACT = 0x6D,
    NAUI_KEY_DECIMAL = 0x6E,
    NAUI_KEY_DIVIDE = 0x6F,

    NAUI_KEY_F1 = 0x70,
    NAUI_KEY_F2 = 0x71,
    NAUI_KEY_F3 = 0x72,
    NAUI_KEY_F4 = 0x73,
    NAUI_KEY_F5 = 0x74,
    NAUI_KEY_F6 = 0x75,
    NAUI_KEY_F7 = 0x76,
    NAUI_KEY_F8 = 0x77,
    NAUI_KEY_F9 = 0x78,
    NAUI_KEY_F10 = 0x79,
    NAUI_KEY_F11 = 0x7A,
    NAUI_KEY_F12 = 0x7B,
    NAUI_KEY_F13 = 0x7C,
    NAUI_KEY_F14 = 0x7D,
    NAUI_KEY_F15 = 0x7E,
    NAUI_KEY_F16 = 0x7F,
    NAUI_KEY_F17 = 0x80,
    NAUI_KEY_F18 = 0x81,
    NAUI_KEY_F19 = 0x82,
    NAUI_KEY_F20 = 0x83,
    NAUI_KEY_F21 = 0x84,
    NAUI_KEY_F22 = 0x85,
    NAUI_KEY_F23 = 0x86,
    NAUI_KEY_F24 = 0x87,

    NAUI_KEY_NUMLOCK = 0x90,

    NAUI_KEY_SCROLL = 0x91,

    NAUI_KEY_NUMPAD_EQUAL = 0x92,

    NAUI_KEY_LSHIFT = 0xA0,
    NAUI_KEY_RSHIFT = 0xA1,
    NAUI_KEY_LCONTROL = 0xA2,
    NAUI_KEY_RCONTROL = 0xA3,
    NAUI_KEY_LALT = 0xA4,
    NAUI_KEY_RALT = 0xA5,

    NAUI_KEY_SEMICOLON = 0x3B,

    NAUI_KEY_APOSTROPHE = 0xDE,
    NAUI_KEY_QUOTE = NAUI_KEY_APOSTROPHE,
    NAUI_KEY_EQUAL = 0xBB,
    NAUI_KEY_COMMA = 0xBC,
    NAUI_KEY_MINUS = 0xBD,
    NAUI_KEY_PERIOD = 0xBE,
    NAUI_KEY_SLASH = 0xBF,

    NAUI_KEY_GRAVE = 0xC0,

    NAUI_KEY_LBRACKET = 0xDB,
    NAUI_KEY_PIPE = 0xDC,
    NAUI_KEY_BACKSLASH = NAUI_KEY_PIPE,
    NAUI_KEY_RBRACKET = 0xDD,

    NAUI_KEY_MAX = 0xFF
};

typedef uint8_t Naui_Cursor;
enum
{
    NAUI_CURSOR_ARROW,
    NAUI_CURSOR_IBEAM,
    NAUI_CURSOR_CROSSHAIR,
    NAUI_CURSOR_HAND,
    NAUI_CURSOR_RESIZE_NS,
    NAUI_CURSOR_RESIZE_EW,
    NAUI_CURSOR_RESIZE_NESW,
    NAUI_CURSOR_RESIZE_NWSE,
    NAUI_CURSOR_RESIZE_ALL,
    NAUI_CURSOR_NOT_ALLOWED,
    NAUI_CURSOR_HIDDEN,
    NAUI_CURSOR_MAX
};

NAUI_API bool naui_key_down(Naui_Key key);
NAUI_API bool naui_key_pressed(Naui_Key key);
NAUI_API uint32_t naui_app_char_pressed(void);

NAUI_API bool naui_mouse_down(Naui_MouseButton button);
NAUI_API bool naui_mouse_pressed(Naui_MouseButton button);
NAUI_API bool naui_mouse_released(Naui_MouseButton button);
NAUI_API bool naui_mouse_clicked(Naui_MouseButton button);
NAUI_API bool naui_mouse_double_clicked(Naui_MouseButton button);

NAUI_API int8_t naui_mouse_scroll_delta(void);
NAUI_API int32_t naui_mouse_x(void);
NAUI_API int32_t naui_mouse_y(void);
NAUI_API bool naui_mouse_dragging(Naui_MouseButton button);
NAUI_API void naui_set_cursor(Naui_Cursor cursor);
NAUI_API void naui_request_cursor(Naui_Cursor cursor);