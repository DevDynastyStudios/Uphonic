#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef MG_APP_API
#if defined(_WIN32) && defined(MG_DLL) && defined(MG_APP_IMPL)
#define MG_APP_API __declspec(dllexport)
#elif defined(_WIN32) && defined(MG_DLL)
#define MG_APP_API __declspec(dllimport)
#else
#define MG_APP_API extern
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

typedef uint8_t mg_app_event_type;
enum
{
    MG_APP_EVENT_RESIZE,
    MG_APP_EVENT_KEY_DOWN,
    MG_APP_EVENT_KEY_UP,
    MG_APP_EVENT_CHAR,
    MG_APP_EVENT_MOUSE_DOWN,
    MG_APP_EVENT_MOUSE_UP,
    MG_APP_EVENT_MOUSE_MOVE,
    MG_APP_EVENT_MOUSE_SCROLL
};

typedef struct
{
    union
    {
        struct
        {
            int32_t window_width, window_height;
        };
        struct
        {
            int32_t mouse_x, mouse_y;
        };
        int8_t scroll_delta;
        mg_key key;
        mg_mouse_button mouse_button;
        char ch;
    };
    
    mg_app_event_type type;
}
mg_app_event;

typedef uint32_t mg_app_flags;
enum
{
    MG_APP_FLAG_NO_TITLEBAR = 1 << 0,
    MG_APP_FLAG_HIDDEN = 1 << 1
};

typedef struct
{
    const char *title;
    mg_app_flags flags;
    uint32_t width, height;
    struct
    {
        void (*start)(void);
        void (*end)(void);
        void (*update)(void);
        void (*event)(const mg_app_event *event);
    }
    events;
}
mg_app_init_info;

MG_APP_API int32_t mg_app_run(const mg_app_init_info *info);
MG_APP_API void mg_app_close(void);
MG_APP_API void mg_app_show(bool value);
MG_APP_API void mg_app_minimize(void);
MG_APP_API void mg_app_maximize(void);
MG_APP_API void mg_app_restore(void);
MG_APP_API bool mg_app_maximized(void);

MG_APP_API float mg_app_time(void);
MG_APP_API float mg_app_delta_time(void);

MG_APP_API int32_t mg_app_width(void);
MG_APP_API int32_t mg_app_height(void);

MG_APP_API void mg_app_set_cursor(mg_cursor cursor);

MG_APP_API bool mg_app_key_down(mg_key key);
MG_APP_API bool mg_app_key_pressed(mg_key key);

MG_APP_API bool mg_app_mouse_down(mg_mouse_button button);
MG_APP_API bool mg_app_mouse_pressed(mg_mouse_button button);
MG_APP_API bool mg_app_mouse_released(mg_mouse_button button);
MG_APP_API bool mg_app_mouse_clicked(mg_mouse_button button);
MG_APP_API bool mg_app_mouse_double_clicked(mg_mouse_button button);

MG_APP_API int8_t mg_app_mouse_scroll_delta(void);
MG_APP_API int32_t mg_app_mouse_x(void);
MG_APP_API int32_t mg_app_mouse_y(void);

MG_APP_API uint32_t mg_app_dpi(void);
MG_APP_API float mg_app_dpi_scale(void);

MG_APP_API void mg_app_set_caption_area(int32_t x, int32_t y, int32_t width, int32_t height);

MG_APP_API void *mg_app_primary_handle(void);
MG_APP_API void *mg_app_secondary_handle(void);

#ifdef __cplusplus
} // extern "C"

inline int32_t mg_app_run(const mg_app_init_info &info) { return mg_app_run(&info); }

#endif

#define MG_DEFINE_APP(name) \
int main(void) { \
    extern void mg_app_on_start(void); \
    extern void mg_app_on_end(void); \
    extern void mg_app_on_update(void); \
    extern void mg_app_on_event(const mg_app_event *event); \
    mg_app_init_info info = { \
        .title = name, \
        .events = { \
            .start  = mg_app_on_start, \
            .end    = mg_app_on_end, \
            .update = mg_app_on_update, \
            .event  = mg_app_on_event \
        } \
    }; \
    return mg_app_run(&info); \
}

#if defined(MG_IMPL) || defined(MGAPP_IMPL)

#include <string.h>

#ifndef MG_CLICK_TIME_THRESHOLD
    #define MG_CLICK_TIME_THRESHOLD 0.5f
#endif

#ifndef MG_CLICK_MOVE_THRESHOLD
    #define MG_CLICK_MOVE_THRESHOLD 4
#endif

#ifndef MG_DOUBLE_CLICK_TIME_THRESHOLD
    #define MG_DOUBLE_CLICK_TIME_THRESHOLD 0.4f
#endif

#ifndef MG_DOUBLE_CLICK_MOVE_THRESHOLD
    #define MG_DOUBLE_CLICK_MOVE_THRESHOLD 6
#endif

typedef struct
{
    struct
    {
        bool keys[MG_KEY_MAX];
        bool keys_pressed[MG_KEY_MAX];
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
        bool buttons_pressed[4];
        bool buttons_released[4];
        bool buttons_clicked[4];
        bool buttons_double_clicked[4];
    }
    mouse;
}
mg_app_input_state;

static mg_app_input_state input_state;

static inline void mg_app_input_process_key(mg_key key, bool pressed)
{
    if (pressed && !input_state.keyboard.keys[key])
        input_state.keyboard.keys_pressed[key] = true;
    input_state.keyboard.keys[key] = pressed;
}

static inline void mg_app_input_process_mouse_button(mg_mouse_button button, bool pressed, float time_now)
{
    if (pressed && !input_state.mouse.buttons[button])
    {
        input_state.mouse.buttons_pressed[button] = true;
        input_state.mouse.down_time[button] = time_now;
        input_state.mouse.down_x[button] = input_state.mouse.x;
        input_state.mouse.down_y[button] = input_state.mouse.y;
    }

    if (!pressed && input_state.mouse.buttons[button])
    {
        input_state.mouse.buttons_released[button] = true;

        float held = time_now - input_state.mouse.down_time[button];
        int16_t dx = input_state.mouse.x - input_state.mouse.down_x[button];
        int16_t dy = input_state.mouse.y - input_state.mouse.down_y[button];
        int32_t move_sq = (int32_t)dx * dx + (int32_t)dy * dy;

        if (held <= MG_CLICK_TIME_THRESHOLD &&
            move_sq <= (MG_CLICK_MOVE_THRESHOLD * MG_CLICK_MOVE_THRESHOLD))
        {
            input_state.mouse.buttons_clicked[button] = true;

            float since_last = time_now - input_state.mouse.last_click_time[button];
            int16_t ldx = input_state.mouse.x - input_state.mouse.last_click_x[button];
            int16_t ldy = input_state.mouse.y - input_state.mouse.last_click_y[button];
            int32_t last_move_sq = (int32_t)ldx * ldx + (int32_t)ldy * ldy;

            if (since_last <= MG_DOUBLE_CLICK_TIME_THRESHOLD &&
                last_move_sq <= (MG_DOUBLE_CLICK_MOVE_THRESHOLD * MG_DOUBLE_CLICK_MOVE_THRESHOLD))
            {
                input_state.mouse.buttons_double_clicked[button] = true;
                input_state.mouse.last_click_time[button] = -1000.0f;
            }
            else
            {
                input_state.mouse.last_click_time[button] = time_now;
                input_state.mouse.last_click_x[button] = input_state.mouse.x;
                input_state.mouse.last_click_y[button] = input_state.mouse.y;
            }
        }
    }

    input_state.mouse.buttons[button] = pressed;
}

static inline void mg_app_input_frame(void)
{
    input_state.mouse.delta = 0;
    memset(input_state.keyboard.keys_pressed, 0, sizeof(input_state.keyboard.keys_pressed));
    memset(input_state.mouse.buttons_pressed, 0, sizeof(input_state.mouse.buttons_pressed));
    memset(input_state.mouse.buttons_released, 0, sizeof(input_state.mouse.buttons_released));
    memset(input_state.mouse.buttons_clicked, 0, sizeof(input_state.mouse.buttons_clicked));
    memset(input_state.mouse.buttons_double_clicked, 0, sizeof(input_state.mouse.buttons_double_clicked));
}

bool mg_app_key_down(mg_key key)
{
    return input_state.keyboard.keys[key];
}

bool mg_app_key_pressed(mg_key key)
{
    return input_state.keyboard.keys_pressed[key];
}

bool mg_app_mouse_down(mg_mouse_button button)
{
    return input_state.mouse.buttons[button];
}

bool mg_app_mouse_pressed(mg_mouse_button button)
{
    return input_state.mouse.buttons_pressed[button];
}

bool mg_app_mouse_released(mg_mouse_button button)
{
    return input_state.mouse.buttons_released[button];
}

bool mg_app_mouse_clicked(mg_mouse_button button)
{
    return input_state.mouse.buttons_clicked[button];
}

bool mg_app_mouse_double_clicked(mg_mouse_button button)
{
    return input_state.mouse.buttons_double_clicked[button];
}

int8_t mg_app_mouse_scroll_delta(void)
{
    return input_state.mouse.delta;
}

int32_t mg_app_mouse_x(void)
{
    return input_state.mouse.x;
}

int32_t mg_app_mouse_y(void)
{
    return input_state.mouse.y;
}

#if defined(__EMSCRIPTEN__)

#include <emscripten.h>
#include <emscripten/html5.h>

typedef struct mg_emscripten_platform
{
    int32_t window_width, window_height;
    void (*on_event_call)(const mg_app_event *event);
    float time, delta_time;
    bool running;
    const mg_app_init_info *info;
}
mg_emscripten_platform;

static mg_emscripten_platform mg_app_state;

static inline void mg_app_call_event(const mg_app_event *event)
{
    if (mg_app_state.on_event_call)
        mg_app_state.on_event_call(event);
}

static EM_BOOL mg_em_resize_callback(int event_type, const EmscriptenUiEvent *ui_event, void *user_data)
{
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);

    mg_app_state.window_width  = (int32_t)w;
    mg_app_state.window_height = (int32_t)h;

    mg_app_event event = {
        .window_width  = mg_app_state.window_width,
        .window_height = mg_app_state.window_height,
        .type          = MG_APP_EVENT_RESIZE
    };
    mg_app_call_event(&event);

    return EM_TRUE;
}

static void mg_em_frame(void *user_data)
{
    double now = emscripten_get_now() * 0.001;
    mg_app_state.delta_time = (float)(now - mg_app_state.time);
    mg_app_state.time = (float)now;

    if (mg_app_state.info->events.update)
        mg_app_state.info->events.update();

    mg_app_input_frame();
}

int32_t mg_app_run(const mg_app_init_info *info)
{
    mg_app_state.info = info;
    mg_app_state.on_event_call = info->events.event;
    mg_app_state.running = true;

    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    mg_app_state.window_width  = (int32_t)w;
    mg_app_state.window_height = (int32_t)h;

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, mg_em_resize_callback);

    mg_app_state.time = (float)(emscripten_get_now() * 0.001);
    mg_app_state.delta_time = 0.0f;

    if (info->events.start)
        info->events.start();

    emscripten_set_main_loop_arg(mg_em_frame, NULL, 0, 1);

    return 0;
}

void mg_app_close(void)
{
    emscripten_cancel_main_loop();
    mg_app_state.running = false;

    if (mg_app_state.info && mg_app_state.info->events.end)
        mg_app_state.info->events.end();
}

void mg_app_set_cursor(mg_cursor cursor)
{
    // TODO (box): Implement this
}

float mg_app_time(void)
{
    return mg_app_state.time;
}

float mg_app_delta_time(void)
{
    return mg_app_state.delta_time;
}

int32_t mg_app_width(void)
{
    return mg_app_state.window_width;
}

int32_t mg_app_height(void)
{
    return mg_app_state.window_height;
}

void *mg_app_primary_handle(void)
{
    return NULL;
}

#elif defined(_WIN32)

#include <windows.h>
#include <windowsx.h>
#pragma comment(lib, "user32")

typedef struct mg_win32_platform
{
    HWND hwnd;
    HINSTANCE hinstace;
    int32_t window_width, window_height;
    uint32_t dpi;
    float dpi_scale;

    void (*on_event_call)(const mg_app_event *event);
    float time, delta_time;

    HCURSOR cursors[MG_CURSOR_MAX];
    mg_cursor current_cursor;

    WNDPROC original_proc;
    int32_t caption_x, caption_y, caption_width, caption_height;

    const mg_app_init_info *info;
}
mg_win32_platform;

static mg_win32_platform mg_app_state;

static float clock_frequency;
static LARGE_INTEGER start_time;

static inline void mg_app_call_event(const mg_app_event *event)
{
    if (mg_app_state.on_event_call)
        mg_app_state.on_event_call(event);
}

static void mg_app_setup_clock(void)
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0f / (float)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);
}

static LRESULT CALLBACK mg_win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
        {
            RECT r;
            GetClientRect(hwnd, &r);
            mg_app_state.window_width  = r.right - r.left;
            mg_app_state.window_height = r.bottom - r.top;
            mg_app_event event = {
                .window_width = mg_app_state.window_width,
                .window_height = mg_app_state.window_height,
                .type = MG_APP_EVENT_RESIZE
            };
            mg_app_call_event(&event);
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

            mg_app_input_process_key(key, pressed);
            mg_app_event event = {
                .key = key,
                .type = (mg_app_event_type)(pressed ? MG_APP_EVENT_KEY_DOWN : MG_APP_EVENT_KEY_UP)
            };
            mg_app_call_event(&event);
            break;
        }
        case WM_CHAR:
        {
            mg_app_event event = {
                .ch = (char)w_param,
                .type = MG_APP_EVENT_CHAR
            };
            mg_app_call_event(&event);
        break;
        }
        case WM_MOUSEMOVE:
        {
            int32_t x = GET_X_LPARAM(l_param);
            int32_t y = GET_Y_LPARAM(l_param);
            input_state.mouse.x = x;
            input_state.mouse.y = y;
            mg_app_event event = {
                .mouse_x = x,
                .mouse_y = y,
                .type = MG_APP_EVENT_MOUSE_MOVE
            };
            mg_app_call_event(&event);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int32_t delta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (delta != 0)
            {
                delta = (delta < 0) ? -1 : 1;
                input_state.mouse.delta = (int8_t)delta;
                mg_app_event event = {
                    .scroll_delta = (int8_t)delta,
                    .type = MG_APP_EVENT_MOUSE_SCROLL
                };
                mg_app_call_event(&event);
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

            mg_app_input_process_mouse_button(mouse_button, pressed, mg_app_state.time);
            mg_app_event event = {
                .mouse_button = mouse_button,
                .type = (mg_app_event_type)(pressed ? MG_APP_EVENT_MOUSE_DOWN : MG_APP_EVENT_MOUSE_UP)
            };
            mg_app_call_event(&event);
            break;
        }
        case WM_SETCURSOR:
        {
            if (LOWORD(l_param) == HTCLIENT)
            {
                SetCursor(mg_app_state.cursors[platform.current_cursor]);
                return TRUE;
            }
            break;
        }
        case WM_DPICHANGED:
        {
            mg_app_state.dpi = HIWORD(w_param);
            mg_app_state.dpi_scale = (float)mg_app_state.dpi / 96.0f;
            RECT *suggested = (RECT*)l_param;
            SetWindowPos(hwnd, NULL,
                suggested->left, suggested->top,
                suggested->right - suggested->left,
                suggested->bottom - suggested->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
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
                LARGE_INTEGER now_time;
                QueryPerformanceCounter(&now_time);
                float new_time = (float)(now_time.QuadPart - start_time.QuadPart) * clock_frequency;
                mg_app_state.delta_time = new_time - mg_app_state.time;
                mg_app_state.time = new_time;

                if (mg_app_state.info && mg_app_state.info->events.update)
                    mg_app_state.info->events.update();

                mg_app_input_frame();
            }
            break;
        }
    }
    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

static LRESULT CALLBACK mg_win32_no_titlebar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_NCCALCSIZE:
        {
            if (!w_param) break;
            NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS *)l_param;
            RECT *r = params->rgrc;
            int32_t bx = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            int32_t by = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            r->left   += bx;
            r->right  -= bx;
            r->bottom -= by;
            r->top    += IsZoomed(hwnd) ? by : 0;
            return 0;
        }
        case WM_NCHITTEST:
        {
            POINTS mp = MAKEPOINTS(l_param);
            POINT  sp = { mp.x, mp.y };
            RECT   wr;
            GetWindowRect(hwnd, &wr);
            const int bw = 8;
            int rx = sp.x - wr.left;
            int ry = sp.y - wr.top;
            int w  = wr.right  - wr.left;
            int h  = wr.bottom - wr.top;

            if (ry >= h - bw) return (rx <= bw) ? HTBOTTOMLEFT : (rx >= w - bw) ? HTBOTTOMRIGHT : HTBOTTOM;
            if (ry <= bw) return (rx <= bw) ? HTTOPLEFT : (rx >= w - bw) ? HTTOPRIGHT    : HTTOP;
            if (rx <= bw) return HTLEFT;
            if (rx >= w - bw) return HTRIGHT;

            if (rx >= mg_app_state.caption_x && rx < mg_app_state.caption_x + mg_app_state.caption_width &&
                ry >= mg_app_state.caption_y && ry < mg_app_state.caption_y + mg_app_state.caption_height)
                return HTCAPTION;
            break;
        }
        case WM_NCMOUSEMOVE:
        {
            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            ScreenToClient(mg_app_state.hwnd, &pt);
            input_state.mouse.x = (int16_t)pt.x;
            input_state.mouse.y = (int16_t)pt.y;
            mg_app_event event = {
                .mouse_x = pt.x,
                .mouse_y = pt.y,
                .type = MG_APP_EVENT_MOUSE_MOVE
            };
            mg_app_call_event(&event);
            break;
        }

        case WM_NCACTIVATE:
            break;
    }
    return CallWindowProc(mg_app_state.original_proc, hwnd, msg, w_param, l_param);
}

int32_t mg_app_run(const mg_app_init_info *info)
{
    const char *CLASS_NAME = "MAGMA";
    mg_app_state.hinstace = GetModuleHandleA(NULL);
    mg_app_state.info = info;

    mg_app_setup_clock();

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    const LPCSTR win32_cursor_map[MG_CURSOR_MAX] = {
        (LPCSTR)IDC_ARROW,
        (LPCSTR)IDC_IBEAM,
        (LPCSTR)IDC_CROSS,
        (LPCSTR)IDC_HAND,
        (LPCSTR)IDC_SIZENS,
        (LPCSTR)IDC_SIZEWE,
        (LPCSTR)IDC_SIZENESW,
        (LPCSTR)IDC_SIZENWSE,
        (LPCSTR)IDC_SIZEALL,
        (LPCSTR)IDC_NO,
        NULL
    };

    for (uint8_t i = 0; i < MG_CURSOR_MAX; i++)
        mg_app_state.cursors[i] = win32_cursor_map[i] ? LoadCursorA(NULL, win32_cursor_map[i]) : NULL;

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = mg_win32_process_message;
    wc.hInstance = mg_app_state.hinstace;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = mg_app_state.cursors[MG_CURSOR_ARROW];

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(NULL, "Window registration failed!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    const int32_t width = info->width ? info->width : CW_USEDEFAULT;
    const int32_t height = info->height ? info->height : CW_USEDEFAULT;

    mg_app_state.hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        info->title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL,
        mg_app_state.hinstace,
        NULL
    );

    if (!platform.hwnd)
    {
        MessageBoxA(NULL, "Window creation failed!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    mg_app_state.dpi = GetDpiForWindow(mg_app_state.hwnd);
    mg_app_state.dpi_scale = (float)mg_app_state.dpi / 96.0f;

    if (info->flags & MG_APP_FLAG_NO_TITLEBAR)
    {
        LONG_PTR style = GetWindowLongPtr(mg_app_state.hwnd, GWL_STYLE);
        style |= WS_THICKFRAME | WS_CAPTION;
        SetWindowLongPtr(mg_app_state.hwnd, GWL_STYLE, style);

        RECT wr;
        GetWindowRect(mg_app_state.hwnd, &wr);
        int w = wr.right - wr.left;
        int h = wr.bottom - wr.top;

        mg_app_state.original_proc = (WNDPROC)GetWindowLongPtr(mg_app_state.hwnd, GWLP_WNDPROC);
        SetWindowLongPtr(mg_app_state.hwnd, GWLP_WNDPROC, (LONG_PTR)mg_win32_no_titlebar_proc);
        SetWindowPos(mg_app_state.hwnd, NULL, 0, 0, w, h, SWP_FRAMECHANGED | SWP_NOMOVE);
    }

    ShowWindow(mg_app_state.hwnd, info->flags & MG_APP_FLAG_HIDDEN ? SW_HIDE : SW_SHOW);
    UpdateWindow(mg_app_state.hwnd);

    mg_app_state.on_event_call = info->events.event;

    if (info->events.start)
        info->events.start();

    bool running = true;
    while (running)
    {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        LARGE_INTEGER now_time;
        QueryPerformanceCounter(&now_time);
        float new_time = (float)(now_time.QuadPart - start_time.QuadPart) * clock_frequency;
        mg_app_state.delta_time = new_time - mg_app_state.time;
        mg_app_state.time = new_time;

        mg_app_set_cursor(MG_CURSOR_ARROW);

        if (info->events.update)
            info->events.update();

        POINT pt;
        GetCursorPos(&pt);
        HWND hovered = WindowFromPoint(pt);
        if (hovered == mg_app_state.hwnd)
        {
            LRESULT ht = SendMessage(mg_app_state.hwnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
            SendMessage(mg_app_state.hwnd, WM_SETCURSOR, (WPARAM)mg_app_state.hwnd, MAKELPARAM(ht, WM_MOUSEMOVE));
        }

        mg_app_input_frame();
    }

    if (info->events.end)
        info->events.end();

    DestroyWindow(mg_app_state.hwnd);
    UnregisterClassA(CLASS_NAME, mg_app_state.hinstace);
    return 0;
}

void mg_app_close(void)
{
    PostMessage(mg_app_state.hwnd, WM_CLOSE, 0, 0);
}

void mg_app_show(bool value)
{
    ShowWindow(mg_app_state.hwnd, value);
}

void mg_app_minimize(void)
{
    ShowWindow(mg_app_state.hwnd, SW_MINIMIZE);
}

void mg_app_maximize(void)
{
    ShowWindow(mg_app_state.hwnd, SW_MAXIMIZE);
}

void mg_app_restore(void)
{
    ShowWindow(mg_app_state.hwnd, SW_RESTORE);
}

bool mg_app_maximized(void)
{
    return IsZoomed(mg_app_state.hwnd);
}

void mg_app_set_cursor(mg_cursor cursor)
{
    mg_app_state.current_cursor = cursor;
}

float mg_app_time(void)
{
    return mg_app_state.time;
}

float mg_app_delta_time(void)
{
    return mg_app_state.delta_time;
}

int32_t mg_app_width(void)
{
    return mg_app_state.window_width;
}

int32_t mg_app_height(void)
{
    return mg_app_state.window_height;
}

uint32_t mg_app_dpi(void)
{
    return mg_app_state.dpi;
}

float mg_app_dpi_scale(void)
{
    return mg_app_state.dpi_scale;
}

void mg_app_set_caption_area(int32_t x, int32_t y, int32_t width, int32_t height)
{
    mg_app_state.caption_x = x;
    mg_app_state.caption_y = y;
    mg_app_state.caption_width = width;
    mg_app_state.caption_height = height;
}

void *mg_app_primary_handle(void)
{
    return (void*)mg_app_state.hwnd;
}

void *mg_app_secondary_handle(void)
{
    return (void*)mg_app_state.hinstace;
}

#elif defined(__linux__)
 
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
 
typedef struct mg_xlib_platform
{
    Window window;
    Display *display;
    Atom wm_delete_window;
    Cursor cursors[MG_CURSOR_MAX];
    Cursor current_cursor;
    void (*on_event_call)(const mg_app_event *event);
    int screen;
    int32_t window_width, window_height;
    float time, delta_time;
    uint32_t dpi;
    float dpi_scale;
    bool running;
    bool no_titlebar;

    int32_t caption_x, caption_y, caption_width, caption_height;

    struct
    {
        Atom wm_state, max_horz, max_vert, moveresize, motif_hints;
    } atoms;
}
mg_xlib_platform;
 
static mg_xlib_platform mg_app_state;
 
static inline void mg_app_call_event(const mg_app_event *event)
{
    if (mg_app_state.on_event_call)
        mg_app_state.on_event_call(event);
}
 
static float mg_xlib_get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (float)ts.tv_sec + (float)ts.tv_nsec * 1e-9f;
}

static mg_key mg_xlib_translate_key(KeySym sym)
{
    switch (sym)
    {
        case XK_a: case XK_A: return MG_KEY_A;
        case XK_b: case XK_B: return MG_KEY_B;
        case XK_c: case XK_C: return MG_KEY_C;
        case XK_d: case XK_D: return MG_KEY_D;
        case XK_e: case XK_E: return MG_KEY_E;
        case XK_f: case XK_F: return MG_KEY_F;
        case XK_g: case XK_G: return MG_KEY_G;
        case XK_h: case XK_H: return MG_KEY_H;
        case XK_i: case XK_I: return MG_KEY_I;
        case XK_j: case XK_J: return MG_KEY_J;
        case XK_k: case XK_K: return MG_KEY_K;
        case XK_l: case XK_L: return MG_KEY_L;
        case XK_m: case XK_M: return MG_KEY_M;
        case XK_n: case XK_N: return MG_KEY_N;
        case XK_o: case XK_O: return MG_KEY_O;
        case XK_p: case XK_P: return MG_KEY_P;
        case XK_q: case XK_Q: return MG_KEY_Q;
        case XK_r: case XK_R: return MG_KEY_R;
        case XK_s: case XK_S: return MG_KEY_S;
        case XK_t: case XK_T: return MG_KEY_T;
        case XK_u: case XK_U: return MG_KEY_U;
        case XK_v: case XK_V: return MG_KEY_V;
        case XK_w: case XK_W: return MG_KEY_W;
        case XK_x: case XK_X: return MG_KEY_X;
        case XK_y: case XK_Y: return MG_KEY_Y;
        case XK_z: case XK_Z: return MG_KEY_Z;

        case XK_0: return MG_KEY_0;
        case XK_1: return MG_KEY_1;
        case XK_2: return MG_KEY_2;
        case XK_3: return MG_KEY_3;
        case XK_4: return MG_KEY_4;
        case XK_5: return MG_KEY_5;
        case XK_6: return MG_KEY_6;
        case XK_7: return MG_KEY_7;
        case XK_8: return MG_KEY_8;
        case XK_9: return MG_KEY_9;

        case XK_F1:  return MG_KEY_F1;
        case XK_F2:  return MG_KEY_F2;
        case XK_F3:  return MG_KEY_F3;
        case XK_F4:  return MG_KEY_F4;
        case XK_F5:  return MG_KEY_F5;
        case XK_F6:  return MG_KEY_F6;
        case XK_F7:  return MG_KEY_F7;
        case XK_F8:  return MG_KEY_F8;
        case XK_F9:  return MG_KEY_F9;
        case XK_F10: return MG_KEY_F10;
        case XK_F11: return MG_KEY_F11;
        case XK_F12: return MG_KEY_F12;

        case XK_Left:      return MG_KEY_LEFT;
        case XK_Right:     return MG_KEY_RIGHT;
        case XK_Up:        return MG_KEY_UP;
        case XK_Down:      return MG_KEY_DOWN;
        case XK_Home:      return MG_KEY_HOME;
        case XK_End:       return MG_KEY_END;
        case XK_Page_Up:   return MG_KEY_PAGE_UP;
        case XK_Page_Down: return MG_KEY_PAGE_DOWN;
        case XK_Insert:    return MG_KEY_INSERT;
        case XK_Delete:    return MG_KEY_DELETE;

        case XK_Shift_L:   return MG_KEY_LSHIFT;
        case XK_Shift_R:   return MG_KEY_RSHIFT;
        case XK_Control_L: return MG_KEY_LCONTROL;
        case XK_Control_R: return MG_KEY_RCONTROL;
        case XK_Alt_L:     return MG_KEY_LALT;
        case XK_Alt_R:     return MG_KEY_RALT;
        case XK_Super_L:   return MG_KEY_LSUPER;
        case XK_Super_R:   return MG_KEY_RSUPER;
        case XK_Escape:    return MG_KEY_ESCAPE;
        case XK_Tab:       return MG_KEY_TAB;
        case XK_Return:    return MG_KEY_ENTER;
        case XK_BackSpace: return MG_KEY_BACKSPACE;
        case XK_space:     return MG_KEY_SPACE;

        case XK_comma:        return MG_KEY_COMMA;
        case XK_period:       return MG_KEY_PERIOD;
        case XK_minus:        return MG_KEY_MINUS;
        case XK_equal:        return MG_KEY_EQUAL;
        case XK_semicolon:    return MG_KEY_SEMICOLON;
        case XK_grave:        return MG_KEY_GRAVE;
        case XK_slash:        return MG_KEY_SLASH;
        case XK_backslash:    return MG_KEY_BACKSLASH;
        case XK_bracketleft:  return MG_KEY_LBRACKET;
        case XK_bracketright: return MG_KEY_RBRACKET;
        case XK_apostrophe:   return MG_KEY_APOSTROPHE;

        default: return MG_KEY_MAX;
    }
}

typedef struct
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
}
mg_motif_wm_hints;

#define MG_MWM_HINTS_DECORATIONS (1L << 1)

static void mg_xlib_set_borderless(Display *display, Window window, Atom motif_hints_atom, bool borderless)
{
    mg_motif_wm_hints hints = {0};
    hints.flags = MG_MWM_HINTS_DECORATIONS;
    hints.decorations = borderless ? 0 : 1;

    XChangeProperty(
        display, window,
        motif_hints_atom, motif_hints_atom,
        32, PropModeReplace,
        (unsigned char *)&hints,
        sizeof(mg_motif_wm_hints) / sizeof(long));
}

enum
{
    MG_NET_WM_MOVERESIZE_SIZE_TOPLEFT = 0,
    MG_NET_WM_MOVERESIZE_SIZE_TOP = 1,
    MG_NET_WM_MOVERESIZE_SIZE_TOPRIGHT = 2,
    MG_NET_WM_MOVERESIZE_SIZE_RIGHT = 3,
    MG_NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT = 4,
    MG_NET_WM_MOVERESIZE_SIZE_BOTTOM = 5,
    MG_NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT = 6,
    MG_NET_WM_MOVERESIZE_SIZE_LEFT = 7,
    MG_NET_WM_MOVERESIZE_MOVE = 8
};

static void mg_xlib_send_moveresize(int32_t root_x, int32_t root_y, int32_t direction)
{
    XUngrabPointer(mg_app_state.display, CurrentTime);
    XFlush(mg_app_state.display);

    XEvent e = {0};
    e.xclient.type = ClientMessage;
    e.xclient.window = mg_app_state.window;
    e.xclient.message_type = mg_app_state.atoms.moveresize;
    e.xclient.format = 32;
    e.xclient.data.l[0] = root_x;
    e.xclient.data.l[1] = root_y;
    e.xclient.data.l[2] = direction;
    e.xclient.data.l[3] = Button1;
    e.xclient.data.l[4] = 1;

    XSendEvent(
        mg_app_state.display,
        DefaultRootWindow(mg_app_state.display),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &e);

    XFlush(mg_app_state.display);
}

static int32_t mg_xlib_hittest(int32_t x, int32_t y)
{
    const int32_t bw = 8;
    int32_t w = mg_app_state.window_width;
    int32_t h = mg_app_state.window_height;

    bool left   = x <= bw;
    bool right  = x >= w - bw;
    bool top    = y <= bw;
    bool bottom = y >= h - bw;

    if (top && left)     return MG_NET_WM_MOVERESIZE_SIZE_TOPLEFT;
    if (top && right)    return MG_NET_WM_MOVERESIZE_SIZE_TOPRIGHT;
    if (bottom && left)  return MG_NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT;
    if (bottom && right) return MG_NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;
    if (top)              return MG_NET_WM_MOVERESIZE_SIZE_TOP;
    if (bottom)           return MG_NET_WM_MOVERESIZE_SIZE_BOTTOM;
    if (left)             return MG_NET_WM_MOVERESIZE_SIZE_LEFT;
    if (right)            return MG_NET_WM_MOVERESIZE_SIZE_RIGHT;

    if (x >= mg_app_state.caption_x && x < mg_app_state.caption_x + mg_app_state.caption_width &&
        y >= mg_app_state.caption_y && y < mg_app_state.caption_y + mg_app_state.caption_height)
        return MG_NET_WM_MOVERESIZE_MOVE;

    return -1;
}

static uint32_t mg_xlib_query_dpi(Display *display, int screen)
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

    int width_px  = DisplayWidth(display, screen);
    int width_mm  = DisplayWidthMM(display, screen);
    if (width_mm > 0)
        return (uint32_t)(width_px * 25.4f / (float)width_mm + 0.5f);

    return 96;
}

int32_t mg_app_run(const mg_app_init_info *info)
{
    mg_app_state.display = XOpenDisplay(NULL);
    if (!mg_app_state.display)
    {
        fprintf(stderr, "Failed to open X display\n");
        return 1;
    }
 
    mg_app_state.screen = DefaultScreen(mg_app_state.display);
 
    const int32_t width  = info->width  ? (int32_t)info->width  : 800;
    const int32_t height = info->height ? (int32_t)info->height : 600;
 
    mg_app_state.window_width  = width;
    mg_app_state.window_height = height;
 
    Window root = RootWindow(mg_app_state.display, mg_app_state.screen);
 
    XSetWindowAttributes swa = {0};
    swa.background_pixmap = None;
    swa.bit_gravity = NorthWestGravity;
    swa.event_mask =
        StructureNotifyMask | ExposureMask |
        KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask;

    mg_app_state.window = XCreateWindow(
        mg_app_state.display,
        root,
        0, 0,
        (unsigned int)width, (unsigned int)height,
        0,
        DefaultDepth(mg_app_state.display, mg_app_state.screen),
        InputOutput,
        DefaultVisual(mg_app_state.display, mg_app_state.screen),
        CWBackPixmap | CWBitGravity | CWEventMask,
        &swa
    );
 
    if (!mg_app_state.window)
    {
        fprintf(stderr, "Failed to create X window\n");
        XCloseDisplay(mg_app_state.display);
        return 1;
    }
 
    if (info->title)
        XStoreName(mg_app_state.display, mg_app_state.window, info->title);
 
    mg_app_state.wm_delete_window = XInternAtom(mg_app_state.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(mg_app_state.display, mg_app_state.window, &mg_app_state.wm_delete_window, 1);

    mg_app_state.atoms.motif_hints = XInternAtom(mg_app_state.display, "_MOTIF_WM_HINTS", False);
    mg_app_state.atoms.moveresize  = XInternAtom(mg_app_state.display, "_NET_WM_MOVERESIZE", False);

    mg_app_state.no_titlebar = (info->flags & MG_APP_FLAG_NO_TITLEBAR) != 0;
    if (mg_app_state.no_titlebar)
        mg_xlib_set_borderless(mg_app_state.display, mg_app_state.window, mg_app_state.atoms.motif_hints, true);
 
    XMapWindow(mg_app_state.display, mg_app_state.window);
    XFlush(mg_app_state.display);

    // creating the hidden cursor
    {
        const Pixmap cursor_pixmap    = XCreatePixmap(mg_app_state.display, mg_app_state.window, 1, 1, 1);
        GC gfx_ctx = XCreateGC(mg_app_state.display, cursor_pixmap, 0, NULL);
        XDrawPoint(mg_app_state.display, cursor_pixmap, gfx_ctx, 0, 0);
        XFreeGC(mg_app_state.display, gfx_ctx);

        XColor color;
        color.flags = DoRed | DoGreen | DoBlue;
        color.red = color.blue = color.green = 0;
        mg_app_state.cursors[MG_CURSOR_HIDDEN] = XCreatePixmapCursor(mg_app_state.display, cursor_pixmap, cursor_pixmap, &color, &color, 0, 0);

        XFreePixmap(mg_app_state.display, cursor_pixmap);
    }

    mg_app_state.cursors[MG_CURSOR_ARROW] = XCreateFontCursor(mg_app_state.display, XC_left_ptr);
    mg_app_state.cursors[MG_CURSOR_IBEAM] = XCreateFontCursor(mg_app_state.display, XC_xterm);
    mg_app_state.cursors[MG_CURSOR_CROSSHAIR] = XCreateFontCursor(mg_app_state.display, XC_crosshair);
    mg_app_state.cursors[MG_CURSOR_HAND] = XCreateFontCursor(mg_app_state.display, XC_hand2);
    mg_app_state.cursors[MG_CURSOR_RESIZE_ALL] = XCreateFontCursor(mg_app_state.display, XC_fleur);
    mg_app_state.cursors[MG_CURSOR_RESIZE_NS] = XCreateFontCursor(mg_app_state.display, XC_sb_v_double_arrow);
    mg_app_state.cursors[MG_CURSOR_RESIZE_EW] = XCreateFontCursor(mg_app_state.display, XC_sb_h_double_arrow);
    mg_app_state.cursors[MG_CURSOR_RESIZE_NESW] = XCreateFontCursor(mg_app_state.display, XC_fleur);
    mg_app_state.cursors[MG_CURSOR_RESIZE_NWSE] = XCreateFontCursor(mg_app_state.display, XC_fleur);
    mg_app_state.cursors[MG_CURSOR_NOT_ALLOWED] = XCreateFontCursor(mg_app_state.display, XC_pirate);
    XDefineCursor(mg_app_state.display, mg_app_state.window, mg_app_state.cursors[MG_CURSOR_ARROW]);

    mg_app_state.on_event_call = info->events.event;
    mg_app_state.running = true;
 
    mg_app_state.time = mg_xlib_get_time();
    mg_app_state.delta_time = 0.0f;

    mg_app_state.dpi = mg_xlib_query_dpi(mg_app_state.display, mg_app_state.screen);
    mg_app_state.dpi_scale = (float)mg_app_state.dpi / 96.0f;

    mg_app_state.atoms.wm_state = XInternAtom(mg_app_state.display, "_NET_WM_STATE", False);
    mg_app_state.atoms.max_horz = XInternAtom(mg_app_state.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    mg_app_state.atoms.max_vert = XInternAtom(mg_app_state.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
 
    if (info->events.start)
        info->events.start();
 
    while (mg_app_state.running)
    {
        while (XPending(mg_app_state.display))
        {
            XEvent xev;
            XNextEvent(mg_app_state.display, &xev);
 
            switch (xev.type)
            {
                case ConfigureNotify:
                {
                    XConfigureEvent *ce = &xev.xconfigure;

                    while (XPending(mg_app_state.display))
                    {
                        XEvent next;
                        XPeekEvent(mg_app_state.display, &next);
                        if (next.type != ConfigureNotify || next.xconfigure.window != ce->window)
                            break;
                        XNextEvent(mg_app_state.display, &xev);
                        ce = &xev.xconfigure;
                    }

                    if (ce->width  != mg_app_state.window_width ||
                        ce->height != mg_app_state.window_height)
                    {
                        mg_app_state.window_width  = ce->width;
                        mg_app_state.window_height = ce->height;
                        mg_app_event event = {
                            .window_width = mg_app_state.window_width,
                            .window_height = mg_app_state.window_height,
                            .type = MG_APP_EVENT_RESIZE
                        };
                        mg_app_call_event(&event);

                        float resize_time = mg_xlib_get_time();
                        mg_app_state.delta_time = resize_time - mg_app_state.time;
                        mg_app_state.time = resize_time;

                        mg_app_input_frame();
                    }
                }
                break;

                case KeyPress:
                case KeyRelease:
                {
                    KeySym sym = XLookupKeysym(&xev.xkey, 0);
                    mg_key key = mg_xlib_translate_key(sym);

                    bool pressed = (xev.type == KeyPress);

                    mg_app_input_process_key(key, pressed);

                    switch (key)
                    {
                        case MG_KEY_LCONTROL:
                        case MG_KEY_RCONTROL:
                            mg_app_input_process_key(MG_KEY_CONTROL, pressed);
                            break;
                        case MG_KEY_LSHIFT:
                        case MG_KEY_RSHIFT:
                            mg_app_input_process_key(MG_KEY_SHIFT, pressed);
                            break;
                        case MG_KEY_LALT:
                        case MG_KEY_RALT:
                            mg_app_input_process_key(MG_KEY_ALT, pressed);
                            break;
                    }

                    mg_app_event event = {
                        .key  = key,
                        .type = (mg_app_event_type)(pressed ? MG_APP_EVENT_KEY_DOWN : MG_APP_EVENT_KEY_UP)
                    };
                    mg_app_call_event(&event);

                    if (pressed)
                    {
                        char buf[4] = {0};
                        int len = XLookupString(&xev.xkey, buf, sizeof(buf), NULL, NULL);
                        if (len == 1 && (unsigned char)buf[0] >= 32) {
                            mg_app_event event = {
                                .ch   = buf[0],
                                .type = MG_APP_EVENT_CHAR
                            };
                            mg_app_call_event(&event);
                        }
                    }
                }
                break;

                case ButtonPress:
                case ButtonRelease:
                {
                    bool pressed = (xev.type == ButtonPress);

                    if (xev.xbutton.button == Button4 || xev.xbutton.button == Button5)
                    {
                        if (pressed)
                        {
                            int8_t delta = (xev.xbutton.button == Button4) ? 1 : -1;
                            input_state.mouse.delta = delta;
                            mg_app_event event = {
                                .scroll_delta = delta,
                                .type         = MG_APP_EVENT_MOUSE_SCROLL
                            };
                            mg_app_call_event(&event);
                        }
                        break;
                    }

                    if (mg_app_state.no_titlebar && pressed && xev.xbutton.button == Button1)
                    {
                        int32_t direction = mg_xlib_hittest(xev.xbutton.x, xev.xbutton.y);
                        if (direction != -1)
                        {
                            mg_xlib_send_moveresize(xev.xbutton.x_root, xev.xbutton.y_root, direction);
                            break;
                        }
                    }

                    mg_mouse_button mb = MG_MOUSE_BUTTON_MAX;
                    switch (xev.xbutton.button)
                    {
                        case Button1: mb = MG_MOUSE_BUTTON_LEFT;   break;
                        case Button2: mb = MG_MOUSE_BUTTON_MIDDLE; break;
                        case Button3: mb = MG_MOUSE_BUTTON_RIGHT;  break;
                    }

                    if (mb != MG_MOUSE_BUTTON_MAX)
                    {
                        mg_app_input_process_mouse_button(mb, pressed, mg_app_state.time);
                        mg_app_event event = {
                            .mouse_button = mb,
                            .type         = (mg_app_event_type)(pressed ? MG_APP_EVENT_MOUSE_DOWN : MG_APP_EVENT_MOUSE_UP)
                        };
                        mg_app_call_event(&event);
                    }
                }
                break;

                case MotionNotify:
                {
                    input_state.mouse.x = xev.xmotion.x;
                    input_state.mouse.y = xev.xmotion.y;
                    mg_app_event event = {
                        .mouse_x = xev.xmotion.x,
                        .mouse_y = xev.xmotion.y,
                        .type    = MG_APP_EVENT_MOUSE_MOVE
                    };
                    mg_app_call_event(&event);
                }
                break;
 
                case ClientMessage:
                    if ((Atom)xev.xclient.data.l[0] == mg_app_state.wm_delete_window)
                        mg_app_state.running = false;
                    break;
 
                default:
                    break;
            }
        }
 
        float new_time = mg_xlib_get_time();
        mg_app_state.delta_time = new_time - mg_app_state.time;
        mg_app_state.time = new_time;

        mg_app_set_cursor(MG_CURSOR_ARROW);
 
        if (info->events.update)
            info->events.update();

        mg_app_input_frame();
    }
 
    if (info->events.end)
        info->events.end();
 
    XDestroyWindow(mg_app_state.display, mg_app_state.window);
    XCloseDisplay(mg_app_state.display);
    return 0;
}
 
void mg_app_close(void)
{
    mg_app_state.running = false;
}

void mg_app_set_cursor(mg_cursor cursor)
{
    if (mg_app_state.current_cursor == cursor)
        return;
    mg_app_state.current_cursor = cursor;
    XDefineCursor(mg_app_state.display, mg_app_state.window, mg_app_state.cursors[cursor]);
}
 
float mg_app_time(void)
{
    return mg_app_state.time;
}
 
float mg_app_delta_time(void)
{
    return mg_app_state.delta_time;
}
 
int32_t mg_app_width(void)
{
    return mg_app_state.window_width;
}
 
int32_t mg_app_height(void)
{
    return mg_app_state.window_height;
}
 
void *mg_app_primary_handle(void)
{
    return (void*)mg_app_state.window;
}

void *mg_app_secondary_handle(void)
{
    return (void*)mg_app_state.display;
}

void mg_app_show(bool value)
{
    if (value)
        XMapWindow(mg_app_state.display, mg_app_state.window);
    else
        XUnmapWindow(mg_app_state.display, mg_app_state.window);
}

void mg_app_minimize(void)
{
    XIconifyWindow(mg_app_state.display, mg_app_state.window, mg_app_state.screen);
    XFlush(mg_app_state.display);
}

void mg_app_maximize(void)
{
    XEvent e = {0};
    e.xclient.type = ClientMessage;
    e.xclient.window = mg_app_state.window;
    e.xclient.message_type = mg_app_state.atoms.wm_state;
    e.xclient.format = 32;
    e.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
    e.xclient.data.l[1] = mg_app_state.atoms.max_horz;
    e.xclient.data.l[2] = mg_app_state.atoms.max_vert;
    e.xclient.data.l[3] = 1;
    e.xclient.data.l[4] = 0;

    XSendEvent(
        mg_app_state.display,
        DefaultRootWindow(mg_app_state.display),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &e);

    XFlush(mg_app_state.display);
}

void mg_app_restore(void)
{
    XEvent e = {0};
    e.xclient.type = ClientMessage;
    e.xclient.window = mg_app_state.window;
    e.xclient.message_type = mg_app_state.atoms.wm_state;
    e.xclient.format = 32;
    e.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
    e.xclient.data.l[1] = mg_app_state.atoms.max_horz;
    e.xclient.data.l[2] = mg_app_state.atoms.max_vert;
    e.xclient.data.l[3] = 1;
    e.xclient.data.l[4] = 0;

    XSendEvent(
        mg_app_state.display,
        DefaultRootWindow(mg_app_state.display),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &e);

    XFlush(mg_app_state.display);
}

bool mg_app_maximized(void)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    if (XGetWindowProperty(mg_app_state.display, mg_app_state.window,
        mg_app_state.atoms.wm_state,
        0, 1024,
        False,
        XA_ATOM,
        &actual_type,
        &actual_format,
        &nitems,
        &bytes_after,
        &data) == Success)
    {
        mg_app_state.atoms.max_horz = XInternAtom(mg_app_state.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        mg_app_state.atoms.max_vert = XInternAtom(mg_app_state.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

        int horz = 0, vert = 0;

        Atom *atoms = (Atom*)data;
        for (unsigned long i = 0; i < nitems; i++)
        {
            if (atoms[i] == mg_app_state.atoms.max_horz)
                horz = 1;
            if (atoms[i] == mg_app_state.atoms.max_vert)
                vert = 1;
        }

        if (data)
            XFree(data);

        if (horz && vert)
            return true;
    }
    return false;
}

uint32_t mg_app_dpi(void)
{
    return mg_app_state.dpi;
}

float mg_app_dpi_scale(void)
{
    return mg_app_state.dpi_scale;
}

void mg_app_set_caption_area(int32_t x, int32_t y, int32_t width, int32_t height)
{
    mg_app_state.caption_x = x;
    mg_app_state.caption_y = y;
    mg_app_state.caption_width = width;
    mg_app_state.caption_height = height;
}
 
#endif // mg_app_state

#endif // MGAPP_IMPL
