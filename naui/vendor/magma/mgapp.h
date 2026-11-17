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
    MGAPP_EVENT_WINDOW_CLOSE,
    MGAPP_EVENT_WINDOW_CLOSE_REQUEST
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
    #define MGAPP_MAX_CODEPOINTS 256
#endif

#ifndef MGAPP_IME_MAX_TEXT
    #define MGAPP_IME_MAX_TEXT 1024
#endif

#ifndef MGAPP_IME_MAX_CLAUSES
    #define MGAPP_IME_MAX_CLAUSES 32
#endif

MGAPP_API int32_t mgapp_run(const mgapp_init_info *info);
MGAPP_API void mgapp_close(void);
MGAPP_API void mgapp_cancel_close(void);
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
MGAPP_API bool mgapp_key_pressed_repeat(mg_key key);
MGAPP_API bool mgapp_key_released(mg_key key);

MGAPP_API uint32_t mgapp_codepoint(void);

typedef uint8_t mgapp_ime_mode;
enum
{
    MGAPP_IME_MODE_OFF,
    MGAPP_IME_MODE_TEXT,
    MGAPP_IME_MODE_NUMBER,
    MGAPP_IME_MODE_PASSWORD
};

typedef struct
{
    int32_t start;
    int32_t end;
    bool target;
}
mgapp_ime_clause;

typedef struct
{
    const char *text;
    int32_t length;
    int32_t cursor;
    const mgapp_ime_clause *clauses;
    int32_t clause_count;
}
mgapp_ime_composition;

MGAPP_API void mgapp_ime_set_mode(mgapp_ime_mode mode);
MGAPP_API mgapp_ime_mode mgapp_ime_get_mode(void);
MGAPP_API void mgapp_ime_set_area(int32_t x, int32_t y, int32_t width, int32_t height, int32_t caret_x);
MGAPP_API bool mgapp_ime_get_composition(mgapp_ime_composition *out);
MGAPP_API void mgapp_ime_cancel(void);

MGAPP_API bool mgapp_mouse_down(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_pressed(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_released(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_clicked(mg_mouse_button button);
MGAPP_API bool mgapp_mouse_double_clicked(mg_mouse_button button);

MGAPP_API int8_t mgapp_mouse_scroll_delta(void);
MGAPP_API int32_t mgapp_mouse_x(void);
MGAPP_API int32_t mgapp_mouse_y(void);
MGAPP_API bool mgapp_mouse_inside(void);

MGAPP_API uint32_t mgapp_dpi(void);
MGAPP_API float mgapp_dpi_scale(void);

MGAPP_API void mgapp_set_clipboard(const char *text);
MGAPP_API const char *mgapp_get_clipboard(void);

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
        bool inside;

        bool buttons[4];
    }
    mouse;

    struct
    {
        struct
        {
            bool keys_pressed[MG_KEY_MAX];
            bool keys_pressed_repeat[MG_KEY_MAX];
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
#include <imm.h>
#pragma comment(lib, "user32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "imm32")

#define MGAPP_IME_MAX_UTF16 512
#define MGAPP_IS_SURROGATE(unit) (IS_HIGH_SURROGATE(unit) || IS_LOW_SURROGATE(unit))
#define MGAPP_WIN32_IS_TARGET(attribute) ((attribute) == ATTR_TARGET_CONVERTED || (attribute) == ATTR_TARGET_NOTCONVERTED)

typedef struct
{
    HWND hwnd;
    HINSTANCE hinstance;

    WNDPROC original_proc;

    HCURSOR cursor_table[MG_CURSOR_MAX];
    bool tracking_mouse;

    HIMC ime_himc;

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

#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

#define MGAPP_XLIB_IME_MAX_CHARS 256

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

    Atom clipboard_atom;
    Atom utf8_string_atom;
    Atom targets_atom;
    Atom clipboard_prop_atom;

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

    char *clipboard_owned;
    size_t clipboard_owned_len;

    Cursor cursor_table[MG_CURSOR_MAX];

    long event_mask;
    bool focused;

    XIM xim;
    XIC xic;
    XIMStyle xim_style;
    XFontSet ime_fontset;
    XICCallback preedit_callbacks[4];
    uint32_t preedit_chars[MGAPP_XLIB_IME_MAX_CHARS];
    XIMFeedback preedit_feedback[MGAPP_XLIB_IME_MAX_CHARS];
    int32_t preedit_length;
    int32_t preedit_caret;
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
    void (*set_clipboard)(const char*);
    const char *(*get_clipboard)(void);
    void (*ime_set_mode)(mgapp_ime_mode);
    void (*ime_cancel)(void);
    void (*ime_apply_area)(void);

    bool maximized;
}
mgapp_linux_state;

#elif defined(__APPLE__)

#if !defined(__OBJC__)
#error "The mgapp macOS backend must be compiled as Objective-C (include the implementation from a .m file)."
#endif

#if __has_feature(objc_arc)
#error "The mgapp macOS backend uses manual reference counting; compile it without -fobjc-arc."
#endif

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#ifndef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif
#include <Carbon/Carbon.h>
#include <mach/mach_time.h>
#include <stdlib.h>

#ifndef MGAPP_MACOS_SCROLL_STEP
    #define MGAPP_MACOS_SCROLL_STEP 10.0f
#endif

#ifndef MGAPP_MACOS_RESIZE_BORDER
    #define MGAPP_MACOS_RESIZE_BORDER 6.0f
#endif

#ifndef MGAPP_MACOS_MIN_SIZE
    #define MGAPP_MACOS_MIN_SIZE 64.0f
#endif

typedef uint8_t mgapp_macos_edge;
enum
{
    MGAPP_MACOS_EDGE_NONE = 0,
    MGAPP_MACOS_EDGE_LEFT = 1 << 0,
    MGAPP_MACOS_EDGE_RIGHT = 1 << 1,
    MGAPP_MACOS_EDGE_TOP = 1 << 2,
    MGAPP_MACOS_EDGE_BOTTOM = 1 << 3
};

typedef uint8_t mgapp_macos_drag_mode;
enum
{
    MGAPP_MACOS_DRAG_NONE,
    MGAPP_MACOS_DRAG_MOVE,
    MGAPP_MACOS_DRAG_RESIZE
};

typedef struct
{
    NSWindow *window;
    NSView *view;
    CAMetalLayer *layer;
    id delegate;

    NSCursor *cursor_table[MG_CURSOR_MAX];
    bool cursor_hidden;

    struct
    {
        mgapp_macos_drag_mode mode;
        mgapp_macos_edge edges;
        NSPoint start_mouse;
        NSRect start_frame;
    }
    drag;

    NSRect restore_frame;
    bool maximized;

    NSUInteger marked_length;
    bool ime_handled;

    float scroll_accumulator;
    float backing_scale;

    double clock_scale;
    uint64_t start_time;
}
mgapp_macos_state;

#endif

typedef struct
{
#if defined(__EMSCRIPTEN__)
    mgapp_emscripten_state emscripten;
#elif defined(_WIN32)
    mgapp_win32_state win32;
#elif defined(__linux__)
    mgapp_linux_state _linux;
#elif defined(__APPLE__)
    mgapp_macos_state macos;
#endif
    mgapp_input_state input;

    struct
    {
        mgapp_ime_mode mode;
        bool composing;
        char text[MGAPP_IME_MAX_TEXT];
        int32_t length, cursor;
        mgapp_ime_clause clauses[MGAPP_IME_MAX_CLAUSES];
        int32_t clause_count;
        int32_t area_x, area_y, area_width, area_height, caret_x;
    }
    ime;

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
    char *clipboard_buffer;

    bool running;
    bool close_cancelled;
}
mgapp_core_state;
static mgapp_core_state mgapp_state;

static inline void mgapp_call_event(const mgapp_event *event)
{
    if (mgapp_state.events.event)
        mgapp_state.events.event(event);
}

static inline bool mgapp_request_close(void)
{
    mgapp_state.close_cancelled = false;
    mgapp_event event = { .type = MGAPP_EVENT_WINDOW_CLOSE_REQUEST };
    mgapp_call_event(&event);
    return !mgapp_state.close_cancelled;
}

void mgapp_cancel_close(void)
{
    mgapp_state.close_cancelled = true;
}

static inline bool mgapp_modifier_pair(mg_key key, mg_key *generic, mg_key *other)
{
    switch (key)
    {
        case MG_KEY_LSHIFT:
            *generic = MG_KEY_SHIFT;
            *other = MG_KEY_RSHIFT;
            return true;
            
        case MG_KEY_RSHIFT:
            *generic = MG_KEY_SHIFT;
            *other = MG_KEY_LSHIFT;
            return true;
            
        case MG_KEY_LCONTROL:
            *generic = MG_KEY_CONTROL;
            *other = MG_KEY_RCONTROL;
            return true;
            
        case MG_KEY_RCONTROL:
            *generic = MG_KEY_CONTROL;
            *other = MG_KEY_LCONTROL;
            return true;
            
        case MG_KEY_LALT:
            *generic = MG_KEY_ALT;
            *other = MG_KEY_RALT;
            return true;
            
        case MG_KEY_RALT:
            *generic = MG_KEY_ALT;
            *other = MG_KEY_LALT;
            return true;
            
        default: return false;
    }
}

static inline void mgapp_input_process_key(mg_key key, bool pressed)
{
    if (pressed)
    {
        mgapp_state.input.clear.keyboard.keys_pressed_repeat[key] = true;

        if (!mgapp_state.input.clear.keyboard.keys_released[key])
            mgapp_state.input.clear.keyboard.keys_pressed[key] = true;
    }

    if (!pressed && mgapp_state.input.keyboard.keys[key])
        mgapp_state.input.clear.keyboard.keys_released[key] = true;

    mgapp_state.input.keyboard.keys[key] = pressed;
    mg_key generic, other;
    if (mgapp_modifier_pair(key, &generic, &other))
    {
        bool was_down = mgapp_state.input.keyboard.keys[generic];
        bool now_down = pressed || mgapp_state.input.keyboard.keys[other];
        if (pressed)
        {
            mgapp_state.input.clear.keyboard.keys_pressed_repeat[generic] = true;

            if (!was_down && !mgapp_state.input.clear.keyboard.keys_released[generic])
                mgapp_state.input.clear.keyboard.keys_pressed[generic] = true;
        }

        if (was_down && !now_down)
            mgapp_state.input.clear.keyboard.keys_released[generic] = true;

        mgapp_state.input.keyboard.keys[generic] = now_down;
    }
}

static inline void mgapp_input_release_all_keys(void)
{
    for (int32_t key = 0; key < MG_KEY_MAX; ++key)
    {
        if (!mgapp_state.input.keyboard.keys[key])
            continue;

        mgapp_state.input.keyboard.keys[key] = false;
        mgapp_state.input.clear.keyboard.keys_released[key] = true;
        mgapp_event event = { .key = (mg_key)key, .type = MGAPP_EVENT_KEY_UP };
        mgapp_call_event(&event);
    }
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

bool mgapp_key_pressed_repeat(mg_key key)
{
    return mgapp_state.input.clear.keyboard.keys_pressed_repeat[key];
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

static void mgapp_platform_ime_set_mode(mgapp_ime_mode mode);
static void mgapp_platform_ime_cancel(void);
static void mgapp_platform_ime_apply_area(void);

static inline void mgapp_ime_clear_composition(void)
{
    mgapp_state.ime.composing = false;
    mgapp_state.ime.text[0] = '\0';
    mgapp_state.ime.length = 0;
    mgapp_state.ime.cursor = 0;
    mgapp_state.ime.clause_count = 0;
}

static inline void mgapp_ime_push_clause(int32_t start, int32_t end, bool target)
{
    if (end <= start)
        return;

    if (mgapp_state.ime.clause_count == MGAPP_IME_MAX_CLAUSES)
    {
        mgapp_ime_clause *last = &mgapp_state.ime.clauses[MGAPP_IME_MAX_CLAUSES - 1];
        last->end = end;
        last->target = last->target || target;
        return;
    }

    mgapp_state.ime.clauses[mgapp_state.ime.clause_count++] = (mgapp_ime_clause){ .start = start, .end = end, .target = target };
}

void mgapp_ime_set_mode(mgapp_ime_mode mode)
{
    if (mgapp_state.ime.mode == mode)
        return;

    mgapp_state.ime.mode = mode;
    if (mode != MGAPP_IME_MODE_TEXT)
        mgapp_ime_clear_composition();

    mgapp_platform_ime_set_mode(mode);
}

mgapp_ime_mode mgapp_ime_get_mode(void)
{
    return mgapp_state.ime.mode;
}

void mgapp_ime_set_area(int32_t x, int32_t y, int32_t width, int32_t height, int32_t caret_x)
{
    if (mgapp_state.ime.area_x == x && mgapp_state.ime.area_y == y &&
        mgapp_state.ime.area_width == width && mgapp_state.ime.area_height == height &&
        mgapp_state.ime.caret_x == caret_x)
        return;

    mgapp_state.ime.area_x = x;
    mgapp_state.ime.area_y = y;
    mgapp_state.ime.area_width = width;
    mgapp_state.ime.area_height = height;
    mgapp_state.ime.caret_x = caret_x;
    mgapp_platform_ime_apply_area();
}

bool mgapp_ime_get_composition(mgapp_ime_composition *out)
{
    if (mgapp_state.ime.mode != MGAPP_IME_MODE_TEXT || mgapp_state.ime.length <= 0)
        return false;

    if (mgapp_state.ime.clause_count == 0)
        mgapp_ime_push_clause(0, mgapp_state.ime.length, false);

    if (out)
    {
        out->text = mgapp_state.ime.text;
        out->length = mgapp_state.ime.length;
        out->cursor = mgapp_state.ime.cursor;
        out->clauses = mgapp_state.ime.clauses;
        out->clause_count = mgapp_state.ime.clause_count;
    }

    return true;
}

void mgapp_ime_cancel(void)
{
    mgapp_platform_ime_cancel();
    mgapp_ime_clear_composition();
}

static int32_t mgapp_utf8_encode(uint32_t codepoint, char *dst)
{
    if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
        codepoint = 0xFFFD;

    if (codepoint < 0x80)
    {
        dst[0] = (char)codepoint;
        return 1;
    }
    if (codepoint < 0x800)
    {
        dst[0] = (char)(0xC0 | (codepoint >> 6));
        dst[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    }
    if (codepoint < 0x10000)
    {
        dst[0] = (char)(0xE0 | (codepoint >> 12));
        dst[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        dst[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    }
    dst[0] = (char)(0xF0 | (codepoint >> 18));
    dst[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
    dst[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
    dst[3] = (char)(0x80 | (codepoint & 0x3F));
    return 4;
}

static int32_t mgapp_utf8_decode(const char *text, int32_t length, uint32_t *codepoint)
{
    const unsigned char *src = (const unsigned char*)text;
    const unsigned char lead = src[0];
    int32_t units;
    uint32_t value;

    if (lead < 0x80)
    {
        *codepoint = lead;
        return 1;
    }
    else if ((lead & 0xE0) == 0xC0) { units = 2; value = lead & 0x1F; }
    else if ((lead & 0xF0) == 0xE0) { units = 3; value = lead & 0x0F; }
    else if ((lead & 0xF8) == 0xF0) { units = 4; value = lead & 0x07; }
    else
    {
        *codepoint = 0xFFFD;
        return 1;
    }

    if (units > length)
    {
        *codepoint = 0xFFFD;
        return length;
    }

    for (int32_t i = 1; i < units; ++i)
    {
        if ((src[i] & 0xC0) != 0x80)
        {
            *codepoint = 0xFFFD;
            return i;
        }
        value = (value << 6) | (src[i] & 0x3F);
    }

    *codepoint = value;
    return units;
}

static void mgapp_emit_char(uint32_t codepoint)
{
    mgapp_input_process_char(codepoint);
    mgapp_event event = { .codepoint = codepoint, .type = MGAPP_EVENT_CHAR };
    mgapp_call_event(&event);
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

bool mgapp_mouse_inside(void)
{
    return mgapp_state.input.mouse.inside;
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

static inline void mgapp_clipboard_cache(const char *text, size_t len)
{
    if (!text)
    {
        free(mgapp_state.clipboard_buffer);
        mgapp_state.clipboard_buffer = NULL;
        return;
    }

    if (text == mgapp_state.clipboard_buffer)
    {
        mgapp_state.clipboard_buffer[len] = '\0';
        return;
    }

    char *buf = (char*)realloc(mgapp_state.clipboard_buffer, len + 1);
    if (!buf)
    {
        free(mgapp_state.clipboard_buffer);
        mgapp_state.clipboard_buffer = NULL;
        return;
    }

    memcpy(buf, text, len);
    buf[len] = '\0';
    mgapp_state.clipboard_buffer = buf;
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

static bool mgapp_emscripten_ime_focused(void)
{
    return mgapp_state.ime.mode == MGAPP_IME_MODE_TEXT && EM_ASM_INT({
        var ime = Module.mgappIme;
        return ime && document.activeElement === ime.textarea ? 1 : 0;
    });
}

static EM_BOOL mgapp_emscripten_key_callback(int32_t event_type, const EmscriptenKeyboardEvent *e, void *user_data)
{
    bool pressed = event_type == EMSCRIPTEN_EVENT_KEYDOWN;
    const bool ime_focused = mgapp_emscripten_ime_focused();

    if (pressed && ime_focused &&
        (mgapp_state.ime.composing || e->keyCode == 229 || strcmp(e->key, "Process") == 0 || strcmp(e->key, "Dead") == 0))
        return EM_FALSE;

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

    bool shortcut = (e->ctrlKey || e->metaKey) && !e->altKey;

    const int32_t key_length = (int32_t)strlen(e->key);
    uint32_t codepoint = 0;
    const bool printable =
        pressed && !shortcut && key_length > 0 &&
        mgapp_utf8_decode(e->key, key_length, &codepoint) == key_length &&
        codepoint >= 0x20 && codepoint != 0x7F;

    if (printable)
    {
        if (ime_focused)
            return EM_FALSE;
        mgapp_emit_char(codepoint);
    }

    if (pressed && (e->ctrlKey || e->metaKey) && strcmp(e->code, "KeyV") == 0)
        return EM_FALSE;

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

static EM_BOOL mgapp_emscripten_mouse_cross_callback(int32_t event_type, const EmscriptenMouseEvent *e, void *user_data)
{
    mgapp_state.input.mouse.inside = event_type == EMSCRIPTEN_EVENT_MOUSEENTER;
    return EM_TRUE;
}

static EM_BOOL mgapp_emscripten_mouse_move_callback(int32_t event_type, const EmscriptenMouseEvent *e, void *user_data)
{
    mgapp_state.input.mouse.inside = true;
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

EMSCRIPTEN_KEEPALIVE void mgapp_emscripten_on_paste(const char *text)
{
    mgapp_clipboard_cache(text, strlen(text));
}

EMSCRIPTEN_KEEPALIVE void mgapp_emscripten_ime_begin(void)
{
    mgapp_ime_clear_composition();
    mgapp_state.ime.composing = true;
}

EMSCRIPTEN_KEEPALIVE void mgapp_emscripten_ime_preedit(const char *text, int32_t cursor)
{
    if (mgapp_state.ime.mode != MGAPP_IME_MODE_TEXT)
        return;

    int32_t length = (int32_t)strlen(text);
    if (length >= MGAPP_IME_MAX_TEXT)
    {
        length = MGAPP_IME_MAX_TEXT - 1;
        while (length > 0 && ((unsigned char)text[length] & 0xC0) == 0x80)
            --length;
    }

    memcpy(mgapp_state.ime.text, text, (size_t)length);
    mgapp_state.ime.text[length] = '\0';
    mgapp_state.ime.composing = true;
    mgapp_state.ime.length = length;
    mgapp_state.ime.cursor = cursor < 0 ? 0 : cursor > length ? length : cursor;
    mgapp_state.ime.clause_count = 0;
    mgapp_ime_push_clause(0, length, false);
}

EMSCRIPTEN_KEEPALIVE void mgapp_emscripten_ime_end(void)
{
    mgapp_ime_clear_composition();
}

EMSCRIPTEN_KEEPALIVE void mgapp_emscripten_ime_commit(const char *text)
{
    const int32_t length = (int32_t)strlen(text);
    int32_t i = 0;
    while (i < length)
    {
        uint32_t codepoint;
        i += mgapp_utf8_decode(text + i, length - i, &codepoint);
        if (codepoint >= 0x20 && codepoint != 0x7F)
            mgapp_emit_char(codepoint);
    }
}

static void mgapp_emscripten_ime_setup(void)
{
    EM_ASM({
        var textarea = document.createElement("textarea");
        textarea.setAttribute("autocomplete", "off");
        textarea.setAttribute("autocorrect", "off");
        textarea.setAttribute("autocapitalize", "off");
        textarea.setAttribute("aria-hidden", "true");
        textarea.spellcheck = false;
        textarea.tabIndex = -1;

        var style = textarea.style;
        style.position = "fixed";
        style.left = "0px";
        style.top = "0px";
        style.width = "1px";
        style.height = "1px";
        style.margin = "0";
        style.padding = "0";
        style.border = "0";
        style.outline = "none";
        style.resize = "none";
        style.overflow = "hidden";
        style.whiteSpace = "pre";
        style.opacity = "0";
        style.color = "transparent";
        style.background = "transparent";
        style.caretColor = "transparent";
        style.pointerEvents = "none";
        document.body.appendChild(textarea);

        var ime = {
            canvas: document.querySelector(UTF8ToString($0)),
            textarea: textarea,
            enabled: false,
            composing: false,
            discarding: false,
            committed: 0
        };
        Module.mgappIme = ime;

        ime.withString = function(text, callback) {
            var size = lengthBytesUTF8(text) + 1;
            var ptr = _malloc(size);
            stringToUTF8(text, ptr, size);
            callback(ptr);
            _free(ptr);
        };

        ime.commit = function(text) {
            if (text.length > 0)
                ime.withString(text, function(ptr) { _mgapp_emscripten_ime_commit(ptr); });
        };

        ime.reset = function() {
            textarea.value = "";
            ime.committed = 0;
        };

        ime.flush = function() {
            if (ime.composing)
                return;
            var text = textarea.value.substring(ime.committed);
            ime.reset();
            ime.commit(text);
        };

        ime.focus = function() {
            if (ime.enabled && document.activeElement !== textarea)
                textarea.focus({ preventScroll: true });
        };

        ime.cancel = function() {
            if (!ime.composing) {
                ime.reset();
                return;
            }
            ime.discarding = true;
            ime.composing = false;
            textarea.blur();
            ime.reset();
            ime.focus();
            setTimeout(function() {
                ime.discarding = false;
                ime.reset();
            }, 0);
        };

        ime.setEnabled = function(enabled) {
            ime.enabled = enabled;
            if (enabled) {
                ime.reset();
                ime.focus();
            } else {
                ime.cancel();
                textarea.blur();
            }
        };

        ime.setRect = function(x, y, width, height) {
            if (!ime.canvas)
                return;
            var rect = ime.canvas.getBoundingClientRect();
            var scale_x = ime.canvas.width ? rect.width / ime.canvas.width : 1;
            var scale_y = ime.canvas.height ? rect.height / ime.canvas.height : 1;
            var line = Math.max(1, height * scale_y);
            style.left = (rect.left + x * scale_x) + "px";
            style.top = (rect.top + y * scale_y) + "px";
            style.width = Math.max(1, width * scale_x) + "px";
            style.height = line + "px";
            style.fontSize = line + "px";
            style.lineHeight = line + "px";
        };

        textarea.addEventListener("compositionstart", function() {
            ime.discarding = false;
            ime.commit(textarea.value.substring(ime.committed));
            ime.committed = textarea.value.length;
            ime.composing = true;
            _mgapp_emscripten_ime_begin();
        });

        textarea.addEventListener("input", function() {
            if (ime.discarding) {
                ime.reset();
                return;
            }
            if (!ime.composing) {
                ime.flush();
                return;
            }
            var value = textarea.value.substring(ime.committed);
            var caret = Math.max(0, Math.min(value.length, textarea.selectionEnd - ime.committed));
            var caret_bytes = lengthBytesUTF8(value.substring(0, caret));
            ime.withString(value, function(ptr) { _mgapp_emscripten_ime_preedit(ptr, caret_bytes); });
        });

        textarea.addEventListener("compositionend", function() {
            _mgapp_emscripten_ime_end();
            if (ime.discarding) {
                ime.discarding = false;
                ime.reset();
                return;
            }
            ime.composing = false;
            setTimeout(ime.flush, 0);
        });

        textarea.addEventListener("paste", function(e) { e.preventDefault(); });

        window.addEventListener("focus", function() { ime.focus(); });
        if (ime.canvas)
            ime.canvas.addEventListener("mousedown", function() { ime.focus(); });
    }, MGAPP_EMSCRIPTEN_CANVAS);
}

static void mgapp_platform_ime_set_mode(mgapp_ime_mode mode)
{
    const bool enabled = mode == MGAPP_IME_MODE_TEXT;
    EM_ASM({
        if (Module.mgappIme)
            Module.mgappIme.setEnabled(!!$0);
    }, enabled);

    if (enabled)
        mgapp_platform_ime_apply_area();
}

static void mgapp_platform_ime_cancel(void)
{
    EM_ASM({
        if (Module.mgappIme)
            Module.mgappIme.cancel();
    });
}

static void mgapp_platform_ime_apply_area(void)
{
    EM_ASM({
        if (Module.mgappIme)
            Module.mgappIme.setRect($0, $1, $2, $3);
    }, mgapp_state.ime.area_x + mgapp_state.ime.caret_x, mgapp_state.ime.area_y, 1, mgapp_state.ime.area_height);
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
    emscripten_set_mouseenter_callback(MGAPP_EMSCRIPTEN_CANVAS, NULL, EM_TRUE, mgapp_emscripten_mouse_cross_callback);
    emscripten_set_mouseleave_callback(MGAPP_EMSCRIPTEN_CANVAS, NULL, EM_TRUE, mgapp_emscripten_mouse_cross_callback);
    emscripten_set_wheel_callback(MGAPP_EMSCRIPTEN_CANVAS, NULL, EM_TRUE, mgapp_emscripten_wheel_callback);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, mgapp_emscripten_resize_callback);

    mgapp_emscripten_ime_setup();

    EM_ASM({
        window.addEventListener("paste", function(e) {
            var text = e.clipboardData ? e.clipboardData.getData("text/plain") : "";
            var size = lengthBytesUTF8(text) + 1;
            var ptr = _malloc(size);
            stringToUTF8(text, ptr, size);
            _mgapp_emscripten_on_paste(ptr);
            _free(ptr);
        });
    });

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

void mgapp_set_clipboard(const char *text)
{
    if (!text)
        return;

    mgapp_clipboard_cache(text, strlen(text));
    EM_ASM({
        var text = UTF8ToString($0);
        if (navigator.clipboard && navigator.clipboard.writeText)
            navigator.clipboard.writeText(text).catch(function() {});
    }, text);
}

const char *mgapp_get_clipboard(void)
{
    return mgapp_state.clipboard_buffer;
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
    win32_state->cursor_table[MG_CURSOR_ARROW]        = LoadCursor(NULL, IDC_ARROW);
    win32_state->cursor_table[MG_CURSOR_IBEAM]        = LoadCursor(NULL, IDC_IBEAM);
    win32_state->cursor_table[MG_CURSOR_CROSSHAIR]    = LoadCursor(NULL, IDC_CROSS);
    win32_state->cursor_table[MG_CURSOR_HAND]         = LoadCursor(NULL, IDC_HAND);
    win32_state->cursor_table[MG_CURSOR_RESIZE_NS]    = LoadCursor(NULL, IDC_SIZENS);
    win32_state->cursor_table[MG_CURSOR_RESIZE_EW]    = LoadCursor(NULL, IDC_SIZEWE);
    win32_state->cursor_table[MG_CURSOR_RESIZE_NESW]  = LoadCursor(NULL, IDC_SIZENESW);
    win32_state->cursor_table[MG_CURSOR_RESIZE_NWSE]  = LoadCursor(NULL, IDC_SIZENWSE);
    win32_state->cursor_table[MG_CURSOR_RESIZE_ALL]   = LoadCursor(NULL, IDC_SIZEALL);
    win32_state->cursor_table[MG_CURSOR_NOT_ALLOWED]  = LoadCursor(NULL, IDC_NO);
    win32_state->cursor_table[MG_CURSOR_HIDDEN]       = NULL;
}

static int32_t mgapp_win32_utf16_to_utf8(const WCHAR *src, int32_t src_len, char *dst, int32_t dst_capacity, int32_t *offsets)
{
    int32_t out = 0;
    int32_t i = 0;
    for (; i < src_len; ++i)
    {
        uint32_t codepoint = src[i];
        int32_t units = 1;
        if (IS_HIGH_SURROGATE(src[i]) && i + 1 < src_len && IS_LOW_SURROGATE(src[i + 1]))
        {
            codepoint = 0x10000u + (((uint32_t)src[i] - 0xD800u) << 10) + ((uint32_t)src[i + 1] - 0xDC00u);
            units = 2;
        }
        else if (MGAPP_IS_SURROGATE(src[i]))
        {
            codepoint = 0xFFFD;
        }

        const int32_t bytes = codepoint < 0x80 ? 1 : codepoint < 0x800 ? 2 : codepoint < 0x10000 ? 3 : 4;
        if (out + bytes >= dst_capacity)
            break;

        if (offsets)
        {
            offsets[i] = out;
            if (units == 2)
                offsets[i + 1] = out;
        }

        if (bytes == 1)
            dst[out] = (char)codepoint;
        else if (bytes == 2)
        {
            dst[out]     = (char)(0xC0 | (codepoint >> 6));
            dst[out + 1] = (char)(0x80 | (codepoint & 0x3F));
        }
        else if (bytes == 3)
        {
            dst[out]     = (char)(0xE0 | (codepoint >> 12));
            dst[out + 1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            dst[out + 2] = (char)(0x80 | (codepoint & 0x3F));
        }
        else
        {
            dst[out]     = (char)(0xF0 | (codepoint >> 18));
            dst[out + 1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
            dst[out + 2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            dst[out + 3] = (char)(0x80 | (codepoint & 0x3F));
        }

        out += bytes;
        i += units - 1;
    }

    if (offsets)
    {
        for (int32_t k = i; k <= src_len; ++k)
            offsets[k] = out;
    }

    dst[out] = '\0';
    return out;
}

static void mgapp_win32_ime_commit(const WCHAR *text, int32_t length)
{
    for (int32_t i = 0; i < length; ++i)
    {
        uint32_t codepoint = text[i];
        if (IS_HIGH_SURROGATE(text[i]) && i + 1 < length && IS_LOW_SURROGATE(text[i + 1]))
        {
            codepoint = 0x10000u + (((uint32_t)text[i] - 0xD800u) << 10) + ((uint32_t)text[i + 1] - 0xDC00u);
            ++i;
        }
        else if (MGAPP_IS_SURROGATE(text[i]))
        {
            continue;
        }

        mgapp_input_process_char(codepoint);
        mgapp_event event = { .codepoint = codepoint, .type = MGAPP_EVENT_CHAR };
        mgapp_call_event(&event);
    }
}

static void mgapp_win32_ime_composition(HWND hwnd, LPARAM flags)
{
    HIMC himc = ImmGetContext(hwnd);
    if (!himc)
        return;

    if (flags & GCS_RESULTSTR)
    {
        WCHAR result[MGAPP_IME_MAX_UTF16];
        LONG bytes = ImmGetCompositionStringW(himc, GCS_RESULTSTR, result, sizeof(result));
        if (bytes > (LONG)sizeof(result))
            bytes = (LONG)sizeof(result);
        if (bytes > 0)
            mgapp_win32_ime_commit(result, bytes / (LONG)sizeof(WCHAR));
    }

    if (flags & GCS_COMPSTR)
    {
        WCHAR comp[MGAPP_IME_MAX_UTF16];
        LONG bytes = ImmGetCompositionStringW(himc, GCS_COMPSTR, comp, sizeof(comp));
        if (bytes > (LONG)sizeof(comp))
            bytes = (LONG)sizeof(comp);
        const int32_t length = bytes > 0 ? (int32_t)(bytes / (LONG)sizeof(WCHAR)) : 0;

        if (length == 0)
        {
            mgapp_ime_clear_composition();
            mgapp_state.ime.composing = true;
        }
        else
        {
            int32_t offsets[MGAPP_IME_MAX_UTF16 + 1];
            mgapp_state.ime.length = mgapp_win32_utf16_to_utf8(comp, length, mgapp_state.ime.text, MGAPP_IME_MAX_TEXT, offsets);
            mgapp_state.ime.composing = true;

            LONG cursor = ImmGetCompositionStringW(himc, GCS_CURSORPOS, NULL, 0);
            if (cursor < 0 || cursor > length)
                cursor = length;
            mgapp_state.ime.cursor = offsets[cursor];

            BYTE attributes[MGAPP_IME_MAX_UTF16];
            LONG attribute_bytes = ImmGetCompositionStringW(himc, GCS_COMPATTR, attributes, sizeof(attributes));
            if (attribute_bytes < 0)
                attribute_bytes = 0;

            DWORD clause_offsets[MGAPP_IME_MAX_CLAUSES + 1];
            int32_t clause_offset_count = 0;
            const LONG clause_bytes = ImmGetCompositionStringW(himc, GCS_COMPCLAUSE, NULL, 0);
            if (clause_bytes > 0 && clause_bytes <= (LONG)sizeof(clause_offsets))
            {
                const LONG read = ImmGetCompositionStringW(himc, GCS_COMPCLAUSE, clause_offsets, sizeof(clause_offsets));
                clause_offset_count = read > 0 ? (int32_t)(read / (LONG)sizeof(DWORD)) : 0;
            }

            mgapp_state.ime.clause_count = 0;
            if (clause_offset_count >= 2)
            {
                for (int32_t i = 0; i + 1 < clause_offset_count; ++i)
                {
                    const int32_t start = (int32_t)(clause_offsets[i] < (DWORD)length ? clause_offsets[i] : (DWORD)length);
                    const int32_t end = (int32_t)(clause_offsets[i + 1] < (DWORD)length ? clause_offsets[i + 1] : (DWORD)length);
                    const BYTE attribute = start < attribute_bytes ? attributes[start] : ATTR_INPUT;
                    mgapp_ime_push_clause(offsets[start], offsets[end], MGAPP_WIN32_IS_TARGET(attribute));
                }
            }
            else
            {
                int32_t start = 0;
                while (start < length)
                {
                    const BYTE attribute = start < attribute_bytes ? attributes[start] : ATTR_INPUT;
                    int32_t end = start + 1;
                    while (end < length && (end < attribute_bytes ? attributes[end] : ATTR_INPUT) == attribute)
                        ++end;
                    mgapp_ime_push_clause(offsets[start], offsets[end], MGAPP_WIN32_IS_TARGET(attribute));
                    start = end;
                }
            }
        }
    }
    else
    {
        mgapp_ime_clear_composition();
    }

    ImmReleaseContext(hwnd, himc);
}

static void mgapp_platform_ime_apply_area(void)
{
    HWND hwnd = mgapp_state.win32.hwnd;
    if (!hwnd)
        return;

    HIMC himc = ImmGetContext(hwnd);
    if (!himc)
        return;

    const int32_t x = mgapp_state.ime.area_x;
    const int32_t y = mgapp_state.ime.area_y;
    const int32_t width = mgapp_state.ime.area_width > 0 ? mgapp_state.ime.area_width : 1;
    const int32_t height = mgapp_state.ime.area_height;
    const int32_t caret_x = x + mgapp_state.ime.caret_x;

    COMPOSITIONFORM composition = { .dwStyle = CFS_POINT, .ptCurrentPos = { caret_x, y } };
    ImmSetCompositionWindow(himc, &composition);

    CANDIDATEFORM candidate = {
        .dwIndex = 0,
        .dwStyle = CFS_EXCLUDE,
        .ptCurrentPos = { caret_x, y + height },
        .rcArea = { x, y, x + width, y + height }
    };
    ImmSetCandidateWindow(himc, &candidate);

    ImmReleaseContext(hwnd, himc);
}

static LRESULT mgapp_win32_ime_query_char_position(HWND hwnd, IMECHARPOSITION *position)
{
    if (mgapp_state.ime.mode != MGAPP_IME_MODE_TEXT || !position || position->dwSize < sizeof(IMECHARPOSITION))
        return 0;

    POINT caret = { mgapp_state.ime.area_x + mgapp_state.ime.caret_x, mgapp_state.ime.area_y };
    POINT origin = { mgapp_state.ime.area_x, mgapp_state.ime.area_y };
    ClientToScreen(hwnd, &caret);
    ClientToScreen(hwnd, &origin);

    position->pt = caret;
    position->cLineHeight = (UINT)(mgapp_state.ime.area_height > 0 ? mgapp_state.ime.area_height : 1);
    position->rcDocument = (RECT){
        origin.x,
        origin.y,
        origin.x + mgapp_state.ime.area_width,
        origin.y + mgapp_state.ime.area_height
    };
    return 1;
}

static void mgapp_platform_ime_cancel(void)
{
    HWND hwnd = mgapp_state.win32.hwnd;
    if (!hwnd)
        return;

    HIMC himc = ImmGetContext(hwnd);
    if (!himc)
        return;

    ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmReleaseContext(hwnd, himc);
}

static void mgapp_platform_ime_set_mode(mgapp_ime_mode mode)
{
    mgapp_win32_state *win32_state = &mgapp_state.win32;
    if (!win32_state->hwnd)
        return;

    if (mode == MGAPP_IME_MODE_TEXT)
    {
        ImmAssociateContext(win32_state->hwnd, win32_state->ime_himc);
        mgapp_platform_ime_apply_area();
    }
    else
    {
        mgapp_platform_ime_cancel();
        ImmAssociateContext(win32_state->hwnd, NULL);
    }
}

static LRESULT CALLBACK mgapp_win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_CLOSE:
        {
            if (!mgapp_request_close())
                return 0;

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
        case WM_KILLFOCUS:
        {
            mgapp_input_release_all_keys();
            break;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            mg_key key = (mg_key)w_param;

            if (w_param == VK_PROCESSKEY && mgapp_state.ime.mode != MGAPP_IME_MODE_TEXT)
            {
                const UINT real_key = ImmGetVirtualKey(hwnd);
                if (real_key != 0 && real_key != VK_PROCESSKEY)
                    key = (mg_key)real_key;
            }

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
        case WM_IME_SETCONTEXT:
        {
            l_param &= ~(LPARAM)ISC_SHOWUICOMPOSITIONWINDOW;
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }
        case WM_IME_STARTCOMPOSITION:
        {
            mgapp_state.ime.composing = true;
            mgapp_platform_ime_apply_area();
            return 0;
        }
        case WM_IME_COMPOSITION:
        {
            mgapp_win32_ime_composition(hwnd, l_param);
            return 0;
        }
        case WM_IME_ENDCOMPOSITION:
        {
            mgapp_ime_clear_composition();
            return 0;
        }
        case WM_IME_CHAR:
            return 0;
        case WM_IME_REQUEST:
        {
            if (w_param == IMR_QUERYCHARPOSITION)
                return mgapp_win32_ime_query_char_position(hwnd, (IMECHARPOSITION*)l_param);
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
                codepoint = 0x10000 + ((pending_high_surrogate - 0xD800) << 10)  + (ch - 0xDC00);
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
            RECT client_rect;
            GetClientRect(hwnd, &client_rect);
            mgapp_state.input.mouse.inside = x >= client_rect.left && x < client_rect.right && y >= client_rect.top && y < client_rect.bottom;
            if (!mgapp_state.win32.tracking_mouse)
            {
                TRACKMOUSEEVENT track = { .cbSize = sizeof(track), .dwFlags = TME_LEAVE, .hwndTrack = hwnd };
                mgapp_state.win32.tracking_mouse = TrackMouseEvent(&track) != 0;
            }
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
        case WM_MOUSELEAVE:
        {
            mgapp_state.win32.tracking_mouse = false;
            mgapp_state.input.mouse.inside = false;
            break;
        }
        case WM_SETCURSOR:
        {
            mgapp_state.input.mouse.inside = LOWORD(l_param) == HTCLIENT;
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
            UINT count = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);

            const size_t path_bytes = (size_t)MAX_PATH * 4;
            size_t ptr_block_size = sizeof(char*) * count;
            size_t str_block_size = path_bytes * count;
            char *block = (char*)malloc(ptr_block_size + str_block_size);

            if (block)
            {
                char **paths = (char**)block;
                char *strings = block + ptr_block_size;

                for (UINT i = 0; i < count; i++)
                {
                    char *dst = strings + (size_t)i * path_bytes;
                    WCHAR wide_path[MAX_PATH];
                    dst[0] = '\0';
                    if (DragQueryFileW(hdrop, i, wide_path, MAX_PATH) > 0)
                        WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, dst, (int)path_bytes, NULL, NULL);
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
        case WM_SYSCOMMAND:
        {
            if ((w_param & 0xFFF0) == SC_KEYMENU && l_param == 0)
                return 0;
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
    return DefWindowProcW(hwnd, msg, w_param, l_param);
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
            mgapp_state.input.mouse.inside = false;
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
    return CallWindowProcW(mgapp_state.win32.original_proc, hwnd, msg, w_param, l_param);
}

int32_t mgapp_run(const mgapp_init_info *info)
{
    mgapp_win32_state *win32_state = &mgapp_state.win32;

    const WCHAR *class_name = L"mgapp_window_class";
    win32_state->hinstance = GetModuleHandleW(NULL);

    mgapp_win32_setup_clock(win32_state);
    mgapp_win32_load_cursors(win32_state);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = mgapp_win32_process_message;
    wc.hInstance = win32_state->hinstance;
    wc.lpszClassName = class_name;
    wc.hCursor = win32_state->cursor_table[MG_CURSOR_ARROW];

    if (!RegisterClassW(&wc))
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

    WCHAR wide_title[256] = L"";
    if (info->title)
        MultiByteToWideChar(CP_UTF8, 0, info->title, -1, wide_title, (int)(sizeof(wide_title) / sizeof(wide_title[0])));

    win32_state->hwnd = CreateWindowExW(
        0,
        class_name,
        wide_title,
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

        win32_state->original_proc = (WNDPROC)GetWindowLongPtrW(win32_state->hwnd, GWLP_WNDPROC);
        SetWindowLongPtrW(win32_state->hwnd, GWLP_WNDPROC, (LONG_PTR)mgapp_win32_no_titlebar_proc);
        SetWindowPos(win32_state->hwnd, NULL, 0, 0, w, h, SWP_FRAMECHANGED | SWP_NOMOVE);
    }

    if (!IsWindowUnicode(win32_state->hwnd))
        MGAPP_ERROR("Window is not a Unicode window; IME and non-ASCII text input will not work.");

    win32_state->ime_himc = ImmGetContext(win32_state->hwnd);
    mgapp_platform_ime_set_mode(MGAPP_IME_MODE_OFF);

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
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                mgapp_state.running = false;
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
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
    UnregisterClassW(class_name, win32_state->hinstance);
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

void mgapp_set_clipboard(const char *text)
{
    if (!text)
        return;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (wlen <= 0)
        return;

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wlen * sizeof(WCHAR));
    if (!mem)
        return;

    WCHAR *dst = (WCHAR*)GlobalLock(mem);
    if (!dst)
    {
        GlobalFree(mem);
        return;
    }
    MultiByteToWideChar(CP_UTF8, 0, text, -1, dst, wlen);
    GlobalUnlock(mem);

    if (!OpenClipboard(mgapp_state.win32.hwnd))
    {
        GlobalFree(mem);
        return;
    }

    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, mem))
        GlobalFree(mem);
    CloseClipboard();
}

const char *mgapp_get_clipboard(void)
{
    mgapp_clipboard_cache(NULL, 0);

    if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return NULL;
    if (!OpenClipboard(mgapp_state.win32.hwnd))
        return NULL;

    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if (handle)
    {
        const WCHAR *wide = (const WCHAR*)GlobalLock(handle);
        if (wide)
        {
            int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
            if (len > 0)
            {
                char *buf = (char*)malloc((size_t)len);
                if (buf)
                {
                    WideCharToMultiByte(CP_UTF8, 0, wide, -1, buf, len, NULL, NULL);
                    mgapp_state.clipboard_buffer = buf;
                }
            }
            GlobalUnlock(handle);
        }
    }

    CloseClipboard();
    return mgapp_state.clipboard_buffer;
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

static inline void mgapp_xlib_handle_selection_request(mgapp_xlib_state *xlib_state, const XSelectionRequestEvent *req)
{
    XSelectionEvent reply = {0};
    reply.type = SelectionNotify;
    reply.display = req->display;
    reply.requestor = req->requestor;
    reply.selection = req->selection;
    reply.target = req->target;
    reply.time = req->time;
    reply.property = None;

    Atom prop = req->property ? req->property : req->target;

    if (req->selection == xlib_state->clipboard_atom && xlib_state->clipboard_owned)
    {
        if (req->target == xlib_state->targets_atom)
        {
            Atom supported[] = { xlib_state->targets_atom, xlib_state->utf8_string_atom, XA_STRING };
            XChangeProperty(xlib_state->display, req->requestor, prop, XA_ATOM, 32,
                PropModeReplace, (unsigned char*)supported, 3);
            reply.property = prop;
        }
        else if (req->target == xlib_state->utf8_string_atom || req->target == XA_STRING)
        {
            XChangeProperty(xlib_state->display, req->requestor, prop, req->target, 8,
                PropModeReplace, (unsigned char*)xlib_state->clipboard_owned,
                (int)xlib_state->clipboard_owned_len);
            reply.property = prop;
        }
    }

    XSendEvent(xlib_state->display, req->requestor, False, NoEventMask, (XEvent*)&reply);
    XFlush(xlib_state->display);
}

static Bool mgapp_xlib_clipboard_predicate(Display *display, XEvent *event, XPointer arg)
{
    (void)display;
    const mgapp_xlib_state *xlib_state = (const mgapp_xlib_state*)arg;
    return event->type == SelectionNotify &&
           event->xselection.selection == xlib_state->clipboard_atom;
}

static void mgapp_xlib_process_text(mgapp_xlib_state *xlib_state, XKeyEvent *key_event)
{
    char stack_buffer[64];
    char *buffer = stack_buffer;
    KeySym keysym = NoSymbol;
    int32_t length = 0;

    if (!xlib_state->xic)
    {
        length = XLookupString(key_event, buffer, sizeof(stack_buffer) - 1, &keysym, NULL);
        for (int32_t i = 0; i < length; ++i)
            mgapp_emit_char((uint32_t)(unsigned char)buffer[i]);
        return;
    }

    Status status = 0;
    length = Xutf8LookupString(xlib_state->xic, key_event, buffer, sizeof(stack_buffer) - 1, &keysym, &status);
    if (status == XBufferOverflow)
    {
        buffer = (char*)malloc((size_t)length + 1);
        if (!buffer)
            return;
        length = Xutf8LookupString(xlib_state->xic, key_event, buffer, length, &keysym, &status);
    }

    if (status == XLookupChars || status == XLookupBoth)
    {
        int32_t i = 0;
        while (i < length)
        {
            uint32_t codepoint;
            i += mgapp_utf8_decode(buffer + i, length - i, &codepoint);
            mgapp_emit_char(codepoint);
        }
    }

    if (buffer != stack_buffer)
        free(buffer);
}

static void mgapp_xlib_ime_sync_preedit(mgapp_xlib_state *xlib_state)
{
    mgapp_ime_clear_composition();
    mgapp_state.ime.composing = true;

    int32_t out = 0;
    int32_t cursor = -1;
    int32_t clause_start = 0;
    XIMFeedback clause_feedback = 0;
    for (int32_t i = 0; i < xlib_state->preedit_length; ++i)
    {
        if (i == xlib_state->preedit_caret)
            cursor = out;

        char bytes[4];
        const int32_t count = mgapp_utf8_encode(xlib_state->preedit_chars[i], bytes);
        if (out + count >= MGAPP_IME_MAX_TEXT)
            break;

        const XIMFeedback feedback = xlib_state->preedit_feedback[i];
        if (i > 0 && feedback != clause_feedback)
        {
            mgapp_ime_push_clause(clause_start, out, (clause_feedback & (XIMReverse | XIMHighlight)) != 0);
            clause_start = out;
        }
        clause_feedback = feedback;

        memcpy(mgapp_state.ime.text + out, bytes, (size_t)count);
        out += count;
    }
    mgapp_ime_push_clause(clause_start, out, (clause_feedback & (XIMReverse | XIMHighlight)) != 0);

    mgapp_state.ime.text[out] = '\0';
    mgapp_state.ime.length = out;
    mgapp_state.ime.cursor = cursor < 0 ? out : cursor;
}

static void mgapp_xlib_ime_reset_preedit(mgapp_xlib_state *xlib_state)
{
    xlib_state->preedit_length = 0;
    xlib_state->preedit_caret = 0;
    mgapp_ime_clear_composition();
}

static int mgapp_xlib_preedit_start(XIC xic, XPointer client_data, XPointer call_data)
{
    (void)xic; (void)call_data;
    mgapp_xlib_state *xlib_state = (mgapp_xlib_state*)client_data;
    mgapp_xlib_ime_reset_preedit(xlib_state);
    mgapp_state.ime.composing = true;
    return MGAPP_XLIB_IME_MAX_CHARS;
}

static int mgapp_xlib_preedit_done(XIC xic, XPointer client_data, XPointer call_data)
{
    (void)xic; (void)call_data;
    mgapp_xlib_ime_reset_preedit((mgapp_xlib_state*)client_data);
    return 0;
}

static int mgapp_xlib_preedit_draw(XIC xic, XPointer client_data, XPointer call_data)
{
    (void)xic;
    mgapp_xlib_state *xlib_state = (mgapp_xlib_state*)client_data;
    const XIMPreeditDrawCallbackStruct *draw = (const XIMPreeditDrawCallbackStruct*)call_data;
    const XIMText *text = draw->text;

    const int32_t length = xlib_state->preedit_length;
    const int32_t first = draw->chg_first < 0 ? 0 : draw->chg_first > length ? length : draw->chg_first;
    const int32_t removed = draw->chg_length < 0 ? 0 : draw->chg_length > length - first ? length - first : draw->chg_length;

    if (text && !text->string.multi_byte)
    {
        for (int32_t i = 0; i < removed && i < (int32_t)text->length; ++i)
            xlib_state->preedit_feedback[first + i] = text->feedback ? text->feedback[i] : 0;
    }
    else
    {
        uint32_t inserted[MGAPP_XLIB_IME_MAX_CHARS];
        int32_t inserted_count = 0;

        if (text && text->encoding_is_wchar)
        {
            for (; inserted_count < (int32_t)text->length && inserted_count < MGAPP_XLIB_IME_MAX_CHARS; ++inserted_count)
                inserted[inserted_count] = (uint32_t)text->string.wide_char[inserted_count];
        }
        else if (text)
        {
            wchar_t wide[MGAPP_XLIB_IME_MAX_CHARS];
            const size_t converted = mbstowcs(wide, text->string.multi_byte, MGAPP_XLIB_IME_MAX_CHARS);
            if (converted != (size_t)-1)
            {
                for (; inserted_count < (int32_t)converted; ++inserted_count)
                    inserted[inserted_count] = (uint32_t)wide[inserted_count];
            }
        }

        const int32_t tail = length - first - removed;
        if (first + inserted_count + tail > MGAPP_XLIB_IME_MAX_CHARS)
            inserted_count = MGAPP_XLIB_IME_MAX_CHARS - first - tail;

        memmove(xlib_state->preedit_chars + first + inserted_count, xlib_state->preedit_chars + first + removed, (size_t)tail * sizeof(uint32_t));
        memmove(xlib_state->preedit_feedback + first + inserted_count, xlib_state->preedit_feedback + first + removed, (size_t)tail * sizeof(XIMFeedback));

        for (int32_t i = 0; i < inserted_count; ++i)
        {
            xlib_state->preedit_chars[first + i] = inserted[i];
            xlib_state->preedit_feedback[first + i] = (text->feedback && i < (int32_t)text->length) ? text->feedback[i] : 0;
        }

        xlib_state->preedit_length = first + inserted_count + tail;
    }

    const int32_t caret = draw->caret;
    xlib_state->preedit_caret = caret < 0 ? 0 : caret > xlib_state->preedit_length ? xlib_state->preedit_length : caret;
    mgapp_xlib_ime_sync_preedit(xlib_state);
    return 0;
}

static int mgapp_xlib_preedit_caret(XIC xic, XPointer client_data, XPointer call_data)
{
    (void)xic;
    mgapp_xlib_state *xlib_state = (mgapp_xlib_state*)client_data;
    XIMPreeditCaretCallbackStruct *caret = (XIMPreeditCaretCallbackStruct*)call_data;

    int32_t position = xlib_state->preedit_caret;
    switch (caret->direction)
    {
        case XIMForwardChar:      ++position; break;
        case XIMBackwardChar:     --position; break;
        case XIMAbsolutePosition: position = caret->position; break;
        case XIMLineStart:        position = 0; break;
        case XIMLineEnd:          position = xlib_state->preedit_length; break;
        default: break;
    }

    if (position < 0)
        position = 0;
    if (position > xlib_state->preedit_length)
        position = xlib_state->preedit_length;

    xlib_state->preedit_caret = position;
    caret->position = position;
    mgapp_xlib_ime_sync_preedit(xlib_state);
    return 0;
}

static void mgapp_xlib_ime_instantiate(Display *display, XPointer client_data, XPointer call_data);

static void mgapp_xlib_ime_destroyed(XIM xim, XPointer client_data, XPointer call_data)
{
    (void)xim; (void)call_data;
    mgapp_xlib_state *xlib_state = (mgapp_xlib_state*)client_data;
    xlib_state->xim = NULL;
    xlib_state->xic = NULL;
    mgapp_xlib_ime_reset_preedit(xlib_state);
    XRegisterIMInstantiateCallback(xlib_state->display, NULL, NULL, NULL, mgapp_xlib_ime_instantiate, (XPointer)xlib_state);
}

static void mgapp_xlib_ime_apply_area(void)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    if (!xlib_state->xic || !(xlib_state->xim_style & (XIMPreeditPosition | XIMPreeditCallbacks)))
        return;

    XPoint spot = {
        .x = (short)(mgapp_state.ime.area_x + mgapp_state.ime.caret_x),
        .y = (short)(mgapp_state.ime.area_y + mgapp_state.ime.area_height)
    };
    XVaNestedList attributes = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
    XSetICValues(xlib_state->xic, XNPreeditAttributes, attributes, NULL);
    XFree(attributes);
}

static XFontSet mgapp_xlib_ime_fontset(mgapp_xlib_state *xlib_state)
{
    if (!xlib_state->ime_fontset)
    {
        char **missing = NULL;
        int missing_count = 0;
        char *fallback = NULL;
        xlib_state->ime_fontset = XCreateFontSet(xlib_state->display, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*", &missing, &missing_count, &fallback);
        if (missing)
            XFreeStringList(missing);
    }

    return xlib_state->ime_fontset;
}

static XIC mgapp_xlib_ime_create_ic(mgapp_xlib_state *xlib_state, XIMStyle style)
{
    XVaNestedList preedit_attributes = NULL;
    XPoint spot = { 0, 0 };

    if (style & XIMPreeditCallbacks)
    {
        const XICProc procs[4] = { mgapp_xlib_preedit_start, mgapp_xlib_preedit_done, mgapp_xlib_preedit_draw, mgapp_xlib_preedit_caret };
        for (int32_t i = 0; i < 4; ++i)
            xlib_state->preedit_callbacks[i] = (XICCallback){ .client_data = (XPointer)xlib_state, .callback = procs[i] };

        preedit_attributes = XVaCreateNestedList(0,
            XNPreeditStartCallback, &xlib_state->preedit_callbacks[0],
            XNPreeditDoneCallback, &xlib_state->preedit_callbacks[1],
            XNPreeditDrawCallback, &xlib_state->preedit_callbacks[2],
            XNPreeditCaretCallback, &xlib_state->preedit_callbacks[3],
            NULL);
    }
    else if (style & XIMPreeditPosition)
    {
        XFontSet fontset = mgapp_xlib_ime_fontset(xlib_state);
        if (!fontset)
            return NULL;

        preedit_attributes = XVaCreateNestedList(0, XNSpotLocation, &spot, XNFontSet, fontset, NULL);
    }

    if (!preedit_attributes)
    {
        return XCreateIC(xlib_state->xim,
            XNInputStyle, style,
            XNClientWindow, xlib_state->window,
            XNFocusWindow, xlib_state->window,
            NULL);
    }

    XIC xic = XCreateIC(xlib_state->xim,
        XNInputStyle, style,
        XNClientWindow, xlib_state->window,
        XNFocusWindow, xlib_state->window,
        XNPreeditAttributes, preedit_attributes,
        NULL);
    XFree(preedit_attributes);
    return xic;
}

static bool mgapp_xlib_ime_create_best_ic(mgapp_xlib_state *xlib_state)
{
    XIMStyles *styles = NULL;
    if (XGetIMValues(xlib_state->xim, XNQueryInputStyle, &styles, NULL) || !styles)
        return false;

    const XIMStyle preedit_mask = XIMPreeditArea | XIMPreeditCallbacks | XIMPreeditPosition | XIMPreeditNothing | XIMPreeditNone;
    const XIMStyle preferred[] = { XIMPreeditCallbacks, XIMPreeditPosition, XIMPreeditNothing, XIMPreeditNone };

    for (size_t p = 0; p < sizeof(preferred) / sizeof(preferred[0]) && !xlib_state->xic; ++p)
    {
        for (uint16_t i = 0; i < styles->count_styles && !xlib_state->xic; ++i)
        {
            const XIMStyle style = styles->supported_styles[i];
            const XIMStyle status = style & ~preedit_mask;
            if ((style & preedit_mask) != preferred[p] || (status != XIMStatusNothing && status != XIMStatusNone))
                continue;

            xlib_state->xic = mgapp_xlib_ime_create_ic(xlib_state, style);
            if (xlib_state->xic)
                xlib_state->xim_style = style;
        }
    }

    XFree(styles);
    return xlib_state->xic != NULL;
}

static bool mgapp_xlib_ime_open(mgapp_xlib_state *xlib_state)
{
    xlib_state->xim = XOpenIM(xlib_state->display, NULL, NULL, NULL);
    if (!xlib_state->xim)
        return false;

    XIMCallback destroy = { .client_data = (XPointer)xlib_state, .callback = mgapp_xlib_ime_destroyed };
    XSetIMValues(xlib_state->xim, XNDestroyCallback, &destroy, NULL);

    if (!mgapp_xlib_ime_create_best_ic(xlib_state))
    {
        XCloseIM(xlib_state->xim);
        xlib_state->xim = NULL;
        return false;
    }

    long filter_events = 0;
    XGetICValues(xlib_state->xic, XNFilterEvents, &filter_events, NULL);
    XSelectInput(xlib_state->display, xlib_state->window, xlib_state->event_mask | filter_events);

    XUnsetICFocus(xlib_state->xic);
    if (mgapp_state.ime.mode == MGAPP_IME_MODE_TEXT && xlib_state->focused)
    {
        XSetICFocus(xlib_state->xic);
        mgapp_xlib_ime_apply_area();
    }

    return true;
}

static void mgapp_xlib_ime_instantiate(Display *display, XPointer client_data, XPointer call_data)
{
    (void)call_data;
    mgapp_xlib_state *xlib_state = (mgapp_xlib_state*)client_data;
    if (!xlib_state->xic && mgapp_xlib_ime_open(xlib_state))
        XUnregisterIMInstantiateCallback(display, NULL, NULL, NULL, mgapp_xlib_ime_instantiate, client_data);
}

static void mgapp_xlib_ime_init(mgapp_xlib_state *xlib_state)
{
    const char *locale = setlocale(LC_CTYPE, NULL);
    if (!locale || strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0)
        setlocale(LC_CTYPE, "");

    if (!XSupportsLocale())
        setlocale(LC_CTYPE, "C.UTF-8");

    if (!XSetLocaleModifiers(""))
        XSetLocaleModifiers("@im=none");

    if (!mgapp_xlib_ime_open(xlib_state))
        XRegisterIMInstantiateCallback(xlib_state->display, NULL, NULL, NULL, mgapp_xlib_ime_instantiate, (XPointer)xlib_state);
}

static void mgapp_xlib_ime_shutdown(mgapp_xlib_state *xlib_state)
{
    XUnregisterIMInstantiateCallback(xlib_state->display, NULL, NULL, NULL, mgapp_xlib_ime_instantiate, (XPointer)xlib_state);

    if (xlib_state->xic)
        XDestroyIC(xlib_state->xic);
    if (xlib_state->xim)
        XCloseIM(xlib_state->xim);

    if (xlib_state->ime_fontset)
        XFreeFontSet(xlib_state->display, xlib_state->ime_fontset);

    xlib_state->xic = NULL;
    xlib_state->xim = NULL;
    xlib_state->ime_fontset = NULL;
}

static void mgapp_xlib_ime_cancel(void)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    if (!xlib_state->xic)
        return;

    if ((xlib_state->xim_style & XIMPreeditCallbacks) && xlib_state->preedit_length == 0)
        return;

    char *discarded = XmbResetIC(xlib_state->xic);
    if (discarded)
        XFree(discarded);

    xlib_state->preedit_length = 0;
    xlib_state->preedit_caret = 0;
}

static void mgapp_xlib_ime_set_mode(mgapp_ime_mode mode)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    if (!xlib_state->xic)
        return;

    if (mode == MGAPP_IME_MODE_TEXT)
    {
        if (xlib_state->focused)
            XSetICFocus(xlib_state->xic);
        mgapp_xlib_ime_apply_area();
    }
    else
    {
        mgapp_xlib_ime_cancel();
        XUnsetICFocus(xlib_state->xic);
    }
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
    xlib_state->event_mask =
        StructureNotifyMask | ExposureMask |
        KeyPressMask | KeyReleaseMask | FocusChangeMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask | EnterWindowMask | LeaveWindowMask;

    const uint32_t width = info->width ? info->width : 800;
    const uint32_t height = info->height ? info->height : 600;

    Window root = RootWindow(xlib_state->display, xlib_state->screen);

    XSetWindowAttributes swa = {0};
    swa.background_pixmap = None;
    swa.bit_gravity = NorthWestGravity;
    swa.event_mask = xlib_state->event_mask;

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

    xlib_state->clipboard_atom = XInternAtom(xlib_state->display, "CLIPBOARD", False);
    xlib_state->utf8_string_atom = XInternAtom(xlib_state->display, "UTF8_STRING", False);
    xlib_state->targets_atom = XInternAtom(xlib_state->display, "TARGETS", False);
    xlib_state->clipboard_prop_atom = XInternAtom(xlib_state->display, "MGAPP_CLIPBOARD", False);

    if (info->flags & MGAPP_FLAG_NO_TITLEBAR)
        mgapp_xlib_set_decorated(xlib_state, false);

    mgapp_xlib_apply_size_hints(xlib_state, info->flags, (int32_t)width, (int32_t)height);
    mgapp_xlib_apply_allowed_actions(xlib_state, info->flags);

    mgapp_xlib_load_cursors(xlib_state);
    mgapp_xlib_load_dpi(xlib_state);
    mgapp_xlib_ime_init(xlib_state);

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

            if (XFilterEvent(&xev, None))
                continue;

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
                        if (mgapp_request_close())
                        {
                            mgapp_event event = { .type = MGAPP_EVENT_WINDOW_CLOSE };
                            mgapp_call_event(&event);
                            mgapp_state.running = false;
                        }
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
                case SelectionRequest:
                {
                    mgapp_xlib_handle_selection_request(xlib_state, &xev.xselectionrequest);
                    break;
                }
                case SelectionClear:
                {
                    free(xlib_state->clipboard_owned);
                    xlib_state->clipboard_owned = NULL;
                    xlib_state->clipboard_owned_len = 0;
                    break;
                }
                case FocusIn:
                {
                    xlib_state->focused = true;
                    if (xlib_state->xic && mgapp_state.ime.mode == MGAPP_IME_MODE_TEXT)
                    {
                        XSetICFocus(xlib_state->xic);
                        mgapp_xlib_ime_apply_area();
                    }
                    break;
                }
                case FocusOut:
                {
                    xlib_state->focused = false;
                    if (xlib_state->xic)
                        XUnsetICFocus(xlib_state->xic);
                    mgapp_input_release_all_keys();
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
                        mgapp_xlib_process_text(xlib_state, &xev.xkey);
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
                case EnterNotify:
                {
                    mgapp_state.input.mouse.x = (int16_t)xev.xcrossing.x;
                    mgapp_state.input.mouse.y = (int16_t)xev.xcrossing.y;
                    mgapp_state.input.mouse.inside = true;
                    break;
                }
                case LeaveNotify:
                {
                    mgapp_state.input.mouse.inside = false;
                    break;
                }
                case MotionNotify:
                {
                    mgapp_state.input.mouse.x = (int16_t)xev.xmotion.x;
                    mgapp_state.input.mouse.y = (int16_t)xev.xmotion.y;
                    mgapp_state.input.mouse.inside =
                        xev.xmotion.x >= 0 && xev.xmotion.x < mgapp_state.window_width &&
                        xev.xmotion.y >= 0 && xev.xmotion.y < mgapp_state.window_height;
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

    free(xlib_state->clipboard_owned);
    mgapp_xlib_ime_shutdown(xlib_state);
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
    if (!mgapp_request_close())
        return;

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

void mgapp_xlib_set_clipboard(const char *text)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    if (!text)
        return;

    size_t len = strlen(text);
    char *copy = (char*)malloc(len + 1);
    if (!copy)
        return;
    memcpy(copy, text, len + 1);

    free(xlib_state->clipboard_owned);
    xlib_state->clipboard_owned = copy;
    xlib_state->clipboard_owned_len = len;

    XSetSelectionOwner(xlib_state->display, xlib_state->clipboard_atom, xlib_state->window, CurrentTime);
    XFlush(xlib_state->display);
}

const char *mgapp_xlib_get_clipboard(void)
{
    mgapp_xlib_state *xlib_state = &mgapp_state._linux.xlib;
    Display *display = xlib_state->display;

    Window owner = XGetSelectionOwner(display, xlib_state->clipboard_atom);

    if (owner == xlib_state->window)
    {
        mgapp_clipboard_cache(xlib_state->clipboard_owned, xlib_state->clipboard_owned_len);
        return mgapp_state.clipboard_buffer;
    }

    mgapp_clipboard_cache(NULL, 0);
    if (owner == None)
        return NULL;

    XDeleteProperty(display, xlib_state->window, xlib_state->clipboard_prop_atom);
    XConvertSelection(display, xlib_state->clipboard_atom, xlib_state->utf8_string_atom,
        xlib_state->clipboard_prop_atom, xlib_state->window, CurrentTime);
    XFlush(display);

    XEvent event;
    bool got_reply = false;
    for (int32_t i = 0; i < 1000 && !got_reply; i++)
    {
        if (XCheckIfEvent(display, &event, mgapp_xlib_clipboard_predicate, (XPointer)xlib_state))
            got_reply = true;
        else
        {
            struct timespec ts = { 0, 1000000 };
            nanosleep(&ts, NULL);
        }
    }

    if (!got_reply || event.xselection.property == None)
        return NULL;

    Atom actual_type;
    int32_t actual_format;
    unsigned long item_count, bytes_after;
    unsigned char *data = NULL;

    XGetWindowProperty(display, xlib_state->window, xlib_state->clipboard_prop_atom,
        0, LONG_MAX, True, AnyPropertyType,
        &actual_type, &actual_format, &item_count, &bytes_after, &data);

    if (data)
    {
        if (actual_format == 8 && item_count > 0)
            mgapp_clipboard_cache((const char*)data, item_count);
        XFree(data);
    }

    return mgapp_state.clipboard_buffer;
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
        state->set_clipboard = mgapp_xlib_set_clipboard;
        state->get_clipboard = mgapp_xlib_get_clipboard;
        state->ime_set_mode = mgapp_xlib_ime_set_mode;
        state->ime_cancel = mgapp_xlib_ime_cancel;
        state->ime_apply_area = mgapp_xlib_ime_apply_area;
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

void mgapp_set_clipboard(const char *text)
{
    mgapp_state._linux.set_clipboard(text);
}

const char *mgapp_get_clipboard(void)
{
    return mgapp_state._linux.get_clipboard();
}

static void mgapp_platform_ime_set_mode(mgapp_ime_mode mode)
{
    if (mgapp_state._linux.ime_set_mode)
        mgapp_state._linux.ime_set_mode(mode);
}

static void mgapp_platform_ime_cancel(void)
{
    if (mgapp_state._linux.ime_cancel)
        mgapp_state._linux.ime_cancel();
}

static void mgapp_platform_ime_apply_area(void)
{
    if (mgapp_state._linux.ime_apply_area)
        mgapp_state._linux.ime_apply_area();
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

#elif defined(__APPLE__)

static inline mg_key mgapp_macos_translate_key(unsigned short key_code)
{
    switch (key_code)
    {
        case kVK_ANSI_A:              return MG_KEY_A;
        case kVK_ANSI_B:              return MG_KEY_B;
        case kVK_ANSI_C:              return MG_KEY_C;
        case kVK_ANSI_D:              return MG_KEY_D;
        case kVK_ANSI_E:              return MG_KEY_E;
        case kVK_ANSI_F:              return MG_KEY_F;
        case kVK_ANSI_G:              return MG_KEY_G;
        case kVK_ANSI_H:              return MG_KEY_H;
        case kVK_ANSI_I:              return MG_KEY_I;
        case kVK_ANSI_J:              return MG_KEY_J;
        case kVK_ANSI_K:              return MG_KEY_K;
        case kVK_ANSI_L:              return MG_KEY_L;
        case kVK_ANSI_M:              return MG_KEY_M;
        case kVK_ANSI_N:              return MG_KEY_N;
        case kVK_ANSI_O:              return MG_KEY_O;
        case kVK_ANSI_P:              return MG_KEY_P;
        case kVK_ANSI_Q:              return MG_KEY_Q;
        case kVK_ANSI_R:              return MG_KEY_R;
        case kVK_ANSI_S:              return MG_KEY_S;
        case kVK_ANSI_T:              return MG_KEY_T;
        case kVK_ANSI_U:              return MG_KEY_U;
        case kVK_ANSI_V:              return MG_KEY_V;
        case kVK_ANSI_W:              return MG_KEY_W;
        case kVK_ANSI_X:              return MG_KEY_X;
        case kVK_ANSI_Y:              return MG_KEY_Y;
        case kVK_ANSI_Z:              return MG_KEY_Z;

        case kVK_ANSI_0:              return MG_KEY_0;
        case kVK_ANSI_1:              return MG_KEY_1;
        case kVK_ANSI_2:              return MG_KEY_2;
        case kVK_ANSI_3:              return MG_KEY_3;
        case kVK_ANSI_4:              return MG_KEY_4;
        case kVK_ANSI_5:              return MG_KEY_5;
        case kVK_ANSI_6:              return MG_KEY_6;
        case kVK_ANSI_7:              return MG_KEY_7;
        case kVK_ANSI_8:              return MG_KEY_8;
        case kVK_ANSI_9:              return MG_KEY_9;

        case kVK_F1:                  return MG_KEY_F1;
        case kVK_F2:                  return MG_KEY_F2;
        case kVK_F3:                  return MG_KEY_F3;
        case kVK_F4:                  return MG_KEY_F4;
        case kVK_F5:                  return MG_KEY_F5;
        case kVK_F6:                  return MG_KEY_F6;
        case kVK_F7:                  return MG_KEY_F7;
        case kVK_F8:                  return MG_KEY_F8;
        case kVK_F9:                  return MG_KEY_F9;
        case kVK_F10:                 return MG_KEY_F10;
        case kVK_F11:                 return MG_KEY_F11;
        case kVK_F12:                 return MG_KEY_F12;
        case kVK_F13:                 return MG_KEY_F13;
        case kVK_F14:                 return MG_KEY_F14;
        case kVK_F15:                 return MG_KEY_F15;
        case kVK_F16:                 return MG_KEY_F16;
        case kVK_F17:                 return MG_KEY_F17;
        case kVK_F18:                 return MG_KEY_F18;
        case kVK_F19:                 return MG_KEY_F19;
        case kVK_F20:                 return MG_KEY_F20;

        case kVK_ANSI_Keypad0:        return MG_KEY_NUMPAD0;
        case kVK_ANSI_Keypad1:        return MG_KEY_NUMPAD1;
        case kVK_ANSI_Keypad2:        return MG_KEY_NUMPAD2;
        case kVK_ANSI_Keypad3:        return MG_KEY_NUMPAD3;
        case kVK_ANSI_Keypad4:        return MG_KEY_NUMPAD4;
        case kVK_ANSI_Keypad5:        return MG_KEY_NUMPAD5;
        case kVK_ANSI_Keypad6:        return MG_KEY_NUMPAD6;
        case kVK_ANSI_Keypad7:        return MG_KEY_NUMPAD7;
        case kVK_ANSI_Keypad8:        return MG_KEY_NUMPAD8;
        case kVK_ANSI_Keypad9:        return MG_KEY_NUMPAD9;
        case kVK_ANSI_KeypadMultiply: return MG_KEY_MULTIPLY;
        case kVK_ANSI_KeypadPlus:     return MG_KEY_ADD;
        case kVK_ANSI_KeypadMinus:    return MG_KEY_SUBTRACT;
        case kVK_ANSI_KeypadDecimal:  return MG_KEY_DECIMAL;
        case kVK_ANSI_KeypadDivide:   return MG_KEY_DIVIDE;
        case kVK_ANSI_KeypadEnter:    return MG_KEY_ENTER;
        case kVK_ANSI_KeypadEquals:   return MG_KEY_NUMPAD_EQUAL;
        case kVK_ANSI_KeypadClear:    return MG_KEY_NUMLOCK;

        case kVK_Delete:              return MG_KEY_BACKSPACE;
        case kVK_ForwardDelete:       return MG_KEY_DELETE;
        case kVK_Return:              return MG_KEY_ENTER;
        case kVK_Tab:                 return MG_KEY_TAB;
        case kVK_Escape:              return MG_KEY_ESCAPE;
        case kVK_Space:               return MG_KEY_SPACE;
        case kVK_CapsLock:            return MG_KEY_CAPITAL;
        case kVK_Help:                return MG_KEY_INSERT;
        case kVK_PageUp:              return MG_KEY_PAGE_UP;
        case kVK_PageDown:            return MG_KEY_PAGE_DOWN;
        case kVK_End:                 return MG_KEY_END;
        case kVK_Home:                return MG_KEY_HOME;
        case kVK_LeftArrow:           return MG_KEY_LEFT;
        case kVK_UpArrow:             return MG_KEY_UP;
        case kVK_RightArrow:          return MG_KEY_RIGHT;
        case kVK_DownArrow:           return MG_KEY_DOWN;

        case kVK_Shift:               return MG_KEY_LSHIFT;
        case kVK_RightShift:          return MG_KEY_RSHIFT;
        case kVK_Control:             return MG_KEY_LCONTROL;
        case kVK_RightControl:        return MG_KEY_RCONTROL;
        case kVK_Option:              return MG_KEY_LALT;
        case kVK_RightOption:         return MG_KEY_RALT;
        case kVK_Command:             return MG_KEY_LSUPER;
        case kVK_RightCommand:        return MG_KEY_RSUPER;

        case kVK_ANSI_Semicolon:      return MG_KEY_SEMICOLON;
        case kVK_ANSI_Quote:          return MG_KEY_APOSTROPHE;
        case kVK_ANSI_Equal:          return MG_KEY_EQUAL;
        case kVK_ANSI_Comma:          return MG_KEY_COMMA;
        case kVK_ANSI_Minus:          return MG_KEY_MINUS;
        case kVK_ANSI_Period:         return MG_KEY_PERIOD;
        case kVK_ANSI_Slash:          return MG_KEY_SLASH;
        case kVK_ANSI_Grave:          return MG_KEY_GRAVE;
        case kVK_ANSI_LeftBracket:    return MG_KEY_LBRACKET;
        case kVK_ANSI_Backslash:      return MG_KEY_PIPE;
        case kVK_ANSI_RightBracket:   return MG_KEY_RBRACKET;

        default: return MG_KEY_MAX;
    }
}

static inline NSEventModifierFlags mgapp_macos_modifier_flag(mg_key key)
{
    switch (key)
    {
        case MG_KEY_LSHIFT:
        case MG_KEY_RSHIFT:   return NSEventModifierFlagShift;
        case MG_KEY_LCONTROL:
        case MG_KEY_RCONTROL: return NSEventModifierFlagControl;
        case MG_KEY_LALT:
        case MG_KEY_RALT:     return NSEventModifierFlagOption;
        case MG_KEY_LSUPER:
        case MG_KEY_RSUPER:   return NSEventModifierFlagCommand;
        case MG_KEY_CAPITAL:  return NSEventModifierFlagCapsLock;
        default:              return 0;
    }
}

static inline NSCursor *mgapp_macos_private_cursor(NSString *selector_name, NSCursor *fallback)
{
    const SEL selector = NSSelectorFromString(selector_name);
    if ([NSCursor respondsToSelector:selector])
        return [NSCursor performSelector:selector];
    return fallback;
}

static inline void mgapp_macos_load_cursors(mgapp_macos_state *macos_state)
{
    macos_state->cursor_table[MG_CURSOR_ARROW] = [NSCursor arrowCursor];
    macos_state->cursor_table[MG_CURSOR_IBEAM] = [NSCursor IBeamCursor];
    macos_state->cursor_table[MG_CURSOR_CROSSHAIR] = [NSCursor crosshairCursor];
    macos_state->cursor_table[MG_CURSOR_HAND] = [NSCursor pointingHandCursor];
    macos_state->cursor_table[MG_CURSOR_RESIZE_NS] = [NSCursor resizeUpDownCursor];
    macos_state->cursor_table[MG_CURSOR_RESIZE_EW] = [NSCursor resizeLeftRightCursor];
    macos_state->cursor_table[MG_CURSOR_RESIZE_NESW] = mgapp_macos_private_cursor(@"_windowResizeNorthEastSouthWestCursor", [NSCursor crosshairCursor]);
    macos_state->cursor_table[MG_CURSOR_RESIZE_NWSE] = mgapp_macos_private_cursor(@"_windowResizeNorthWestSouthEastCursor", [NSCursor crosshairCursor]);
    macos_state->cursor_table[MG_CURSOR_RESIZE_ALL] = mgapp_macos_private_cursor(@"_moveCursor", [NSCursor openHandCursor]);
    macos_state->cursor_table[MG_CURSOR_NOT_ALLOWED] = [NSCursor operationNotAllowedCursor];
    macos_state->cursor_table[MG_CURSOR_HIDDEN] = nil;

    for (int32_t i = 0; i < MG_CURSOR_MAX; ++i)
        [macos_state->cursor_table[i] retain];
}

static void mgapp_macos_apply_cursor(mg_cursor cursor)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    if (cursor == MG_CURSOR_HIDDEN)
    {
        if (!macos_state->cursor_hidden)
        {
            [NSCursor hide];
            macos_state->cursor_hidden = true;
        }
        return;
    }

    if (macos_state->cursor_hidden)
    {
        [NSCursor unhide];
        macos_state->cursor_hidden = false;
    }

    [macos_state->cursor_table[cursor] set];
}

static void mgapp_macos_emit_key(mg_key key, bool pressed)
{
    mgapp_input_process_key(key, pressed);
    mgapp_event event = {
        .key = key,
        .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_KEY_DOWN : MGAPP_EVENT_KEY_UP)
    };
    mgapp_call_event(&event);
}

static void mgapp_macos_emit_text(NSString *text)
{
    const char *utf8 = [text UTF8String];
    if (!utf8)
        return;

    const int32_t length = (int32_t)strlen(utf8);
    int32_t i = 0;
    while (i < length)
    {
        uint32_t codepoint;
        i += mgapp_utf8_decode(utf8 + i, length - i, &codepoint);
        mgapp_emit_char(codepoint);
    }
}

static void mgapp_macos_emit_key_chars(NSEvent *event)
{
    if ([event modifierFlags] & NSEventModifierFlagCommand)
        return;

    NSString *characters = [event characters];
    const NSUInteger count = [characters length];
    for (NSUInteger i = 0; i < count; ++i)
    {
        uint32_t codepoint = [characters characterAtIndex:i];
        if (CFStringIsSurrogateHighCharacter((UniChar)codepoint) && i + 1 < count)
            codepoint = (uint32_t)CFStringGetLongCharacterForSurrogatePair((UniChar)codepoint, [characters characterAtIndex:++i]);

        switch (codepoint)
        {
            case 0x7F:                 codepoint = 0x08; break;
            case NSDeleteFunctionKey:  codepoint = 0x7F; break;
            case NSEnterCharacter:     codepoint = 0x0D; break;
            default: break;
        }

        if (codepoint >= 0xF700 && codepoint <= 0xF8FF)
            continue;

        mgapp_emit_char(codepoint);
    }
}

static int32_t mgapp_macos_utf8_offset(NSString *string, NSUInteger utf16_index)
{
    const NSUInteger length = [string length];
    if (utf16_index > length)
        utf16_index = length;

    if (utf16_index > 0 && utf16_index < length)
        utf16_index = [string rangeOfComposedCharacterSequenceAtIndex:utf16_index].location;

    return (int32_t)[[string substringToIndex:utf16_index] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
}

static void mgapp_macos_ime_set_composition(NSAttributedString *text, NSRange selection)
{
    NSString *string = [text string];
    mgapp_ime_clear_composition();
    mgapp_state.macos.marked_length = [string length];

    if ([string length] == 0)
        return;

    const char *utf8 = [string UTF8String];
    int32_t length = (int32_t)strlen(utf8);
    if (length >= MGAPP_IME_MAX_TEXT)
    {
        length = MGAPP_IME_MAX_TEXT - 1;
        while (length > 0 && (utf8[length] & 0xC0) == 0x80)
            --length;
    }

    memcpy(mgapp_state.ime.text, utf8, (size_t)length);
    mgapp_state.ime.text[length] = '\0';
    mgapp_state.ime.length = length;
    mgapp_state.ime.composing = true;

    const int32_t cursor = mgapp_macos_utf8_offset(string, NSMaxRange(selection));
    mgapp_state.ime.cursor = cursor < length ? cursor : length;

    [text enumerateAttribute:NSMarkedClauseSegmentAttributeName
        inRange:NSMakeRange(0, [string length])
        options:0
        usingBlock:^(id value, NSRange range, BOOL *stop)
    {
        int32_t start = mgapp_macos_utf8_offset(string, range.location);
        int32_t end = mgapp_macos_utf8_offset(string, NSMaxRange(range));
        if (start > length)
            start = length;
        if (end > length)
            end = length;

        const bool target = selection.length > 0 && NSIntersectionRange(range, selection).length > 0;
        mgapp_ime_push_clause(start, end, target);
    }];
}

static void mgapp_macos_update_size(bool emit)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    const float scale = (float)[macos_state->window backingScaleFactor];
    if (scale != macos_state->backing_scale)
    {
        macos_state->backing_scale = scale;
        mgapp_state.dpi_scale = scale;
        mgapp_state.dpi = (uint32_t)(96.0f * scale);
        [macos_state->layer setContentsScale:scale];
    }

    const NSRect backing = [macos_state->view convertRectToBacking:[macos_state->view bounds]];
    const int32_t width = (int32_t)backing.size.width;
    const int32_t height = (int32_t)backing.size.height;

    if (width == mgapp_state.window_width && height == mgapp_state.window_height)
        return;

    mgapp_state.window_width = width;
    mgapp_state.window_height = height;

    if (!emit)
        return;

    mgapp_event event = {
        .window = {
            .width = width,
            .height = height
        },
        .type = MGAPP_EVENT_RESIZE
    };
    mgapp_call_event(&event);
}

static mgapp_macos_edge mgapp_macos_hit_edge(NSPoint point)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    if (!(mgapp_state.flags & MGAPP_FLAG_NO_TITLEBAR) || (mgapp_state.flags & MGAPP_FLAG_NO_RESIZE) || macos_state->maximized)
        return MGAPP_MACOS_EDGE_NONE;

    const NSRect bounds = [macos_state->view bounds];
    mgapp_macos_edge edges = MGAPP_MACOS_EDGE_NONE;

    if (point.x <= MGAPP_MACOS_RESIZE_BORDER)
        edges |= MGAPP_MACOS_EDGE_LEFT;
    if (point.x >= bounds.size.width - MGAPP_MACOS_RESIZE_BORDER)
        edges |= MGAPP_MACOS_EDGE_RIGHT;
    if (point.y <= MGAPP_MACOS_RESIZE_BORDER)
        edges |= MGAPP_MACOS_EDGE_TOP;
    if (point.y >= bounds.size.height - MGAPP_MACOS_RESIZE_BORDER)
        edges |= MGAPP_MACOS_EDGE_BOTTOM;

    return edges;
}

static mg_cursor mgapp_macos_edge_cursor(mgapp_macos_edge edges)
{
    switch (edges)
    {
        case MGAPP_MACOS_EDGE_LEFT | MGAPP_MACOS_EDGE_TOP:
        case MGAPP_MACOS_EDGE_RIGHT | MGAPP_MACOS_EDGE_BOTTOM:
            return MG_CURSOR_RESIZE_NWSE;
        case MGAPP_MACOS_EDGE_RIGHT | MGAPP_MACOS_EDGE_TOP:
        case MGAPP_MACOS_EDGE_LEFT | MGAPP_MACOS_EDGE_BOTTOM:
            return MG_CURSOR_RESIZE_NESW;
        case MGAPP_MACOS_EDGE_LEFT:
        case MGAPP_MACOS_EDGE_RIGHT:
            return MG_CURSOR_RESIZE_EW;
        case MGAPP_MACOS_EDGE_TOP:
        case MGAPP_MACOS_EDGE_BOTTOM:
            return MG_CURSOR_RESIZE_NS;
        default:
            return MG_CURSOR_ARROW;
    }
}

static bool mgapp_macos_in_caption(int32_t x, int32_t y)
{
    return (mgapp_state.flags & MGAPP_FLAG_NO_TITLEBAR) &&
        x >= mgapp_state.caption.x && x < mgapp_state.caption.x + mgapp_state.caption.width &&
        y >= mgapp_state.caption.y && y < mgapp_state.caption.y + mgapp_state.caption.height;
}

static NSPoint mgapp_macos_track_mouse(NSPoint location_in_window)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    const NSPoint point = [macos_state->view convertPoint:location_in_window fromView:nil];
    const int32_t x = (int32_t)(point.x * macos_state->backing_scale);
    const int32_t y = (int32_t)(point.y * macos_state->backing_scale);
    mgapp_state.input.mouse.x = (int16_t)x;
    mgapp_state.input.mouse.y = (int16_t)y;

    const bool in_view = [macos_state->view mouse:point inRect:[macos_state->view bounds]];
    const mgapp_macos_edge edges = mgapp_macos_hit_edge(point);
    mgapp_state.input.mouse.inside = in_view && edges == MGAPP_MACOS_EDGE_NONE && !mgapp_macos_in_caption(x, y);

    if (in_view && macos_state->drag.mode == MGAPP_MACOS_DRAG_NONE)
        mgapp_macos_apply_cursor(edges != MGAPP_MACOS_EDGE_NONE ? mgapp_macos_edge_cursor(edges) : mgapp_state.current_cursor);

    return point;
}

static void mgapp_macos_mouse_moved(NSEvent *event)
{
    mgapp_macos_track_mouse([event locationInWindow]);
    mgapp_event mouse_event = {
        .mouse = {
            .x = mgapp_state.input.mouse.x,
            .y = mgapp_state.input.mouse.y
        },
        .type = MGAPP_EVENT_MOUSE_MOVE
    };
    mgapp_call_event(&mouse_event);
}

static void mgapp_macos_mouse_button(mg_mouse_button button, bool pressed)
{
    mgapp_input_process_mouse_button(button, pressed, mgapp_state.frame_time);
    mgapp_event event = {
        .mouse = {
            .button = button
        },
        .type = (mgapp_event_type)(pressed ? MGAPP_EVENT_MOUSE_DOWN : MGAPP_EVENT_MOUSE_UP)
    };
    mgapp_call_event(&event);
}

static void mgapp_macos_scroll(int8_t delta)
{
    mgapp_state.input.mouse.delta = delta;
    mgapp_event event = {
        .mouse = {
            .scroll = delta
        },
        .type = MGAPP_EVENT_MOUSE_SCROLL
    };
    mgapp_call_event(&event);
}

static void mgapp_macos_begin_drag(mgapp_macos_drag_mode mode, mgapp_macos_edge edges)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;
    macos_state->drag.mode = mode;
    macos_state->drag.edges = edges;
    macos_state->drag.start_mouse = [NSEvent mouseLocation];
    macos_state->drag.start_frame = [macos_state->window frame];
    macos_state->maximized = false;
}

static void mgapp_macos_continue_drag(void)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    const NSPoint mouse = [NSEvent mouseLocation];
    const CGFloat dx = mouse.x - macos_state->drag.start_mouse.x;
    const CGFloat dy = mouse.y - macos_state->drag.start_mouse.y;
    const NSRect start = macos_state->drag.start_frame;
    NSRect frame = start;

    if (macos_state->drag.mode == MGAPP_MACOS_DRAG_MOVE)
    {
        frame.origin.x += dx;
        frame.origin.y += dy;
        [macos_state->window setFrameOrigin:frame.origin];
        return;
    }

    const mgapp_macos_edge edges = macos_state->drag.edges;

    if (edges & MGAPP_MACOS_EDGE_LEFT)
    {
        frame.size.width = MAX(start.size.width - dx, MGAPP_MACOS_MIN_SIZE);
        frame.origin.x = NSMaxX(start) - frame.size.width;
    }
    if (edges & MGAPP_MACOS_EDGE_RIGHT)
        frame.size.width = MAX(start.size.width + dx, MGAPP_MACOS_MIN_SIZE);
    if (edges & MGAPP_MACOS_EDGE_BOTTOM)
    {
        frame.size.height = MAX(start.size.height - dy, MGAPP_MACOS_MIN_SIZE);
        frame.origin.y = NSMaxY(start) - frame.size.height;
    }
    if (edges & MGAPP_MACOS_EDGE_TOP)
        frame.size.height = MAX(start.size.height + dy, MGAPP_MACOS_MIN_SIZE);

    [macos_state->window setFrame:frame display:YES];
}

static void mgapp_macos_request_close(void)
{
    if (!mgapp_state.running || !mgapp_request_close())
        return;

    mgapp_event event = { .type = MGAPP_EVENT_WINDOW_CLOSE };
    mgapp_call_event(&event);
    mgapp_state.running = false;
}

@interface mgapp_macos_window : NSWindow
@end

@implementation mgapp_macos_window

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

@end

@interface mgapp_macos_view : NSView <NSTextInputClient>
{
    NSTrackingArea *tracking_area;
}
@end

@implementation mgapp_macos_view

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
        [self updateTrackingAreas];
    return self;
}

- (void)dealloc
{
    [tracking_area release];
    [super dealloc];
}

- (BOOL)isFlipped
{
    return YES;
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
    return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
    return NO;
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (CALayer *)makeBackingLayer
{
    return [CAMetalLayer layer];
}

- (void)updateTrackingAreas
{
    if (tracking_area)
    {
        [self removeTrackingArea:tracking_area];
        [tracking_area release];
    }

    const NSTrackingAreaOptions options =
        NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingCursorUpdate |
        NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingEnabledDuringMouseDrag;

    tracking_area = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil];
    [self addTrackingArea:tracking_area];
    [super updateTrackingAreas];
}

- (void)cursorUpdate:(NSEvent *)event
{
    mgapp_macos_track_mouse([event locationInWindow]);
}

- (void)mouseEntered:(NSEvent *)event
{
    mgapp_macos_track_mouse([event locationInWindow]);
}

- (void)mouseExited:(NSEvent *)event
{
    mgapp_state.input.mouse.inside = false;
    if (mgapp_state.macos.drag.mode == MGAPP_MACOS_DRAG_NONE)
        mgapp_macos_apply_cursor(MG_CURSOR_ARROW);
}

- (void)mouseMoved:(NSEvent *)event
{
    mgapp_macos_mouse_moved(event);
}

- (void)mouseDragged:(NSEvent *)event
{
    if (mgapp_state.macos.drag.mode != MGAPP_MACOS_DRAG_NONE)
    {
        mgapp_macos_continue_drag();
        return;
    }
    mgapp_macos_mouse_moved(event);
}

- (void)rightMouseDragged:(NSEvent *)event
{
    mgapp_macos_mouse_moved(event);
}

- (void)otherMouseDragged:(NSEvent *)event
{
    mgapp_macos_mouse_moved(event);
}

- (void)mouseDown:(NSEvent *)event
{
    const NSPoint point = mgapp_macos_track_mouse([event locationInWindow]);
    const mgapp_macos_edge edges = mgapp_macos_hit_edge(point);

    if (edges != MGAPP_MACOS_EDGE_NONE)
    {
        mgapp_macos_begin_drag(MGAPP_MACOS_DRAG_RESIZE, edges);
        return;
    }

    if (mgapp_macos_in_caption(mgapp_state.input.mouse.x, mgapp_state.input.mouse.y))
    {
        if ([event clickCount] == 2)
        {
            if (mgapp_maximized())
                mgapp_restore();
            else
                mgapp_maximize();
            return;
        }

        mgapp_macos_begin_drag(MGAPP_MACOS_DRAG_MOVE, MGAPP_MACOS_EDGE_NONE);
        return;
    }

    mgapp_macos_mouse_button(MG_MOUSE_BUTTON_LEFT, true);
}

- (void)mouseUp:(NSEvent *)event
{
    if (mgapp_state.macos.drag.mode != MGAPP_MACOS_DRAG_NONE)
    {
        mgapp_state.macos.drag.mode = MGAPP_MACOS_DRAG_NONE;
        mgapp_macos_track_mouse([event locationInWindow]);
        return;
    }

    mgapp_macos_track_mouse([event locationInWindow]);
    mgapp_macos_mouse_button(MG_MOUSE_BUTTON_LEFT, false);
}

- (void)rightMouseDown:(NSEvent *)event
{
    mgapp_macos_track_mouse([event locationInWindow]);
    mgapp_macos_mouse_button(MG_MOUSE_BUTTON_RIGHT, true);
}

- (void)rightMouseUp:(NSEvent *)event
{
    mgapp_macos_track_mouse([event locationInWindow]);
    mgapp_macos_mouse_button(MG_MOUSE_BUTTON_RIGHT, false);
}

- (void)otherMouseDown:(NSEvent *)event
{
    if ([event buttonNumber] != 2)
        return;

    mgapp_macos_track_mouse([event locationInWindow]);
    mgapp_macos_mouse_button(MG_MOUSE_BUTTON_MIDDLE, true);
}

- (void)otherMouseUp:(NSEvent *)event
{
    if ([event buttonNumber] != 2)
        return;

    mgapp_macos_track_mouse([event locationInWindow]);
    mgapp_macos_mouse_button(MG_MOUSE_BUTTON_MIDDLE, false);
}

- (void)scrollWheel:(NSEvent *)event
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;
    const float delta = (float)[event scrollingDeltaY];

    if (![event hasPreciseScrollingDeltas])
    {
        if (delta != 0.0f)
            mgapp_macos_scroll(delta > 0.0f ? 1 : -1);
        return;
    }

    if ([event phase] == NSEventPhaseBegan)
        macos_state->scroll_accumulator = 0.0f;

    macos_state->scroll_accumulator += delta;

    while (macos_state->scroll_accumulator >= MGAPP_MACOS_SCROLL_STEP)
    {
        macos_state->scroll_accumulator -= MGAPP_MACOS_SCROLL_STEP;
        mgapp_macos_scroll(1);
    }

    while (macos_state->scroll_accumulator <= -MGAPP_MACOS_SCROLL_STEP)
    {
        macos_state->scroll_accumulator += MGAPP_MACOS_SCROLL_STEP;
        mgapp_macos_scroll(-1);
    }
}

- (void)keyDown:(NSEvent *)event
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    const bool text_mode = mgapp_state.ime.mode == MGAPP_IME_MODE_TEXT;
    const bool was_composing = text_mode && macos_state->marked_length > 0;
    const mg_key key = mgapp_macos_translate_key([event keyCode]);

    if (key != MG_KEY_MAX && !was_composing)
        mgapp_macos_emit_key(key, true);

    if (text_mode)
    {
        macos_state->ime_handled = false;
        [self interpretKeyEvents:@[event]];
        if (macos_state->ime_handled || was_composing)
            return;
    }

    mgapp_macos_emit_key_chars(event);
}

- (void)keyUp:(NSEvent *)event
{
    const mg_key key = mgapp_macos_translate_key([event keyCode]);
    if (key != MG_KEY_MAX)
        mgapp_macos_emit_key(key, false);
}

- (void)flagsChanged:(NSEvent *)event
{
    const mg_key key = mgapp_macos_translate_key([event keyCode]);
    const NSEventModifierFlags flag = mgapp_macos_modifier_flag(key);
    if (key == MG_KEY_MAX || !flag)
        return;

    const bool pressed = ([event modifierFlags] & flag) && !mgapp_state.input.keyboard.keys[key];
    mgapp_macos_emit_key(key, pressed);
}

- (BOOL)hasMarkedText
{
    return mgapp_state.macos.marked_length > 0;
}

- (NSRange)markedRange
{
    const NSUInteger length = mgapp_state.macos.marked_length;
    return length > 0 ? NSMakeRange(0, length) : NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selected_range replacementRange:(NSRange)replacement_range
{
    NSAttributedString *text = [string isKindOfClass:[NSAttributedString class]]
        ? [string retain]
        : [[NSAttributedString alloc] initWithString:string];

    mgapp_macos_ime_set_composition(text, selected_range);
    [text release];
    mgapp_state.macos.ime_handled = true;
}

- (void)unmarkText
{
    mgapp_state.macos.marked_length = 0;
    mgapp_ime_clear_composition();
}

- (NSArray *)validAttributesForMarkedText
{
    return @[];
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actual_range
{
    return nil;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacement_range
{
    NSString *text = [string isKindOfClass:[NSAttributedString class]] ? [string string] : string;

    mgapp_state.macos.marked_length = 0;
    mgapp_ime_clear_composition();
    mgapp_macos_emit_text(text);
    mgapp_state.macos.ime_handled = true;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actual_range
{
    const float scale = mgapp_state.macos.backing_scale > 0.0f ? mgapp_state.macos.backing_scale : 1.0f;
    const NSRect area = NSMakeRect(
        (mgapp_state.ime.area_x + mgapp_state.ime.caret_x) / scale,
        mgapp_state.ime.area_y / scale,
        1.0,
        mgapp_state.ime.area_height / scale);

    return [[self window] convertRectToScreen:[self convertRect:area toView:nil]];
}

- (void)doCommandBySelector:(SEL)selector
{
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    return NSDragOperationCopy;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
    mgapp_macos_track_mouse([sender draggingLocation]);
    return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    NSPasteboard *pasteboard = [sender draggingPasteboard];
    NSDictionary *options = @{ NSPasteboardURLReadingFileURLsOnlyKey: @YES };
    NSArray *urls = [pasteboard readObjectsForClasses:@[[NSURL class]] options:options];
    const NSUInteger count = [urls count];
    if (count == 0)
        return NO;

    const char **paths = (const char**)malloc(sizeof(char*) * count);
    if (!paths)
        return NO;

    for (NSUInteger i = 0; i < count; ++i)
        paths[i] = [[urls objectAtIndex:i] fileSystemRepresentation];

    mgapp_event event = {
        .file_drop = {
            .paths = paths,
            .path_count = (int32_t)count
        },
        .type = MGAPP_EVENT_FILE_DROP
    };
    mgapp_call_event(&event);

    free(paths);
    return YES;
}

@end

@interface mgapp_macos_delegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation mgapp_macos_delegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    mgapp_macos_request_close();
    return NSTerminateCancel;
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
    mgapp_macos_request_close();
    return NO;
}

- (void)windowDidResize:(NSNotification *)notification
{
    mgapp_macos_update_size(true);
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification
{
    mgapp_macos_update_size(true);
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    mgapp_state.macos.drag.mode = MGAPP_MACOS_DRAG_NONE;
    mgapp_input_release_all_keys();
}

@end

static void mgapp_platform_ime_apply_area(void)
{
    NSView *view = mgapp_state.macos.view;
    if (view)
        [[view inputContext] invalidateCharacterCoordinates];
}

static void mgapp_platform_ime_cancel(void)
{
    NSView *view = mgapp_state.macos.view;
    if (view && mgapp_state.macos.marked_length > 0)
        [[view inputContext] discardMarkedText];
    mgapp_state.macos.marked_length = 0;
}

static void mgapp_platform_ime_set_mode(mgapp_ime_mode mode)
{
    if (mode == MGAPP_IME_MODE_TEXT)
    {
        mgapp_platform_ime_apply_area();
        return;
    }

    mgapp_platform_ime_cancel();
}

static inline void mgapp_macos_setup_clock(mgapp_macos_state *macos_state)
{
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    macos_state->clock_scale = (double)timebase.numer / (double)timebase.denom * 1e-9;
    macos_state->start_time = mach_absolute_time();
}

static void mgapp_macos_create_menu(NSString *app_name)
{
    NSMenu *menu_bar = [[NSMenu alloc] init];
    NSMenuItem *app_item = [[NSMenuItem alloc] init];
    NSMenu *app_menu = [[NSMenu alloc] init];

    [app_menu addItemWithTitle:[@"Hide " stringByAppendingString:app_name] action:@selector(hide:) keyEquivalent:@"h"];
    [app_menu addItem:[NSMenuItem separatorItem]];
    [app_menu addItemWithTitle:[@"Quit " stringByAppendingString:app_name] action:@selector(terminate:) keyEquivalent:@"q"];

    [app_item setSubmenu:app_menu];
    [menu_bar addItem:app_item];
    [NSApp setMainMenu:menu_bar];

    [app_menu release];
    [app_item release];
    [menu_bar release];
}

static void mgapp_macos_pump_events(void)
{
    for (;;)
    {
        NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
            untilDate:[NSDate distantPast]
            inMode:NSDefaultRunLoopMode
            dequeue:YES];
        if (!event)
            break;

        NSWindow *key_window = [NSApp keyWindow];
        if ([event type] == NSEventTypeKeyUp && ([event modifierFlags] & NSEventModifierFlagCommand) && key_window)
            [key_window sendEvent:event];
        else
            [NSApp sendEvent:event];
    }
}

int32_t mgapp_run(const mgapp_init_info *info)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        macos_state->delegate = [[mgapp_macos_delegate alloc] init];
        [NSApp setDelegate:macos_state->delegate];

        NSString *title = info->title ? [NSString stringWithUTF8String:info->title] : nil;
        if (!title)
            title = [[NSProcessInfo processInfo] processName];

        mgapp_macos_create_menu(title);
        [NSApp finishLaunching];

        mgapp_macos_setup_clock(macos_state);
        mgapp_macos_load_cursors(macos_state);

        mgapp_state.flags = info->flags;

        const CGFloat width = info->width ? (CGFloat)info->width : 800.0;
        const CGFloat height = info->height ? (CGFloat)info->height : 600.0;
        const NSRect content_rect = NSMakeRect(0.0, 0.0, width, height);

        NSWindowStyleMask style = NSWindowStyleMaskBorderless;
        if (!(info->flags & MGAPP_FLAG_NO_MINIMIZE))
            style |= NSWindowStyleMaskMiniaturizable;

        if (!(info->flags & MGAPP_FLAG_NO_TITLEBAR))
        {
            style |= NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
            if (!(info->flags & MGAPP_FLAG_NO_RESIZE))
                style |= NSWindowStyleMaskResizable;
        }

        macos_state->window = [[mgapp_macos_window alloc] initWithContentRect:content_rect
            styleMask:style
            backing:NSBackingStoreBuffered
            defer:NO];
        if (!macos_state->window)
        {
            MGAPP_ERROR("Window creation failed!");
            return 1;
        }

        [macos_state->window setReleasedWhenClosed:NO];
        [macos_state->window setAcceptsMouseMovedEvents:YES];
        [macos_state->window setDelegate:macos_state->delegate];
        [macos_state->window setTitle:title];
        [macos_state->window setHasShadow:YES];

        if (!(info->flags & MGAPP_FLAG_NO_TITLEBAR) && (info->flags & MGAPP_FLAG_NO_MAXIMIZE))
            [[macos_state->window standardWindowButton:NSWindowZoomButton] setEnabled:NO];

        macos_state->view = [[mgapp_macos_view alloc] initWithFrame:content_rect];
        [macos_state->view setWantsLayer:YES];
        macos_state->layer = [(CAMetalLayer*)[macos_state->view layer] retain];

        [macos_state->window setContentView:macos_state->view];
        [macos_state->window makeFirstResponder:macos_state->view];
        [macos_state->window center];

        if (info->flags & MGAPP_FLAG_ENABLE_FILE_DROPS)
            [macos_state->view registerForDraggedTypes:@[NSPasteboardTypeFileURL]];

        mgapp_macos_update_size(false);
        mgapp_platform_ime_set_mode(MGAPP_IME_MODE_OFF);

        if (!(info->flags & MGAPP_FLAG_HIDE_WINDOW))
            mgapp_show(true);

        mgapp_state.events = info->events;
        mgapp_state.running = true;
        mgapp_state.frame_time = mgapp_time();

        if (info->events.start)
            info->events.start();
    }

    while (mgapp_state.running)
    {
        @autoreleasepool
        {
            mgapp_macos_pump_events();

            const float new_time = mgapp_time();
            mgapp_state.delta_time = new_time - mgapp_state.frame_time;
            mgapp_state.frame_time = new_time;

            if (info->events.update)
                info->events.update();

            mgapp_input_frame();
        }
    }

    @autoreleasepool
    {
        if (info->events.end)
            info->events.end();

        if (macos_state->cursor_hidden)
            [NSCursor unhide];

        [macos_state->window setDelegate:nil];
        [macos_state->window orderOut:nil];
        [macos_state->window close];

        [macos_state->layer release];
        [macos_state->view release];
        [macos_state->window release];
        [NSApp setDelegate:nil];
        [macos_state->delegate release];

        for (int32_t i = 0; i < MG_CURSOR_MAX; ++i)
            [macos_state->cursor_table[i] release];

        macos_state->layer = nil;
        macos_state->view = nil;
        macos_state->window = nil;
        macos_state->delegate = nil;
    }

    mgapp_clipboard_cache(NULL, 0);
    return 0;
}

void mgapp_close(void)
{
    mgapp_macos_request_close();
}

void mgapp_show(bool value)
{
    NSWindow *window = mgapp_state.macos.window;

    if (!value)
    {
        [window orderOut:nil];
        return;
    }

    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

void mgapp_minimize(void)
{
    [mgapp_state.macos.window miniaturize:nil];
}

void mgapp_maximize(void)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;
    if (macos_state->maximized)
        return;

    NSScreen *screen = [macos_state->window screen];
    if (!screen)
        screen = [NSScreen mainScreen];

    macos_state->restore_frame = [macos_state->window frame];
    macos_state->maximized = true;
    [macos_state->window setFrame:[screen visibleFrame] display:YES];
}

void mgapp_restore(void)
{
    mgapp_macos_state *macos_state = &mgapp_state.macos;

    if ([macos_state->window isMiniaturized])
    {
        [macos_state->window deminiaturize:nil];
        return;
    }

    if (!macos_state->maximized)
        return;

    macos_state->maximized = false;
    [macos_state->window setFrame:macos_state->restore_frame display:YES];
}

bool mgapp_maximized(void)
{
    return mgapp_state.macos.maximized;
}

void mgapp_set_cursor(mg_cursor cursor)
{
    if (mgapp_state.current_cursor == cursor)
        return;

    mgapp_state.current_cursor = cursor;
    if (mgapp_state.input.mouse.inside)
        mgapp_macos_apply_cursor(cursor);
}

void mgapp_set_clipboard(const char *text)
{
    if (!text)
        return;

    @autoreleasepool
    {
        NSString *string = [NSString stringWithUTF8String:text];
        if (!string)
            return;

        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        [pasteboard setString:string forType:NSPasteboardTypeString];
    }
}

const char *mgapp_get_clipboard(void)
{
    @autoreleasepool
    {
        NSString *string = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
        const char *utf8 = string ? [string UTF8String] : NULL;
        if (!utf8)
        {
            mgapp_clipboard_cache(NULL, 0);
            return NULL;
        }

        mgapp_clipboard_cache(utf8, strlen(utf8));
    }

    return mgapp_state.clipboard_buffer;
}

float mgapp_time(void)
{
    const uint64_t elapsed = mach_absolute_time() - mgapp_state.macos.start_time;
    return (float)((double)elapsed * mgapp_state.macos.clock_scale);
}

void *mgapp_primary_handle(void)
{
    return (void*)mgapp_state.macos.layer;
}

void *mgapp_secondary_handle(void)
{
    return (void*)mgapp_state.macos.window;
}

#endif

#endif
