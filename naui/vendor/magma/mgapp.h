#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef MGAPP_API
#if defined(_WIN32) && defined(MG_DLL) && (defined(MG_IMPL) || defined(MGAPP_IMPL))
#define MGAPP_API __declspec(dllexport)
#elif defined(_WIN32) && defined(MG_DLL)
#define MGAPP_API __declspec(dllimport)
#else
#define MGAPP_API extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t mg_mouse_button;
enum 
{
    MG_MOUSE_BUTTON_LEFT,
    MG_MOUSE_BUTTON_RIGHT,
    MG_MOUSE_BUTTON_MIDDLE,
    MG_MOUSE_BUTTON_MAX
};

typedef uint8_t mg_key;
enum
{
    MG_KEY_BACKSPACE = 0x08,
    MG_KEY_ENTER = 0x0D,
    MG_KEY_TAB = 0x09,
    MG_KEY_SHIFT = 0x10,
    MG_KEY_CONTROL = 0x11,

    MG_KEY_ALT = 0x12,
    MG_KEY_PAUSE = 0x13,
    MG_KEY_CAPITAL = 0x14,

    MG_KEY_ESCAPE = 0x1B,

    MG_KEY_CONVERT = 0x1C,
    MG_KEY_NONCONVERT = 0x1D,
    MG_KEY_ACCEPT = 0x1E,
    MG_KEY_MODECHANGE = 0x1F,

    MG_KEY_SPACE = 0x20,
    MG_KEY_PAGE_UP = 0x21,
    MG_KEY_PAGE_DOWN = 0x22,
    MG_KEY_END = 0x23,
    MG_KEY_HOME = 0x24,
    MG_KEY_LEFT = 0x25,
    MG_KEY_UP = 0x26,
    MG_KEY_RIGHT = 0x27,
    MG_KEY_DOWN = 0x28,
    MG_KEY_SELECT = 0x29,
    MG_KEY_PRINT = 0x2A,
    MG_KEY_EXECUTE = 0x2B,
    MG_KEY_PRINT_SCREEN = 0x2C,
    MG_KEY_INSERT = 0x2D,
    MG_KEY_DELETE = 0x2E,
    MG_KEY_HELP = 0x2F,

    MG_KEY_0 = 0x30,
    MG_KEY_1 = 0x31,
    MG_KEY_2 = 0x32,
    MG_KEY_3 = 0x33,
    MG_KEY_4 = 0x34,
    MG_KEY_5 = 0x35,
    MG_KEY_6 = 0x36,
    MG_KEY_7 = 0x37,
    MG_KEY_8 = 0x38,
    MG_KEY_9 = 0x39,

    MG_KEY_A = 0x41,
    MG_KEY_B = 0x42,
    MG_KEY_C = 0x43,
    MG_KEY_D = 0x44,
    MG_KEY_E = 0x45,
    MG_KEY_F = 0x46,
    MG_KEY_G = 0x47,
    MG_KEY_H = 0x48,
    MG_KEY_I = 0x49,
    MG_KEY_J = 0x4A,
    MG_KEY_K = 0x4B,
    MG_KEY_L = 0x4C,
    MG_KEY_M = 0x4D,
    MG_KEY_N = 0x4E,
    MG_KEY_O = 0x4F,
    MG_KEY_P = 0x50,
    MG_KEY_Q = 0x51,
    MG_KEY_R = 0x52,
    MG_KEY_S = 0x53,
    MG_KEY_T = 0x54,
    MG_KEY_U = 0x55,
    MG_KEY_V = 0x56,
    MG_KEY_W = 0x57,
    MG_KEY_X = 0x58,
    MG_KEY_Y = 0x59,
    MG_KEY_Z = 0x5A,

    MG_KEY_LSUPER = 0x5B,
    MG_KEY_RSUPER = 0x5C,
    MG_KEY_APPS = 0x5D,

    MG_KEY_SLEEP = 0x5F,

    MG_KEY_NUMPAD0 = 0x60,
    MG_KEY_NUMPAD1 = 0x61,
    MG_KEY_NUMPAD2 = 0x62,
    MG_KEY_NUMPAD3 = 0x63,
    MG_KEY_NUMPAD4 = 0x64,
    MG_KEY_NUMPAD5 = 0x65,
    MG_KEY_NUMPAD6 = 0x66,
    MG_KEY_NUMPAD7 = 0x67,
    MG_KEY_NUMPAD8 = 0x68,
    MG_KEY_NUMPAD9 = 0x69,
    MG_KEY_MULTIPLY = 0x6A,
    MG_KEY_ADD = 0x6B,
    MG_KEY_SEPARATOR = 0x6C,
    MG_KEY_SUBTRACT = 0x6D,
    MG_KEY_DECIMAL = 0x6E,
    MG_KEY_DIVIDE = 0x6F,

    MG_KEY_F1 = 0x70,
    MG_KEY_F2 = 0x71,
    MG_KEY_F3 = 0x72,
    MG_KEY_F4 = 0x73,
    MG_KEY_F5 = 0x74,
    MG_KEY_F6 = 0x75,
    MG_KEY_F7 = 0x76,
    MG_KEY_F8 = 0x77,
    MG_KEY_F9 = 0x78,
    MG_KEY_F10 = 0x79,
    MG_KEY_F11 = 0x7A,
    MG_KEY_F12 = 0x7B,
    MG_KEY_F13 = 0x7C,
    MG_KEY_F14 = 0x7D,
    MG_KEY_F15 = 0x7E,
    MG_KEY_F16 = 0x7F,
    MG_KEY_F17 = 0x80,
    MG_KEY_F18 = 0x81,
    MG_KEY_F19 = 0x82,
    MG_KEY_F20 = 0x83,
    MG_KEY_F21 = 0x84,
    MG_KEY_F22 = 0x85,
    MG_KEY_F23 = 0x86,
    MG_KEY_F24 = 0x87,

    MG_KEY_NUMLOCK = 0x90,

    MG_KEY_SCROLL = 0x91,

    MG_KEY_NUMPAD_EQUAL = 0x92,

    MG_KEY_LSHIFT = 0xA0,
    MG_KEY_RSHIFT = 0xA1,
    MG_KEY_LCONTROL = 0xA2,
    MG_KEY_RCONTROL = 0xA3,
    MG_KEY_LALT = 0xA4,
    MG_KEY_RALT = 0xA5,

    MG_KEY_SEMICOLON = 0x3B,

    MG_KEY_APOSTROPHE = 0xDE,
    MG_KEY_QUOTE = MG_KEY_APOSTROPHE,
    MG_KEY_EQUAL = 0xBB,
    MG_KEY_COMMA = 0xBC,
    MG_KEY_MINUS = 0xBD,
    MG_KEY_PERIOD = 0xBE,
    MG_KEY_SLASH = 0xBF,

    MG_KEY_GRAVE = 0xC0,

    MG_KEY_LBRACKET = 0xDB,
    MG_KEY_PIPE = 0xDC,
    MG_KEY_BACKSLASH = MG_KEY_PIPE,
    MG_KEY_RBRACKET = 0xDD,

    MG_KEY_MAX = 0xFF
};

typedef uint8_t mg_cursor;
enum
{
    MG_CURSOR_ARROW,
    MG_CURSOR_IBEAM,
    MG_CURSOR_CROSSHAIR,
    MG_CURSOR_HAND,
    MG_CURSOR_RESIZE_NS,
    MG_CURSOR_RESIZE_EW,
    MG_CURSOR_RESIZE_NESW,
    MG_CURSOR_RESIZE_NWSE,
    MG_CURSOR_RESIZE_ALL,
    MG_CURSOR_NOT_ALLOWED,
    MG_CURSOR_HIDDEN,
    MG_CURSOR_MAX
};

typedef uint8_t mgapp_event_type;
enum
{
    MGAPP_EVENT_RESIZE,
    MGAPP_EVENT_KEY_DOWN,
    MGAPP_EVENT_KEY_UP,
    MGAPP_EVENT_CHAR,
    MGAPP_EVENT_MOUSE_DOWN,
    MGAPP_EVENT_MOUSE_UP,
    MGAPP_EVENT_MOUSE_MOVE,
    MGAPP_EVENT_MOUSE_SCROLL,
    MGAPP_EVENT_FILE_DROP,
    MGAPP_EVENT_WINDOW_CLOSE
};

typedef struct
{
    union
    {
        struct
        {
            int32_t width, height;
        }
        window;
        struct
        {
            int32_t x, y;
            int32_t scroll;
            mg_mouse_button button;
        }
        mouse;
        struct
        {
            const char **paths;
            int32_t path_count;
        }
        file_drop;
        uint32_t codepoint;
        mg_key key;
    };

    mgapp_event_type type;
}
mgapp_event;

typedef uint32_t mgapp_flags;
enum
{
    MGAPP_FLAG_NONE = 0,
    MGAPP_FLAG_NO_TITLEBAR = 1 << 0,
    MGAPP_FLAG_NO_RESIZE = 1 << 1,
    MGAPP_FLAG_NO_MINIMIZE = 1 << 2,
    MGAPP_FLAG_NO_MAXIMIZE = 1 << 3,
    MGAPP_FLAG_HIDE_WINDOW = 1 << 4,
    MGAPP_FLAG_ENABLE_FILE_DROPS = 1 << 5
};

typedef struct
{
    void (*start)(void);
    void (*end)(void);
    void (*update)(void);
    void (*event)(const mgapp_event *event);
}
mgapp_events;

typedef struct
{
    const char *title;
    mgapp_flags flags;
    uint32_t width, height;
    mgapp_events events;
}
mgapp_init_info;

#ifndef MGAPP_MAX_CODEPOINTS
    #define MGAPP_MAX_CODEPOINTS 64
#endif

MGAPP_API int32_t mgapp_run(const mgapp_init_info *info);
MGAPP_API void mgapp_close(void);
MGAPP_API void mgapp_show(bool value);
MGAPP_API void mgapp_minimize(void);
MGAPP_API void mgapp_maximize(void);
MGAPP_API void mgapp_restore(void);
MGAPP_API bool mgapp_maximized(void);

MGAPP_API float mgapp_time(void);
MGAPP_API float mgapp_frame_time(void);
MGAPP_API float mgapp_delta_time(void);

MGAPP_API int32_t mgapp_width(void);
MGAPP_API int32_t mgapp_height(void);

MGAPP_API void mgapp_set_cursor(mg_cursor cursor);

MGAPP_API bool mgapp_key_down(mg_key key);
MGAPP_API bool mgapp_key_pressed(mg_key key);
MGAPP_API bool mgapp_key_released(mg_key key);

MGAPP_API uint32_t mgapp_codepoint(void);

MGAPP_API bool mgapp_mouse_down(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_pressed(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_released(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_clicked(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_double_clicked(mg_mouse_button button);

MGAPP_API int8_t mgapp_mouse_scroll_delta(void);
MGAPP_API int32_t mgapp_mouse_x(void);
MGAPP_API int32_t mgapp_mouse_y(void);

MGAPP_API uint32_t mgapp_dpi(void);
MGAPP_API float mgapp_dpi_scale(void);

MGAPP_API void mgapp_set_caption_area(int32_t x, int32_t y, int32_t width, int32_t height);

MGAPP_API void *mgapp_primary_handle(void);
MGAPP_API void *mgapp_secondary_handle(void);

#ifdef __cplusplus
}
#endif

#if defined(MG_IMPL) || defined(MGAPP_IMPL)

#include <stdio.h>
#include <string.h>

#define MGAPP_ERROR(msg) \
    fprintf(stderr, "[MGAPP]: Error: %s\nLine: %d\n", msg, __LINE__);

#if !defined(MGAPP_NO_ENTRY)
int main(void)
{
    extern mgapp_init_info mgapp_main(void);
    const mgapp_init_info info = mgapp_main();
    mgapp_run(&info);
}
#endif

#ifndef MGAPP_CLICK_TIME_THRESHOLD
    #define MGAPP_CLICK_TIME_THRESHOLD 0.5f
#endif

#ifndef MGAPP_CLICK_MOVE_THRESHOLD
    #define MGAPP_CLICK_MOVE_THRESHOLD 4
#endif

#ifndef MGAPP_DOUBLE_CLICK_TIME_THRESHOLD
    #define MGAPP_DOUBLE_CLICK_TIME_THRESHOLD 0.4f
#endif

#ifndef MGAPP_DOUBLE_CLICK_MOVE_THRESHOLD
    #define MGAPP_DOUBLE_CLICK_MOVE_THRESHOLD 6
#endif

typedef struct
{
    struct
    {
        bool keys[MG_KEY_MAX];
    }
    keyboard;

    struct
    {
        float down_time[4];
        float last_click_time[4];
        int16_t down_x[4];
        int16_t down_y[4];
        int16_t last_click_x[4];
        int16_t last_click_y[4];

        int16_t x;
        int16_t y;
        int8_t delta;

        bool buttons[4];
    }
    mouse;

    struct
    {
        struct
        {
            bool keys_pressed[MG_KEY_MAX];
            bool keys_released[MG_KEY_MAX];
        }
        keyboard;

        struct
        {
            bool buttons_pressed[4];
            bool buttons_released[4];
            bool buttons_clicked[4];
            bool buttons_double_clicked[4];
        }
        mouse;
    }
    clear;

    struct
    {
        uint32_t codepoints[MGAPP_MAX_CODEPOINTS];
        uint32_t count;
        uint32_t read_index;
    }
    text;
}
mgapp_input_state;

#if defined(__EMSCRIPTEN__)

#include <emscripten.h>
#include <emscripten/html5.h>

#ifndef MGAPP_EMSCRIPTEN_CANVAS
    #define MGAPP_EMSCRIPTEN_CANVAS "#canvas"
#endif

typedef struct
{
    double start_time;
}
mgapp_emscripten_state;

#elif defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#pragma comment(lib, "user32")
#pragma comment(lib, "shell32")

typedef struct
{
    HWND hwnd;
    HINSTANCE hinstance;

    WNDPROC original_proc;

    HCURSOR cursor_table[MG_CURSOR_MAX];

    float clock_frequency;
    LARGE_INTEGER start_time;
}
mgapp_win32_state;

#elif defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
    Window window;
    Display *display;
    int32_t screen;

    Atom wm_delete_window;
    Atom net_wm_state;
    Atom net_wm_state_maximized_horz;
    Atom net_wm_state_maximized_vert;
    Atom motif_wm_hints;
    Atom net_wm_moveresize;
    Atom net_wm_allowed_actions;
    Atom net_wm_action_resize;
    Atom net_wm_action_minimize;
    Atom net_wm_action_maximize_horz;
    Atom net_wm_action_maximize_vert;

    Atom xdnd_aware;
    Atom xdnd_enter;
    Atom xdnd_position;
    Atom xdnd_status;
    Atom xdnd_leave;
    Atom xdnd_drop;
    Atom xdnd_finished;
    Atom xdnd_selection;
    Atom xdnd_type_list;
    Atom xdnd_action_copy;
    Atom uri_list;

    Window xdnd_source;
    bool xdnd_pending;

    Cursor cursor_table[MG_CURSOR_MAX];
}
mgapp_xlib_state;

typedef struct
{
    
}
mgapp_wayland_state;

typedef uint8_t mgapp_linux_state_type;
enum
{
    MGAPP_LINUX_STATE_XLIB,
    MGAPP_LINUX_STATE_WAYLAND
};

typedef struct
{
    union
    {
        mgapp_xlib_state xlib;
        mgapp_wayland_state wayland;
    };
    mgapp_linux_state_type type;

    int32_t (*run)(const mgapp_init_info*);
    void *(*primary_handle)(void);
    void *(*secondary_handle)(void);
    void (*maximize)(void);
    void (*minimize)(void);
    void (*restore)(void);
    void (*show)(bool);
    void (*close)(void);
    void (*set_cursor)(mg_cursor);

    bool maximized;
}
mgapp_linux_state;

#endif

typedef struct
{
#if defined(__EMSCRIPTEN__)
    mgapp_emscripten_state emscripten;
#elif defined(_WIN32)
    mgapp_win32_state win32;
#elif defined(__linux__)
    mgapp_linux_state _linux;
#endif
    mgapp_input_state input;

    struct
    {
        int32_t x, y, width, height;
    }
    caption;

    mgapp_events events;
    mgapp_flags flags;

    int32_t window_width, window_height;
    float frame_time, delta_time;

    float dpi_scale;
    uint32_t dpi;

    mg_cursor current_cursor;

    bool running;
}
mgapp_core_state;
static mgapp_core_state mgapp_state;

static inline void mgapp_call_event(const mgapp_event *event)
{
    if (mgapp_state.events.event)
        mgapp_state.events.event(event);
}

static inline void mgapp_input_process_key(mg_key key, bool pressed)
{
    if (pressed && !mgapp_state.input.keyboard.keys[key])
        mgapp_state.input.clear.keyboard.keys_pressed[key] = true;

    if (!pressed && mgapp_state.input.keyboard.keys[key])
        mgapp_state.input.clear.keyboard.keys_released[key] = true;

    mgapp_state.input.keyboard.keys[key] = pressed;
}

static inline void mgapp_input_process_char(uint32_t codepoint)
{
    mgapp_input_state *input_state = &mgapp_state.input;
    if (input_state->text.count < MGAPP_MAX_CODEPOINTS)
        input_state->text.codepoints[input_state->text.count++] = codepoint;
}

static inline void mgapp_input_process_mouse_button(mg_mouse_button button, bool pressed, float time_now)
{
    mgapp_input_state *input_state = &mgapp_state.input;
    if (pressed && !input_state->mouse.buttons[button])
    {
        input_state->clear.mouse.buttons_pressed[button] = true;
        input_state->mouse.down_time[button] = time_now;
        input_state->mouse.down_x[button] = input_state->mouse.x;
        input_state->mouse.down_y[button] = input_state->mouse.y;
    }

    if (!pressed && input_state->mouse.buttons[button])
    {
        input_state->clear.mouse.buttons_released[button] = true;

        float held = time_now - input_state->mouse.down_time[button];
        int16_t dx = input_state->mouse.x - input_state->mouse.down_x[button];
        int16_t dy = input_state->mouse.y - input_state->mouse.down_y[button];
        int32_t move_sq = (int32_t)dx * dx + (int32_t)dy * dy;

        if (held <= MGAPP_CLICK_TIME_THRESHOLD &&
            move_sq <= (MGAPP_CLICK_MOVE_THRESHOLD * MGAPP_CLICK_MOVE_THRESHOLD))
        {
            input_state->clear.mouse.buttons_clicked[button] = true;

            float since_last = time_now - input_state->mouse.last_click_time[button];
            int16_t ldx = input_state->mouse.x - input_state->mouse.last_click_x[button];
            int16_t ldy = input_state->mouse.y - input_state->mouse.last_click_y[button];
            int32_t last_move_sq = (int32_t)ldx * ldx + (int32_t)ldy * ldy;

            if (since_last <= MGAPP_DOUBLE_CLICK_TIME_THRESHOLD &&
                last_move_sq <= (MGAPP_DOUBLE_CLICK_MOVE_THRESHOLD * MGAPP_DOUBLE_CLICK_MOVE_THRESHOLD))
            {
                input_state->clear.mouse.buttons_double_clicked[button] = true;
                input_state->mouse.last_click_time[button] = -1000.0f;
            }
            else
            {
                input_state->mouse.last_click_time[button] = time_now;
                input_state->mouse.last_click_x[button] = input_state->mouse.x;
                input_state->mouse.last_click_y[button] = input_state->mouse.y;
            }
        }
    }

    mgapp_state.input.mouse.buttons[button] = pressed;
}

static inline void mgapp_input_frame(void)
{
    mgapp_state.input.mouse.delta = 0;
    mgapp_state.input.text.count = 0;
    mgapp_state.input.text.read_index = 0;
    memset(&mgapp_state.input.clear, 0, sizeof(mgapp_state.input.clear));
}

bool mgapp_key_down(mg_key key)
{
    return mgapp_state.input.keyboard.keys[key];
}

bool mgapp_key_pressed(mg_key key)
{
    return mgapp_state.input.clear.keyboard.keys_pressed[key];
}

bool mgapp_key_released(mg_key key)
{
    return mgapp_state.input.clear.keyboard.keys_released[key];
}

uint32_t mgapp_codepoint(void)
{
    mgapp_input_state *input_state = &mgapp_state.input;

    if (input_state->text.read_index >= input_state->text.count)
        return 0;

    return input_state->text.codepoints[input_state->text.read_index++];
}

bool mgapp_mouse_down(mg_mouse_button button)
{
    return mgapp_state.input.mouse.buttons[button];
}

bool mgapp_mouse_pressed(mg_mouse_button button)
{
    return mgapp_state.input.clear.mouse.buttons_pressed[button];
}

bool mgapp_mouse_released(mg_mouse_button button)
{
    return mgapp_state.input.clear.mouse.buttons_released[button];
}

bool mgapp_mouse_clicked(mg_mouse_button button)
{
    return mgapp_state.input.clear.mouse.buttons_clicked[button];
}

bool mgapp_mouse_double_clicked(mg_mouse_button button)
{
    return mgapp_state.input.clear.mouse.buttons_double_clicked[button];
}

int8_t mgapp_mouse_scroll_delta(void)
{
    return mgapp_state.input.mouse.delta;
}

int32_t mgapp_mouse_x(void)
{
    return mgapp_state.input.mouse.x;
}

int32_t mgapp_mouse_y(void)
{
    return mgapp_state.input.mouse.y;
}

float mgapp_frame_time(void)
{
    return mgapp_state.frame_time;
}
 
float mgapp_delta_time(void)
{
    return mgapp_state.delta_time;
}
 
int32_t mgapp_width(void)
{
    return mgapp_state.window_width;
}
 
int32_t mgapp_height(void)
{
    return mgapp_state.window_height;
}

uint32_t mgapp_dpi(void)
{
    return mgapp_state.dpi;
}

float mgapp_dpi_scale(void)
{
    return mgapp_state.dpi_scale;
}

void mgapp_set_caption_area(int32_t x, int32_t y, int32_t width, int32_t height)
{
    mgapp_state.caption.x = x;
    mgapp_state.caption.y = y;
    mgapp_state.caption.width = width;
    mgapp_state.caption.height = height;
}

#if defined(__EMSCRIPTEN__)

static inline mg_key mgapp_emscripten_translate_key(const EmscriptenKeyboardEvent *e)
{
    const char *code = e->code;

    if (strncmp(code, "Key", 3) == 0 && code[3] >= 'A' && code[3] <= 'Z')
        return (mg_key)(MG_KEY_A + (code[3] - 'A'));
    if (strncmp(code, "Digit", 5) == 0 && code[5] >= '0' && code[5] <= '9')
        return (mg_key)(MG_KEY_0 + (code[5] - '0'));
    if (strncmp(code, "Numpad", 6) == 0 && code[6] >= '0' && code[6] <= '9')
        return (mg_key)(MG_KEY_NUMPAD0 + (code[6] - '0'));
    if (code[0] == 'F' && code[1] >= '1' && code[1] <= '9')
    {
        int32_t n = atoi(code + 1);
        if (n >= 1 && n <= 24)
            return (mg_key)(MG_KEY_F1 + (n - 1));
    }

    if (strcmp(code, "Backspace") == 0)      return MG_KEY_BACKSPACE;
    if (strcmp(code, "Enter") == 0)          return MG_KEY_ENTER;
    if (strcmp(code, "NumpadEnter") == 0)    return MG_KEY_ENTER;
    if (strcmp(code, "Tab") == 0)            return MG_KEY_TAB;
    if (strcmp(code, "Pause") == 0)          return MG_KEY_PAUSE;
    if (strcmp(code, "CapsLock") == 0)       return MG_KEY_CAPITAL;
    if (strcmp(code, "Escape") == 0)         return MG_KEY_ESCAPE;
    if (strcmp(code, "Space") == 0)          return MG_KEY_SPACE;
    if (strcmp(code, "PageUp") == 0)         return MG_KEY_PAGE_UP;
    if (strcmp(code, "PageDown") == 0)       return MG_KEY_PAGE_DOWN;
    if (strcmp(code, "End") == 0)            return MG_KEY_END;
    if (strcmp(code, "Home") == 0)           return MG_KEY_HOME;
    if (strcmp(code, "ArrowLeft") == 0)      return MG_KEY_LEFT;
    if (strcmp(code, "ArrowUp") == 0)        return MG_KEY_UP;
    if (strcmp(code, "ArrowRight") == 0)     return MG_KEY_RIGHT;
    if (strcmp(code, "ArrowDown") == 0)      return MG_KEY_DOWN;
    if (strcmp(code, "PrintScreen") == 0)    return MG_KEY_PRINT_SCREEN;
    if (strcmp(code, "Insert") == 0)         return MG_KEY_INSERT;
    if (strcmp(code, "Delete") == 0)         return MG_KEY_DELETE;
    if (strcmp(code, "Help") == 0)           return MG_KEY_HELP;

    if (strcmp(code, "MetaLeft") == 0)       return MG_KEY_LSUPER;
    if (strcmp(code, "MetaRight") == 0)      return MG_KEY_RSUPER;
    if (strcmp(code, "ContextMenu") == 0)    return MG_KEY_APPS;

    if (strcmp(code, "NumpadMultiply") == 0) return MG_KEY_MULTIPLY;
    if (strcmp(code, "NumpadAdd") == 0)      return MG_KEY_ADD;
    if (strcmp(code, "NumpadSubtract") == 0) return MG_KEY_SUBTRACT;
    if (strcmp(code, "NumpadDecimal") == 0)  return MG_KEY_DECIMAL;
    if (strcmp(code, "NumpadDivide") == 0)   return MG_KEY_DIVIDE;
    if (strcmp(code, "NumpadEqual") == 0)    return MG_KEY_NUMPAD_EQUAL;

    if (strcmp(code, "NumLock") == 0)        return MG_KEY_NUMLOCK;
    if (strcmp(code, "ScrollLock") == 0)     return MG_KEY_SCROLL;

    if (strcmp(code, "ShiftLeft") == 0)      return MG_KEY_LSHIFT;
    if (strcmp(code, "ShiftRight") == 0)     return MG_KEY_RSHIFT;
    if (strcmp(code, "ControlLeft") == 0)    return MG_KEY_LCONTROL;
    if (strcmp(code, "ControlRight") == 0)   return MG_KEY_RCONTROL;
    if (strcmp(code, "AltLeft") == 0)        return MG_KEY_LALT;
    if (strcmp(code, "AltRight") == 0)       return MG_KEY_RALT;

    if (strcmp(code, "Semicolon") == 0)      return MG_KEY_SEMICOLON;
    if (strcmp(code, "Quote") == 0)          return MG_KEY_APOSTROPHE;
    if (strcmp(code, "Equal") == 0)          return MG_KEY_EQUAL;
    if (strcmp(code, "Comma") == 0)          return MG_KEY_COMMA;
    if (strcmp(code, "Minus") == 0)          return MG_KEY_MINUS;
    if (strcmp(code, "Period") == 0)         return MG_KEY_PERIOD;
    if (strcmp(code, "Slash") == 0)          return MG_KEY_SLASH;
    if (strcmp(code, "Backquote") == 0)      return MG_KEY_GRAVE;
    if (strcmp(code, "BracketLeft") == 0)    return MG_KEY_LBRACKET;
    if (strcmp(code, "Backslash") == 0)      return MG_KEY_PIPE;
    if (strcmp(code, "BracketRight") == 0)   return MG_KEY_RBRACKET;

    return MG_KEY_MAX;
}

static inline mg_mouse_button mgapp_emscripten_translate_button(unsigned short button)
{
    switch (button)
    {
        case 0: return MG_MOUSE_BUTTON_LEFT;
        case 1: return MG_MOUSE_BUTTON_MIDDLE;
        case 2: return MG_MOUSE_BUTTON_RIGHT;
        default: return MG_MOUSE_BUTTON_MAX;
    }
}

static EM_BOOL mgapp_emscripten_key_callback(int32_t event_type, const EmscriptenKeyboardEvent *e, void *user_data)
{
    bool pressed = event_type == EMSCRIPTEN_EVENT_KEYDOWN;
    mg_key key = mgapp_emscripten_translate_key(e);

    if (key != MG_KEY_MAX)
    {
        mgapp_input_process_key(key, pressed);
        mgapp_event event = {
            .key = key,
            .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_KEY_DOWN : MGAPP_EVENT_KEY_UP)
        };
        mgapp_call_event(&event);
    }

    if (pressed && e->key[0] != '\0' && e->key[1] == '\0')
    {
        unsigned char c = (unsigned char)e->key[0];
        if (c >= 0x20)
        {
            mgapp_input_process_char((uint32_t)c);
            mgapp_event event = {
                .codepoint = (uint32_t)c,
                .type = MGAPP_EVENT_CHAR
            };
            mgapp_call_event(&event);
        }
    }

    return EM_TRUE;
}

static EM_BOOL mgapp_emscripten_mouse_button_callback(int32_t event_type, const EmscriptenMouseEvent *e, void *user_data)
{
    mg_mouse_button button = mgapp_emscripten_translate_button(e->button);
    if (button == MG_MOUSE_BUTTON_MAX)
        return EM_FALSE;

    bool pressed = event_type == EMSCRIPTEN_EVENT_MOUSEDOWN;
    mgapp_input_process_mouse_button(button, pressed, mgapp_state.frame_time);
    mgapp_event event = {
        .mouse = { .button = button },
        .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_MOUSE_DOWN : MGAPP_EVENT_MOUSE_UP)
    };
    mgapp_call_event(&event);
    return EM_TRUE;
}

static EM_BOOL mgapp_emscripten_mouse_move_callback(int32_t event_type, const EmscriptenMouseEvent *e, void *user_data)
{
    mgapp_state.input.mouse.x = (int16_t)e->targetX;
    mgapp_state.input.mouse.y = (int16_t)e->targetY;
    mgapp_event event = {
        .mouse = {
            .x = e->targetX,
            .y = e->targetY
        },
        .type = MGAPP_EVENT_MOUSE_MOVE
    };
    mgapp_call_event(&event);
    return EM_TRUE;
}

static EM_BOOL mgapp_emscripten_wheel_callback(int32_t event_type, const EmscriptenWheelEvent *e, void *user_data)
{
    int8_t delta = (e->deltaY < 0) ? 1 : -1;
    mgapp_state.input.mouse.delta = delta;
    mgapp_event event = {
        .mouse = { .scroll = delta },
        .type = MGAPP_EVENT_MOUSE_SCROLL
    };
    mgapp_call_event(&event);
    return EM_TRUE;
}

static EM_BOOL mgapp_emscripten_resize_callback(int32_t event_type, const EmscriptenUiEvent *e, void *user_data)
{
    int32_t w, h;
    emscripten_get_canvas_element_size(MGAPP_EMSCRIPTEN_CANVAS, &w, &h);
    mgapp_state.window_width = w;
    mgapp_state.window_height = h;
    mgapp_event event = {
        .window = { .width = w, .height = h },
        .type = MGAPP_EVENT_RESIZE
    };
    mgapp_call_event(&event);
    return EM_TRUE;
}

static void mgapp_emscripten_main_loop(void)
{
    const float new_time = mgapp_time();
    mgapp_state.delta_time = new_time - mgapp_state.frame_time;
    mgapp_state.frame_time = new_time;

    if (mgapp_state.events.update)
        mgapp_state.events.update();

    mgapp_input_frame();

    if (!mgapp_state.running)
    {
        mgapp_event event = { .type = MGAPP_EVENT_WINDOW_CLOSE };
        mgapp_call_event(&event);
        emscripten_cancel_main_loop();
        if (mgapp_state.events.end)
            mgapp_state.events.end();
    }
}

int32_t mgapp_run(const mgapp_init_info *info)
{
    mgapp_state.emscripten.start_time = emscripten_get_now();

    int32_t width = info->width ? (int32_t)info->width : 800;
    int32_t height = info->height ? (int32_t)info->height : 600;

    emscripten_set_canvas_element_size(MGAPP_EMSCRIPTEN_CANVAS, width, height);
    mgapp_state.window_width = width;
    mgapp_state.window_height = height;

    double css_w, css_h;
    emscripten_get_element_css_size(MGAPP_EMSCRIPTEN_CANVAS, &css_w, &css_h);
    mgapp_state.dpi_scale = (float)emscripten_get_device_pixel_ratio();
    mgapp_state.dpi = (uint32_t)(96.0f * mgapp_state.dpi_scale);

    mgapp_state.events = info->events;
    mgapp_state.flags = info->flags;

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, mgapp_emscripten_key_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, mgapp_emscripten_key_callback);

    emscripten_set_mousedown_callback(MGAPP_EMSCRIPTEN_CANVAS, NULL, EM_TRUE, mgapp_emscripten_mouse_button_callback);
    emscripten_set_mouseup_callback(MGAPP_EMSCRIPTEN_CANVAS, NULL, EM_TRUE, mgapp_emscripten_mouse_button_callback);
    emscripten_set_mousemove_callback(MGAPP_EMSCRIPTEN_CANVAS, NULL, EM_TRUE, mgapp_emscripten_mouse_move_callback);
    emscripten_set_wheel_callback(MGAPP_EMSCRIPTEN_CANVAS, NULL, EM_TRUE, mgapp_emscripten_wheel_callback);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, mgapp_emscripten_resize_callback);

    if (info->events.start)
        info->events.start();

    mgapp_state.running = true;
    mgapp_state.frame_time = mgapp_time();

    emscripten_set_main_loop(mgapp_emscripten_main_loop, 0, 1);

    return 0;
}

void mgapp_close(void)
{
    mgapp_state.running = false;
}

void mgapp_show(bool value)
{
    EM_ASM({
        var el = document.querySelector(UTF8ToString($0));
        if (el) el.style.display = $1 ? "" : "none";
    }, MGAPP_EMSCRIPTEN_CANVAS, value ? 1 : 0);
}

void mgapp_minimize(void)
{
    
}

void mgapp_maximize(void)
{
    EM_ASM({
        var el = document.querySelector(UTF8ToString($0));
        if (el && el.requestFullscreen) el.requestFullscreen();
    }, MGAPP_EMSCRIPTEN_CANVAS);
    mgapp_state._unused_placeholder = 0;
}

void mgapp_restore(void)
{
    EM_ASM({
        if (document.exitFullscreen) document.exitFullscreen();
    });
}

bool mgapp_maximized(void)
{
    return EM_ASM_INT({ return !!document.fullscreenElement; });
}

void mgapp_set_cursor(mg_cursor cursor)
{
    static const char *names[MG_CURSOR_MAX] = {
        "default", "text", "crosshair", "pointer",
        "ns-resize", "ew-resize", "nesw-resize", "nwse-resize",
        "move", "not-allowed", "none"
    };

    if (mgapp_state.current_cursor == cursor)
        return;

    EM_ASM({
        var el = document.querySelector(UTF8ToString($0));
        if (el) el.style.cursor = UTF8ToString($1);
    }, MGAPP_EMSCRIPTEN_CANVAS, names[cursor]);

    mgapp_state.current_cursor = cursor;
}

float mgapp_time(void)
{
    return (float)((emscripten_get_now() - mgapp_state.emscripten.start_time) / 1000.0);
}

void *mgapp_primary_handle(void)
{
    return (void*)MGAPP_EMSCRIPTEN_CANVAS;
}

void *mgapp_secondary_handle(void)
{
    return NULL;
}

#elif defined(_WIN32)

static inline void mgapp_win32_setup_clock(mgapp_win32_state *win32_state)
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    win32_state->clock_frequency = 1.0f / (float)frequency.QuadPart;
    QueryPerformanceCounter(&win32_state->start_time);
}

static inline void mgapp_win32_load_cursors(mgapp_win32_state *win32_state)
{
    win32_state->cursor_table[MG_CURSOR_ARROW]        = LoadCursorW(NULL, IDC_ARROW);
    win32_state->cursor_table[MG_CURSOR_IBEAM]        = LoadCursorW(NULL, IDC_IBEAM);
    win32_state->cursor_table[MG_CURSOR_CROSSHAIR]    = LoadCursorW(NULL, IDC_CROSS);
    win32_state->cursor_table[MG_CURSOR_HAND]         = LoadCursorW(NULL, IDC_HAND);
    win32_state->cursor_table[MG_CURSOR_RESIZE_NS]    = LoadCursorW(NULL, IDC_SIZENS);
    win32_state->cursor_table[MG_CURSOR_RESIZE_EW]    = LoadCursorW(NULL, IDC_SIZEWE);
    win32_state->cursor_table[MG_CURSOR_RESIZE_NESW]  = LoadCursorW(NULL, IDC_SIZENESW);
    win32_state->cursor_table[MG_CURSOR_RESIZE_NWSE]  = LoadCursorW(NULL, IDC_SIZENWSE);
    win32_state->cursor_table[MG_CURSOR_RESIZE_ALL]   = LoadCursorW(NULL, IDC_SIZEALL);
    win32_state->cursor_table[MG_CURSOR_NOT_ALLOWED]  = LoadCursorW(NULL, IDC_NO);
    win32_state->cursor_table[MG_CURSOR_HIDDEN]       = NULL;
}

static LRESULT CALLBACK mgapp_win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_CLOSE:
        {
            mgapp_event event = { .type = MGAPP_EVENT_WINDOW_CLOSE };
            mgapp_call_event(&event);
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
        {
            RECT r;
            GetClientRect(hwnd, &r);
            mgapp_state.window_width  = r.right - r.left;
            mgapp_state.window_height = r.bottom - r.top;
            mgapp_event event = {
                .window = {
                    .width = mgapp_state.window_width,
                    .height = mgapp_state.window_height,
                },
                .type = MGAPP_EVENT_RESIZE
            };
            mgapp_call_event(&event);
            break;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            mg_key key = (mg_key)w_param;

            bool extended = (l_param & (1 << 24)) != 0;

            switch (key)
            {
                case MG_KEY_CONTROL:
                    key = extended ? MG_KEY_RCONTROL : MG_KEY_LCONTROL;
                    break;
                case MG_KEY_ALT:
                    key = extended ? MG_KEY_RALT : MG_KEY_LALT;
                    break;
                case MG_KEY_SHIFT:
                {
                    uint32_t scancode = (l_param >> 16) & 0xFF;
                    uint32_t vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
                    key = (vk == VK_RSHIFT) ? MG_KEY_RSHIFT : MG_KEY_LSHIFT;
                    break;
                }
            }

            mgapp_input_process_key(key, pressed);
            mgapp_event event = {
                .key = key,
                .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_KEY_DOWN : MGAPP_EVENT_KEY_UP)
            };
            mgapp_call_event(&event);
            break;
        }
        case WM_CHAR:
        {
            static WCHAR pending_high_surrogate = 0;
            WCHAR ch = (WCHAR)w_param;

            if (IS_HIGH_SURROGATE(ch))
            {
                pending_high_surrogate = ch;
                return 0;
            }

            UINT32 codepoint;
            if (IS_LOW_SURROGATE(ch) && pending_high_surrogate)
            {
                codepoint = 0x10000  + ((pending_high_surrogate - 0xD800) << 10)  + (ch - 0xDC00);
                pending_high_surrogate = 0;
            }
            else
            {
                codepoint = ch;
            }
            mgapp_input_process_char(codepoint);
            mgapp_event event = {
                .codepoint = codepoint,
                .type = MGAPP_EVENT_CHAR
            };
            mgapp_call_event(&event);
        break;
        }
        case WM_MOUSEMOVE:
        {
            int32_t x = GET_X_LPARAM(l_param);
            int32_t y = GET_Y_LPARAM(l_param);
            mgapp_state.input.mouse.x = x;
            mgapp_state.input.mouse.y = y;
            mgapp_event event = {
                .mouse = {
                    .x = x,
                    .y = y
                },
                .type = MGAPP_EVENT_MOUSE_MOVE
            };
            mgapp_call_event(&event);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int32_t delta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (delta != 0)
            {
                delta = (delta < 0) ? -1 : 1;
                mgapp_state.input.mouse.delta = (int8_t)delta;
                mgapp_event event = {
                    .mouse = {
                        .scroll = delta
                    },
                    .type = MGAPP_EVENT_MOUSE_SCROLL
                };
                mgapp_call_event(&event);
            }
            break;
        }
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            bool pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
            mg_mouse_button mouse_button = MG_MOUSE_BUTTON_MAX;
            switch (msg)
            {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    mouse_button = MG_MOUSE_BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    mouse_button = MG_MOUSE_BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    mouse_button = MG_MOUSE_BUTTON_RIGHT;
                    break;
            }

            mgapp_input_process_mouse_button(mouse_button, pressed, mgapp_state.frame_time);
            mgapp_event event = {
                .mouse = {
                    .button = mouse_button,
                },
                .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_MOUSE_DOWN : MGAPP_EVENT_MOUSE_UP)
            };
            mgapp_call_event(&event);
            break;
        }
        case WM_SETCURSOR:
        {
            if (LOWORD(l_param) == HTCLIENT)
            {
                SetCursor(mgapp_state.win32.cursor_table[mgapp_state.current_cursor]);
                return TRUE;
            }
            break;
        }
        case WM_DPICHANGED:
        {
            mgapp_state.dpi = HIWORD(w_param);
            mgapp_state.dpi_scale = (float)mgapp_state.dpi / 96.0f;
            RECT *suggested = (RECT*)l_param;
            SetWindowPos(hwnd, NULL,
                suggested->left, suggested->top,
                suggested->right - suggested->left,
                suggested->bottom - suggested->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            break;
        }
        case WM_DROPFILES:
        {
            HDROP hdrop = (HDROP)w_param;
            UINT count = DragQueryFileA(hdrop, 0xFFFFFFFF, NULL, 0);

            size_t ptr_block_size = sizeof(char*) * count;
            size_t str_block_size = (size_t)MAX_PATH * count;
            char *block = (char*)malloc(ptr_block_size + str_block_size);

            if (block)
            {
                char **paths = (char**)block;
                char *strings = block + ptr_block_size;

                for (UINT i = 0; i < count; i++)
                {
                    char *dst = strings + (size_t)i * MAX_PATH;
                    DragQueryFileA(hdrop, i, dst, MAX_PATH);
                    paths[i] = dst;
                }

                mgapp_event event = {
                    .file_drop = {
                        .paths = (const char**)paths,
                        .path_count = (int32_t)count
                    },
                    .type = MGAPP_EVENT_FILE_DROP
                };
                mgapp_call_event(&event);

                free(block);
            }

            DragFinish(hdrop);
            break;
        }
        case WM_ENTERSIZEMOVE:
        {
            SetTimer(hwnd, 1, USER_TIMER_MINIMUM, NULL);
            break;
        }
        case WM_EXITSIZEMOVE:
        {
            KillTimer(hwnd, 1);
            break;
        }
        case WM_TIMER:
        {
            if (w_param == 1)
            {
                const float new_time = mgapp_time();
                mgapp_state.delta_time = new_time - mgapp_state.frame_time;
                mgapp_state.frame_time = new_time;

                if (mgapp_state.events.update)
                    mgapp_state.events.update();
                mgapp_input_frame();
            }
            break;
        }
    }
    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

static LRESULT CALLBACK mgapp_win32_no_titlebar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_NCCALCSIZE:
        {
            if (!w_param)
                break;
            NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS *)l_param;
            RECT *r = params->rgrc;
            int32_t bx = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            int32_t by = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            r->left += bx;
            r->right -= bx;
            r->bottom -= by;
            r->top += IsZoomed(hwnd) ? by : 0;
            return 0;
        }
        case WM_NCHITTEST:
        {
            POINTS mp = MAKEPOINTS(l_param);
            POINT sp = { mp.x, mp.y };
            RECT wr;
            GetWindowRect(hwnd, &wr);
            const int bw = 8;
            int rx = sp.x - wr.left;
            int ry = sp.y - wr.top;
            int w = wr.right  - wr.left;
            int h = wr.bottom - wr.top;

            if (!(mgapp_state.flags & MGAPP_FLAG_NO_RESIZE))
            {
                if (ry >= h - bw)
                    return (rx <= bw) ? HTBOTTOMLEFT :
                        (rx >= w - bw) ? HTBOTTOMRIGHT : HTBOTTOM;

                if (ry <= bw)
                    return (rx <= bw) ? HTTOPLEFT :
                        (rx >= w - bw) ? HTTOPRIGHT : HTTOP;

                if (rx <= bw)
                    return HTLEFT;

                if (rx >= w - bw)
                    return HTRIGHT;
            }

            if (rx >= mgapp_state.caption.x && rx < mgapp_state.caption.x + mgapp_state.caption.width &&
                ry >= mgapp_state.caption.y && ry < mgapp_state.caption.y + mgapp_state.caption.height)
                return HTCAPTION;
            return HTCLIENT;
        }
        case WM_NCMOUSEMOVE:
        {
            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            ScreenToClient(mgapp_state.win32.hwnd, &pt);
            mgapp_state.input.mouse.x = (int16_t)pt.x;
            mgapp_state.input.mouse.y = (int16_t)pt.y;
            mgapp_event event = {
                .mouse = {
                    .x = pt.x,
                    .y = pt.y
                },
                .type = MGAPP_EVENT_MOUSE_MOVE
            };
            mgapp_call_event(&event);
            break;
        }

        case WM_NCACTIVATE:
            break;
    }
    return CallWindowProc(mgapp_state.win32.original_proc, hwnd, msg, w_param, l_param);
}

int32_t mgapp_run(const mgapp_init_info *info)
{
    mgapp_win32_state *win32_state = &mgapp_state.win32;

    const char *class_name = "mgapp_window_class";
    win32_state->hinstance = GetModuleHandleA(NULL);

    mgapp_win32_setup_clock(win32_state);
    mgapp_win32_load_cursors(win32_state);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = mgapp_win32_process_message;
    wc.hInstance = win32_state->hinstance;
    wc.lpszClassName = class_name;
    wc.hCursor = win32_state->cursor_table[MG_CURSOR_ARROW];

    if (!RegisterClassA(&wc))
    {
        MGAPP_ERROR("Window registration failed!");
        return 1;
    }

    const int32_t width = info->width ? (int32_t)info->width : CW_USEDEFAULT;
    const int32_t height = info->height ? (int32_t)info->height : CW_USEDEFAULT;

    DWORD style = WS_OVERLAPPEDWINDOW;

    if (info->flags & MGAPP_FLAG_NO_RESIZE)
        style &= ~WS_THICKFRAME;

    if (info->flags & MGAPP_FLAG_NO_MINIMIZE)
        style &= ~WS_MINIMIZEBOX;

    if (info->flags & MGAPP_FLAG_NO_MAXIMIZE)
        style &= ~WS_MAXIMIZEBOX;

    win32_state->hwnd = CreateWindowExA(
        0,
        class_name,
        info->title,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        NULL,
        NULL,
        win32_state->hinstance,
        NULL
    );

    if (!win32_state->hwnd)
    {
        MGAPP_ERROR("Window creation failed!");
        return 1;
    }

    if (info->flags & MGAPP_FLAG_NO_TITLEBAR)
    {
        LONG_PTR style = GetWindowLongPtr(win32_state->hwnd, GWL_STYLE);
        style |= WS_THICKFRAME | WS_CAPTION;
        SetWindowLongPtr(win32_state->hwnd, GWL_STYLE, style);

        RECT wr;
        GetWindowRect(win32_state->hwnd, &wr);
        int32_t w = wr.right - wr.left;
        int32_t h = wr.bottom - wr.top;

        win32_state->original_proc = (WNDPROC)GetWindowLongPtr(win32_state->hwnd, GWLP_WNDPROC);
        SetWindowLongPtr(win32_state->hwnd, GWLP_WNDPROC, (LONG_PTR)mgapp_win32_no_titlebar_proc);
        SetWindowPos(win32_state->hwnd, NULL, 0, 0, w, h, SWP_FRAMECHANGED | SWP_NOMOVE);
    }

    if (!(info->flags & MGAPP_FLAG_HIDE_WINDOW))
    {
        ShowWindow(
            win32_state->hwnd,
            SW_SHOW
        );
    }
    UpdateWindow(win32_state->hwnd);

    if (info->flags & MGAPP_FLAG_ENABLE_FILE_DROPS)
        DragAcceptFiles(win32_state->hwnd, TRUE);

    mgapp_state.dpi = GetDpiForWindow(win32_state->hwnd);
    mgapp_state.dpi_scale = (float)mgapp_state.dpi / 96.0f;

    mgapp_state.events = info->events;
    mgapp_state.flags = info->flags;

    if (info->events.start)
        info->events.start();

    mgapp_state.running = true;
    while (mgapp_state.running)
    {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                mgapp_state.running = false;
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        const float new_time = mgapp_time();
        mgapp_state.delta_time = new_time - mgapp_state.frame_time;
        mgapp_state.frame_time = new_time;

        if (info->events.update)
            info->events.update();

        mgapp_input_frame();
    }

    if (info->events.end)
        info->events.end();

    DestroyWindow(win32_state->hwnd);
    UnregisterClassA(class_name, win32_state->hinstance);
    return 0;
}

void mgapp_close(void)
{
    PostMessage(mgapp_state.win32.hwnd, WM_CLOSE, 0, 0);
}

void mgapp_show(bool value)
{
    ShowWindow(mgapp_state.win32.hwnd, value ? SW_SHOW : SW_HIDE);
}

void mgapp_minimize(void)
{
    ShowWindow(mgapp_state.win32.hwnd, SW_MINIMIZE);
}

void mgapp_maximize(void)
{
    ShowWindow(mgapp_state.win32.hwnd, SW_MAXIMIZE);
}

void mgapp_restore(void)
{
    ShowWindow(mgapp_state.win32.hwnd, SW_RESTORE);
}

bool mgapp_maximized(void)
{
    return IsZoomed(mgapp_state.win32.hwnd);
}

void mgapp_set_cursor(mg_cursor cursor)
{
    if (mgapp_state.current_cursor == cursor)
        return;
    SetCursor(mgapp_state.win32.cursor_table[cursor]);
    mgapp_state.current_cursor = cursor;
}

float mgapp_time(void)
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (float)(now_time.QuadPart - mgapp_state.win32.start_time.QuadPart) * mgapp_state.win32.clock_frequency;
}

void *mgapp_primary_handle(void)
{
    return (void*)mgapp_state.win32.hwnd;
}

void *mgapp_secondary_handle(void)
{
    return (void*)mgapp_state.win32.hinstance;
}

#elif defined(__linux__)

static inline mg_key mgapp_xlib_translate_key(KeySym sym)
{
    if (sym >= XK_a && sym <= XK_z)
        return (mg_key)(MG_KEY_A + (sym - XK_a));
    if (sym >= XK_A && sym <= XK_Z)
        return (mg_key)(MG_KEY_A + (sym - XK_A));
    if (sym >= XK_0 && sym <= XK_9)
        return (mg_key)(MG_KEY_0 + (sym - XK_0));
    if (sym >= XK_F1 && sym <= XK_F24)
        return (mg_key)(MG_KEY_F1 + (sym - XK_F1));
    if (sym >= XK_KP_0 && sym <= XK_KP_9)
        return (mg_key)(MG_KEY_NUMPAD0 + (sym - XK_KP_0));

    switch (sym)
    {
        case XK_BackSpace:   return MG_KEY_BACKSPACE;
        case XK_Return:      return MG_KEY_ENTER;
        case XK_KP_Enter:    return MG_KEY_ENTER;
        case XK_Tab:         return MG_KEY_TAB;
        case XK_Pause:       return MG_KEY_PAUSE;
        case XK_Caps_Lock:   return MG_KEY_CAPITAL;
        case XK_Escape:      return MG_KEY_ESCAPE;
        case XK_space:       return MG_KEY_SPACE;
        case XK_Page_Up:     return MG_KEY_PAGE_UP;
        case XK_Page_Down:   return MG_KEY_PAGE_DOWN;
        case XK_End:         return MG_KEY_END;
        case XK_Home:        return MG_KEY_HOME;
        case XK_Left:        return MG_KEY_LEFT;
        case XK_Up:          return MG_KEY_UP;
        case XK_Right:       return MG_KEY_RIGHT;
        case XK_Down:        return MG_KEY_DOWN;
        case XK_Select:      return MG_KEY_SELECT;
        case XK_Print:       return MG_KEY_PRINT;
        case XK_Execute:     return MG_KEY_EXECUTE;
        case XK_Insert:      return MG_KEY_INSERT;
        case XK_Delete:      return MG_KEY_DELETE;
        case XK_Help:        return MG_KEY_HELP;

        case XK_Super_L:     return MG_KEY_LSUPER;
        case XK_Super_R:     return MG_KEY_RSUPER;
        case XK_Menu:        return MG_KEY_APPS;

        case XK_KP_Multiply: return MG_KEY_MULTIPLY;
        case XK_KP_Add:      return MG_KEY_ADD;
        case XK_KP_Separator:return MG_KEY_SEPARATOR;
        case XK_KP_Subtract: return MG_KEY_SUBTRACT;
        case XK_KP_Decimal:  return MG_KEY_DECIMAL;
        case XK_KP_Divide:   return MG_KEY_DIVIDE;

        case XK_Num_Lock:    return MG_KEY_NUMLOCK;
        case XK_Scroll_Lock: return MG_KEY_SCROLL;
        case XK_KP_Equal:    return MG_KEY_NUMPAD_EQUAL;

        case XK_Shift_L:     return MG_KEY_LSHIFT;
        case XK_Shift_R:     return MG_KEY_RSHIFT;
        case XK_Control_L:   return MG_KEY_LCONTROL;
        case XK_Control_R:   return MG_KEY_RCONTROL;
        case XK_Alt_L:       return MG_KEY_LALT;
        case XK_Alt_R:       return MG_KEY_RALT;

        case XK_semicolon:   return MG_KEY_SEMICOLON;
        case XK_apostrophe:  return MG_KEY_APOSTROPHE;
        case XK_equal:       return MG_KEY_EQUAL;
        case XK_comma:       return MG_KEY_COMMA;
        case XK_minus:       return MG_KEY_MINUS;
        case XK_period:      return MG_KEY_PERIOD;
        case XK_slash:       return MG_KEY_SLASH;
        case XK_grave:       return MG_KEY_GRAVE;
        case XK_bracketleft: return MG_KEY_LBRACKET;
        case XK_backslash:   return MG_KEY_PIPE;
        case XK_bracketright:return MG_KEY_RBRACKET;

        default: return MG_KEY_MAX;
    }
}

static inline mg_mouse_button mgapp_xlib_translate_button(uint32_t x_button)
{
    switch (x_button)
    {
        case Button1: return MG_MOUSE_BUTTON_LEFT;
        case Button2: return MG_MOUSE_BUTTON_MIDDLE;
        case Button3: return MG_MOUSE_BUTTON_RIGHT;
        default:      return MG_MOUSE_BUTTON_MAX;
    }
}

static inline void mgapp_xlib_load_cursors(mgapp_xlib_state *xlib_state)
{
    xlib_state->cursor_table[MG_CURSOR_ARROW] = XCreateFontCursor(xlib_state->display, XC_left_ptr);
    xlib_state->cursor_table[MG_CURSOR_IBEAM] = XCreateFontCursor(xlib_state->display, XC_xterm);
    xlib_state->cursor_table[MG_CURSOR_CROSSHAIR] = XCreateFontCursor(xlib_state->display, XC_crosshair);
    xlib_state->cursor_table[MG_CURSOR_HAND] = XCreateFontCursor(xlib_state->display, XC_hand2);
    xlib_state->cursor_table[MG_CURSOR_RESIZE_ALL] = XCreateFontCursor(xlib_state->display, XC_fleur);
    xlib_state->cursor_table[MG_CURSOR_RESIZE_NS] = XCreateFontCursor(xlib_state->display, XC_sb_v_double_arrow);
    xlib_state->cursor_table[MG_CURSOR_RESIZE_EW] = XCreateFontCursor(xlib_state->display, XC_sb_h_double_arrow);
    xlib_state->cursor_table[MG_CURSOR_RESIZE_NESW] = XCreateFontCursor(xlib_state->display, XC_bottom_left_corner);
    xlib_state->cursor_table[MG_CURSOR_RESIZE_NWSE] = XCreateFontCursor(xlib_state->display, XC_bottom_right_corner);
    xlib_state->cursor_table[MG_CURSOR_NOT_ALLOWED] = XCreateFontCursor(xlib_state->display, XC_pirate);

    {
        const Pixmap cursor_pixmap = XCreatePixmap(xlib_state->display, xlib_state->window, 1, 1, 1);
        GC gfx_ctx = XCreateGC(xlib_state->display, cursor_pixmap, 0, NULL);
        XDrawPoint(xlib_state->display, cursor_pixmap, gfx_ctx, 0, 0);
        XFreeGC(xlib_state->display, gfx_ctx);

        XColor color;
        color.flags = DoRed | DoGreen | DoBlue;
        color.red = color.blue = color.green = 0;
        xlib_state->cursor_table[MG_CURSOR_HIDDEN] = XCreatePixmapCursor(xlib_state->display, cursor_pixmap, cursor_pixmap, &color, &color, 0, 0);

        XFreePixmap(xlib_state->display, cursor_pixmap);
    }
}

static inline uint32_t mgapp_xlib_query_dpi(Display *display, int32_t screen)
{
    char *rms = XResourceManagerString(display);
    if (rms)
    {
        XrmDatabase db = XrmGetStringDatabase(rms);
        if (db)
        {
            XrmValue value;
            char *type = NULL;
            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value) && value.addr)
            {
                uint32_t dpi = (uint32_t)atoi(value.addr);
                XrmDestroyDatabase(db);
                if (dpi > 0)
                    return dpi;
            }
            XrmDestroyDatabase(db);
        }
    }

    int32_t width_px = DisplayWidth(display, screen);
    int32_t width_mm = DisplayWidthMM(display, screen);
    if (width_mm > 0)
        return (uint32_t)(width_px * 25.4f / (float)width_mm + 0.5f);

    return 96;
}

static inline void mgapp_xlib_load_dpi(mgapp_xlib_state *xlib_state)
{
    mgapp_state.dpi = mgapp_xlib_query_dpi(xlib_state->display, xlib_state->screen);
    mgapp_state.dpi_scale = (float)mgapp_state.dpi / 96.0f;
}

typedef struct
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
}
mgapp_motif_wm_hints;

#define MGAPP_MWM_HINTS_DECORATIONS (1L << 1)
#define MGAPP_MWM_DECOR_ALL (1L << 0)

static inline void mgapp_xlib_set_decorated(mgapp_xlib_state *xlib_state, bool decorated)
{
    mgapp_motif_wm_hints hints = {0};
    hints.flags = MGAPP_MWM_HINTS_DECORATIONS;
    hints.decorations = decorated ? MGAPP_MWM_DECOR_ALL : 0;

    XChangeProperty(
        xlib_state->display, xlib_state->window,
        xlib_state->motif_wm_hints, xlib_state->motif_wm_hints,
        32, PropModeReplace,
        (unsigned char *)&hints, 5
    );
}

typedef enum
{
    MGAPP_XLIB_HIT_NONE,
    MGAPP_XLIB_HIT_LEFT,
    MGAPP_XLIB_HIT_RIGHT,
    MGAPP_XLIB_HIT_TOP,
    MGAPP_XLIB_HIT_BOTTOM,
    MGAPP_XLIB_HIT_TOPLEFT,
    MGAPP_XLIB_HIT_TOPRIGHT,
    MGAPP_XLIB_HIT_BOTTOMLEFT,
    MGAPP_XLIB_HIT_BOTTOMRIGHT
}
mgapp_xlib_hit_test;

static inline mgapp_xlib_hit_test mgapp_xlib_test_edge(int32_t x, int32_t y, int32_t w, int32_t h)
{
    const int32_t bw = 8;
    bool left = x <= bw;
    bool right = x >= w - bw;
    bool top = y <= bw;
    bool bottom = y >= h - bw;

    if (top && left) return MGAPP_XLIB_HIT_TOPLEFT;
    if (top && right) return MGAPP_XLIB_HIT_TOPRIGHT;
    if (bottom && left) return MGAPP_XLIB_HIT_BOTTOMLEFT;
    if (bottom && right) return MGAPP_XLIB_HIT_BOTTOMRIGHT;
    if (left) return MGAPP_XLIB_HIT_LEFT;
    if (right) return MGAPP_XLIB_HIT_RIGHT;
    if (top) return MGAPP_XLIB_HIT_TOP;
    if (bottom) return MGAPP_XLIB_HIT_BOTTOM;
    return MGAPP_XLIB_HIT_NONE;
}

static inline int32_t mgapp_xlib_hit_to_moveresize_dir(mgapp_xlib_hit_test hit)
{
    switch (hit)
    {
        case MGAPP_XLIB_HIT_TOPLEFT:     return 0;
        case MGAPP_XLIB_HIT_TOP:         return 1;
        case MGAPP_XLIB_HIT_TOPRIGHT:    return 2;
        case MGAPP_XLIB_HIT_RIGHT:       return 3;
        case MGAPP_XLIB_HIT_BOTTOMRIGHT: return 4;
        case MGAPP_XLIB_HIT_BOTTOM:      return 5;
        case MGAPP_XLIB_HIT_BOTTOMLEFT:  return 6;
        case MGAPP_XLIB_HIT_LEFT:        return 7;
        default:                         return -1;
    }
}

static inline void mgapp_xlib_apply_size_hints(mgapp_xlib_state *xlib_state, mgapp_flags flags, int32_t width, int32_t height)
{
    XSizeHints *hints = XAllocSizeHints();
    if (!hints)
        return;

    if (flags & MGAPP_FLAG_NO_RESIZE)
    {
        hints->flags = PMinSize | PMaxSize;
        hints->min_width = hints->max_width = width;
        hints->min_height = hints->max_height = height;
    }
    else
    {
        hints->flags = PMinSize;
        hints->min_width = 1;
        hints->min_height = 1;
    }

    XSetWMNormalHints(xlib_state->display, xlib_state->window, hints);
    XFree(hints);
}

static inline void mgapp_xlib_apply_allowed_actions(mgapp_xlib_state *xlib_state, mgapp_flags flags)
{
    Atom actions[8];
    int32_t count = 0;

    if (!(flags & MGAPP_FLAG_NO_RESIZE))
        actions[count++] = xlib_state->net_wm_action_resize;
    if (!(flags & MGAPP_FLAG_NO_MINIMIZE))
        actions[count++] = xlib_state->net_wm_action_minimize;
    if (!(flags & MGAPP_FLAG_NO_MAXIMIZE))
    {
        actions[count++] = xlib_state->net_wm_action_maximize_horz;
        actions[count++] = xlib_state->net_wm_action_maximize_vert;
    }

    XChangeProperty(
        xlib_state->display, xlib_state->window,
        xlib_state->net_wm_allowed_actions, XA_ATOM,
        32, PropModeReplace,
        (unsigned char *)actions, count
    );
}

static inline void mgapp_xlib_handle_xdnd_client_message(mgapp_xlib_state *xlib_state, const XClientMessageEvent *cm)
{
    Atom type = cm->message_type;

    if (type == xlib_state->xdnd_enter)
    {
        xlib_state->xdnd_source = (Window)cm->data.l[0];
    }
    else if (type == xlib_state->xdnd_position)
    {
        xlib_state->xdnd_source = (Window)cm->data.l[0];

        XEvent reply = {0};
        reply.type = ClientMessage;
        reply.xclient.window = xlib_state->xdnd_source;
        reply.xclient.message_type = xlib_state->xdnd_status;
        reply.xclient.format = 32;
        reply.xclient.data.l[0] = xlib_state->window;
        reply.xclient.data.l[1] = 1;
        reply.xclient.data.l[2] = 0;
        reply.xclient.data.l[3] = 0;
        reply.xclient.data.l[4] = xlib_state->xdnd_action_copy;

        XSendEvent(xlib_state->display, xlib_state->xdnd_source, False, NoEventMask, &reply);
        XFlush(xlib_state->display);
    }
    else if (type == xlib_state->xdnd_leave)
    {
        xlib_state->xdnd_source = 0;
    }
    else if (type == xlib_state->xdnd_drop)
    {
        Time drop_time = (Time)cm->data.l[2];

        XConvertSelection(
            xlib_state->display,
            xlib_state->xdnd_selection,
            xlib_state->uri_list,
            xlib_state->xdnd_selection,
            xlib_state->window,
            drop_time
        );
    }
}

static inline void mgapp_xlib_handle_xdnd_selection_notify(mgapp_xlib_state *xlib_state, const XSelectionEvent *se)
{
    if (se->property != xlib_state->xdnd_selection)
        return;

    Atom actual_type;
    int32_t actual_format;
    unsigned long item_count, bytes_after;
    unsigned char *data = NULL;

    XGetWindowProperty(
        xlib_state->display, xlib_state->window,
        xlib_state->xdnd_selection, 0, 65536, False,
        xlib_state->uri_list,
        &actual_type, &actual_format, &item_count, &bytes_after, &data
    );

    if (data)
    {
        char **paths = (char**)malloc(sizeof(char*) * 64);
        int32_t count = 0;
        char *line = (char*)data;

        while (*line && count < 64)
        {
            char *end = strstr(line, "\r\n");
            if (end) *end = '\0';

            if (strncmp(line, "file://", 7) == 0)
                paths[count++] = line + 7;

            if (!end) break;
            line = end + 2;
        }

        if (count > 0)
        {
            mgapp_event event = {
                .file_drop = {
                    .paths = (const char**)paths,
                    .path_count = count
                },
                .type = MGAPP_EVENT_FILE_DROP
            };
            mgapp_call_event(&event);
        }

        free(paths);
        XFree(data);
    }

    XEvent finished = {0};
    finished.type = ClientMessage;
    finished.xclient.window = xlib_state->xdnd_source;
    finished.xclient.message_type = xlib_state->xdnd_finished;
    finished.xclient.format = 32;
    finished.xclient.data.l[0] = xlib_state->window;
    finished.xclient.data.l[1] = 1;
    finished.xclient.data.l[2] = xlib_state->xdnd_action_copy;

    XSendEvent(xlib_state->display, xlib_state->xdnd_source, False, NoEventMask, &finished);
    XFlush(xlib_state->display);

    xlib_state->xdnd_source = 0;
}

static int32_t mgapp_xlib_run(const mgapp_init_info *info)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;

    xlib_state->display = XOpenDisplay(NULL);
    if (!xlib_state->display)
    {
        MGAPP_ERROR("Failed to open X display");
        return 1;
    }

    xlib_state->screen = DefaultScreen(xlib_state->display);

    const uint32_t width = info->width ? info->width : 800;
    const uint32_t height = info->height ? info->height : 600;

    Window root = RootWindow(xlib_state->display, xlib_state->screen);

    XSetWindowAttributes swa = {0};
    swa.background_pixmap = None;
    swa.bit_gravity = NorthWestGravity;
    swa.event_mask =
        StructureNotifyMask | ExposureMask |
        KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask;

    xlib_state->window = XCreateWindow(
        xlib_state->display,
        root,
        0, 0,
        width, height,
        0,
        DefaultDepth(xlib_state->display, xlib_state->screen),
        InputOutput,
        DefaultVisual(xlib_state->display, xlib_state->screen),
        CWBackPixmap | CWBitGravity | CWEventMask,
        &swa
    );

    if (!xlib_state->window)
    {
        MGAPP_ERROR("Failed to create X window");
        XCloseDisplay(xlib_state->display);
        return 1;
    }

    if (info->title)
        XStoreName(xlib_state->display, xlib_state->window, info->title);

    if (!(info->flags & MGAPP_FLAG_HIDE_WINDOW))
    {
        XMapWindow(xlib_state->display, xlib_state->window);
        XFlush(xlib_state->display);
    }

    xlib_state->wm_delete_window = XInternAtom(xlib_state->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(xlib_state->display, xlib_state->window, &xlib_state->wm_delete_window, 1);

    xlib_state->net_wm_state = XInternAtom(xlib_state->display, "_NET_WM_STATE", False);
    xlib_state->net_wm_state_maximized_horz = XInternAtom(xlib_state->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    xlib_state->net_wm_state_maximized_vert = XInternAtom(xlib_state->display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    
    xlib_state->motif_wm_hints = XInternAtom(xlib_state->display, "_MOTIF_WM_HINTS", False);
    xlib_state->net_wm_moveresize = XInternAtom(xlib_state->display, "_NET_WM_MOVERESIZE", False);
    xlib_state->net_wm_allowed_actions = XInternAtom(xlib_state->display, "_NET_WM_ALLOWED_ACTIONS", False);
    xlib_state->net_wm_action_resize = XInternAtom(xlib_state->display, "_NET_WM_ACTION_RESIZE", False);
    xlib_state->net_wm_action_minimize = XInternAtom(xlib_state->display, "_NET_WM_ACTION_MINIMIZE", False);
    xlib_state->net_wm_action_maximize_horz = XInternAtom(xlib_state->display, "_NET_WM_ACTION_MAXIMIZE_HORZ", False);
    xlib_state->net_wm_action_maximize_vert = XInternAtom(xlib_state->display, "_NET_WM_ACTION_MAXIMIZE_VERT", False);

    if (info->flags & MGAPP_FLAG_ENABLE_FILE_DROPS)
    {
        xlib_state->xdnd_aware = XInternAtom(xlib_state->display, "XdndAware", False);
        xlib_state->xdnd_enter = XInternAtom(xlib_state->display, "XdndEnter", False);
        xlib_state->xdnd_position = XInternAtom(xlib_state->display, "XdndPosition", False);
        xlib_state->xdnd_status = XInternAtom(xlib_state->display, "XdndStatus", False);
        xlib_state->xdnd_leave = XInternAtom(xlib_state->display, "XdndLeave", False);
        xlib_state->xdnd_drop = XInternAtom(xlib_state->display, "XdndDrop", False);
        xlib_state->xdnd_finished = XInternAtom(xlib_state->display, "XdndFinished", False);
        xlib_state->xdnd_selection = XInternAtom(xlib_state->display, "XdndSelection", False);
        xlib_state->xdnd_type_list = XInternAtom(xlib_state->display, "XdndTypeList", False);
        xlib_state->xdnd_action_copy = XInternAtom(xlib_state->display, "XdndActionCopy", False);
        xlib_state->uri_list = XInternAtom(xlib_state->display, "text/uri-list", False);

        const Atom xdnd_version = 5;
        XChangeProperty(
            xlib_state->display,
            xlib_state->window,
            xlib_state->xdnd_aware,
            XA_ATOM,
            32,
            PropModeReplace,
            (unsigned char*)&xdnd_version,
            1
        );
    }

    if (info->flags & MGAPP_FLAG_NO_TITLEBAR)
        mgapp_xlib_set_decorated(xlib_state, false);

    mgapp_xlib_apply_size_hints(xlib_state, info->flags, (int32_t)width, (int32_t)height);
    mgapp_xlib_apply_allowed_actions(xlib_state, info->flags);

    mgapp_xlib_load_cursors(xlib_state);
    mgapp_xlib_load_dpi(xlib_state);

    mgapp_state.window_width = (int32_t)width;
    mgapp_state.window_height = (int32_t)height;

    mgapp_state.running = true;

    if (info->events.start)
        info->events.start();

    while (mgapp_state.running)
    {
        while (XPending(xlib_state->display))
        {
            XEvent xev;
            XNextEvent(xlib_state->display, &xev);

            switch (xev.type)
            {
                case ConfigureNotify:
                {
                    XConfigureEvent *ce = &xev.xconfigure;
                    if (ce->width != mgapp_state.window_width ||
                        ce->height != mgapp_state.window_height)
                    {
                        mgapp_state.window_width = ce->width;
                        mgapp_state.window_height = ce->height;
                        mgapp_event event = {
                            .window = {
                                .width = mgapp_state.window_width,
                                .height = mgapp_state.window_height
                            },
                            .type = MGAPP_EVENT_RESIZE
                        };
                        mgapp_call_event(&event);
                    }
                    break;
                }
                case ClientMessage:
                {
                    if ((Atom)xev.xclient.data.l[0] == xlib_state->wm_delete_window)
                    {
                        mgapp_event event = { .type = MGAPP_EVENT_WINDOW_CLOSE };
                        mgapp_call_event(&event);
                        mgapp_state.running = false;
                    }
                    else if (mgapp_state.flags & MGAPP_FLAG_ENABLE_FILE_DROPS)
                        mgapp_xlib_handle_xdnd_client_message(xlib_state, &xev.xclient);
                    break;
                }
                case SelectionNotify:
                {
                    if (mgapp_state.flags & MGAPP_FLAG_ENABLE_FILE_DROPS)
                        mgapp_xlib_handle_xdnd_selection_notify(xlib_state, &xev.xselection);
                    break;
                }
                case KeyPress:
                case KeyRelease:
                {
                    bool pressed = xev.type == KeyPress;
                    KeySym sym = XLookupKeysym(&xev.xkey, 0);
                    mg_key key = mgapp_xlib_translate_key(sym);

                    if (key != MG_KEY_MAX)
                    {
                        mgapp_input_process_key(key, pressed);
                        mgapp_event event = {
                            .key = key,
                            .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_KEY_DOWN : MGAPP_EVENT_KEY_UP)
                        };
                        mgapp_call_event(&event);
                    }

                    if (pressed)
                    {
                        char buf[32];
                        KeySym ks;
                        int32_t len = XLookupString(&xev.xkey, buf, sizeof(buf) - 1, &ks, NULL);
                        if (len > 0)
                        {
                            buf[len] = '\0';
                            mgapp_input_process_char((uint32_t)(unsigned char)buf[0]);
                            mgapp_event event = {
                                .codepoint = (uint32_t)(unsigned char)buf[0],
                                .type = MGAPP_EVENT_CHAR
                            };
                            mgapp_call_event(&event);
                        }
                    }
                    break;
                }
                case ButtonPress:
                case ButtonRelease:
                {
                    uint32_t xbutton = xev.xbutton.button;

                    if (xbutton == Button4 || xbutton == Button5)
                    {
                        if (xev.type == ButtonPress)
                        {
                            int8_t delta = (xbutton == Button4) ? 1 : -1;
                            mgapp_state.input.mouse.delta = delta;
                            mgapp_event event = {
                                .mouse = { .scroll = delta },
                                .type = MGAPP_EVENT_MOUSE_SCROLL
                            };
                            mgapp_call_event(&event);
                        }
                        break;
                    }

                    mg_mouse_button button = mgapp_xlib_translate_button(xbutton);
                    if (button == MG_MOUSE_BUTTON_MAX)
                        break;

                    bool pressed = xev.type == ButtonPress;

                    if (pressed && button == MG_MOUSE_BUTTON_LEFT && (mgapp_state.flags & MGAPP_FLAG_NO_TITLEBAR))
                    {
                        int32_t dir = -1;

                        if (!(mgapp_state.flags & MGAPP_FLAG_NO_RESIZE))
                        {
                            mgapp_xlib_hit_test hit = mgapp_xlib_test_edge(
                                mgapp_state.input.mouse.x, mgapp_state.input.mouse.y,
                                mgapp_state.window_width, mgapp_state.window_height);
                            dir = mgapp_xlib_hit_to_moveresize_dir(hit);
                        }

                        bool in_caption =
                            mgapp_state.input.mouse.x >= mgapp_state.caption.x &&
                            mgapp_state.input.mouse.x < mgapp_state.caption.x + mgapp_state.caption.width &&
                            mgapp_state.input.mouse.y >= mgapp_state.caption.y &&
                            mgapp_state.input.mouse.y < mgapp_state.caption.y + mgapp_state.caption.height;

                        if (dir != -1 || in_caption)
                        {
                            XEvent mev = {0};
                            mev.type = ClientMessage;
                            mev.xclient.window = xlib_state->window;
                            mev.xclient.message_type = xlib_state->net_wm_moveresize;
                            mev.xclient.format = 32;
                            mev.xclient.data.l[0] = xev.xbutton.x_root;
                            mev.xclient.data.l[1] = xev.xbutton.y_root;
                            mev.xclient.data.l[2] = (dir != -1) ? dir : 8;
                            mev.xclient.data.l[3] = Button1;
                            mev.xclient.data.l[4] = 1;

                            XUngrabPointer(xlib_state->display, CurrentTime);
                            XSendEvent(xlib_state->display, RootWindow(xlib_state->display, xlib_state->screen),
                                False, SubstructureRedirectMask | SubstructureNotifyMask, &mev);
                            XFlush(xlib_state->display);
                            break;
                        }
                    }

                    mgapp_input_process_mouse_button(button, pressed, mgapp_state.frame_time);
                    mgapp_event event = {
                        .mouse = { .button = button },
                        .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_MOUSE_DOWN : MGAPP_EVENT_MOUSE_UP)
                    };
                    mgapp_call_event(&event);
                    break;
                }
                case MotionNotify:
                {
                    mgapp_state.input.mouse.x = (int16_t)xev.xmotion.x;
                    mgapp_state.input.mouse.y = (int16_t)xev.xmotion.y;
                    mgapp_event event = {
                        .mouse = {
                            .x = xev.xmotion.x,
                            .y = xev.xmotion.y
                        },
                        .type = MGAPP_EVENT_MOUSE_MOVE
                    };
                    mgapp_call_event(&event);
                    break;
                }
            }
        }

        float new_time = mgapp_time();
        mgapp_state.delta_time = new_time - mgapp_state.frame_time;
        mgapp_state.frame_time = new_time;

        if (info->events.update)
            info->events.update();
        
        mgapp_input_frame();
    }

    if (info->events.end)
        info->events.end();

    XDestroyWindow(xlib_state->display, xlib_state->window);
    XCloseDisplay(xlib_state->display);

    return 0;
}

void mgapp_xlib_maximize(void)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;

    XEvent xev = {0};
    xev.type = ClientMessage;
    xev.xclient.window = xlib_state->window;
    xev.xclient.message_type = xlib_state->net_wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = (long)xlib_state->net_wm_state_maximized_horz;
    xev.xclient.data.l[2] = (long)xlib_state->net_wm_state_maximized_vert;
    xev.xclient.data.l[3] = 1;

    XSendEvent(xlib_state->display, RootWindow(xlib_state->display, xlib_state->screen),
        False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    XFlush(xlib_state->display);
    mgapp_state._linux.maximized = true;
}

void mgapp_xlib_minimize(void)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    XIconifyWindow(xlib_state->display, xlib_state->window, xlib_state->screen);
    XFlush(xlib_state->display);
}

void mgapp_xlib_restore(void)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;

    XEvent xev = {0};
    xev.type = ClientMessage;
    xev.xclient.window = xlib_state->window;
    xev.xclient.message_type = xlib_state->net_wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 0;
    xev.xclient.data.l[1] = (long)xlib_state->net_wm_state_maximized_horz;
    xev.xclient.data.l[2] = (long)xlib_state->net_wm_state_maximized_vert;
    xev.xclient.data.l[3] = 1;

    XSendEvent(xlib_state->display, RootWindow(xlib_state->display, xlib_state->screen),
        False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    XMapWindow(xlib_state->display, xlib_state->window);
    XFlush(xlib_state->display);
    mgapp_state._linux.maximized = false;
}

void mgapp_xlib_show(bool value)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    if (value)
        XMapWindow(xlib_state->display, xlib_state->window);
    else
        XUnmapWindow(xlib_state->display, xlib_state->window);
    XFlush(xlib_state->display);
}

void mgapp_xlib_close(void)
{
    mgapp_event event = { .type = MGAPP_EVENT_WINDOW_CLOSE };
    mgapp_call_event(&event);
    mgapp_state.running = false;
}

void mgapp_xlib_set_cursor(mg_cursor cursor)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    XDefineCursor(xlib_state->display, xlib_state->window, xlib_state->cursor_table[cursor]);
    XFlush(xlib_state->display);
}

void *mgapp_xlib_primary_handle(void)
{
    return (void*)mgapp_state._linux.xlib.window;
}

void *mgapp_xlib_secondary_handle(void)
{
    return (void*)mgapp_state._linux.xlib.display;
}

static int32_t mgapp_wayland_run(const mgapp_init_info *info)
{
    return 0;
}

static inline mgapp_linux_state_type mgapp_linux_get_state_type(void)
{
    const char *session = getenv("XDG_SESSION_TYPE");
    return (session && strcmp(session, "wayland") == 0)
        ? MGAPP_LINUX_STATE_WAYLAND
        : MGAPP_LINUX_STATE_XLIB;
}

int32_t mgapp_run(const mgapp_init_info *info)
{
    mgapp_linux_state *state = &mgapp_state._linux;
    
    state->type = mgapp_linux_get_state_type();

    if (state->type == MGAPP_LINUX_STATE_XLIB)
    {
        state->run = mgapp_xlib_run;
        state->primary_handle = mgapp_xlib_primary_handle;
        state->secondary_handle = mgapp_xlib_secondary_handle;
        state->maximize = mgapp_xlib_maximize;
        state->minimize = mgapp_xlib_minimize;
        state->restore = mgapp_xlib_restore;
        state->show = mgapp_xlib_show;
        state->close = mgapp_xlib_close;
        state->set_cursor = mgapp_xlib_set_cursor;
    }
    else if (state->type == MGAPP_LINUX_STATE_WAYLAND)
    {
        state->run = mgapp_wayland_run;
    }
    
    mgapp_state.events = info->events;
    mgapp_state.flags = info->flags;

    return state->run(info);
}

void mgapp_maximize(void)
{
    mgapp_state._linux.maximize();
}

void mgapp_minimize(void)
{
    mgapp_state._linux.minimize();
}

bool mgapp_maximized(void)
{
    return mgapp_state._linux.maximized;
}

void mgapp_close(void)
{
    mgapp_state._linux.close();
}

void mgapp_show(bool value)
{
    mgapp_state._linux.show(value);
}

void mgapp_restore(void)
{
    mgapp_state._linux.restore();
}

void mgapp_set_cursor(mg_cursor cursor)
{
    if (mgapp_state.current_cursor == cursor)
        return;
    mgapp_state._linux.set_cursor(cursor);
    mgapp_state.current_cursor = cursor;
}

void *mgapp_primary_handle(void)
{
    return mgapp_state._linux.primary_handle();
}

void *mgapp_secondary_handle(void)
{
    return mgapp_state._linux.secondary_handle();
}

float mgapp_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (float)ts.tv_sec + (float)ts.tv_nsec * 1e-9f;
}

#endif

#endif