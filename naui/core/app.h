#define NAUI_DPI(x) ((x) * naui_app_dpi_scale())

typedef void (*Naui_AppEvent)(void);
typedef void (*Naui_DeferredEvent)(void*);

NAUI_API void naui_app_run(
    const char *title,
    Naui_AppEvent start,
    Naui_AppEvent end,
    Naui_AppEvent update
);

NAUI_API void       naui_defer          (Naui_DeferredEvent event, void *data, size_t data_size);

NAUI_API int32_t    naui_app_width      (void);
NAUI_API int32_t    naui_app_height     (void);

NAUI_API void       naui_app_close      (void);
NAUI_API void       naui_app_minimize   (void);
NAUI_API void       naui_app_maximize   (void);
NAUI_API void       naui_app_restore    (void);
NAUI_API bool       naui_app_maximized  (void);

NAUI_API float      naui_app_dpi_scale  (void);

NAUI_API void       naui_app_set_caption_area   (int32_t x, int32_t y, int32_t width, int32_t height);

#if defined(_WIN32) && defined(NDEBUG)
#include <windows.h>
#define _NAUI_ENTRY_POINT(title) \
     int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) { \
         (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow; \
         naui_app_run(title, naui_app_start, naui_app_end, naui_app_update); \
         return 0; \
     }
#else
#  define _NAUI_ENTRY_POINT(title) \
     int main(void) { \
         naui_app_run(title, naui_app_start, naui_app_end, naui_app_update); \
         return 0; \
     }
#endif

#define NAUI_APP(title) \
    void naui_app_start(void); \
    void naui_app_end(void); \
    void naui_app_update(void); \
    _NAUI_ENTRY_POINT(title)

