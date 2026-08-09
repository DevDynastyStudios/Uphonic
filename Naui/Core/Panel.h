#ifndef NAUI_PANEL_BORDER_COLOR_TAG
    #define NAUI_PANEL_BORDER_COLOR_TAG "naui_panel_border_color"
#endif

#ifndef NAUI_PANEL_BORDER_WIDTH_TAG
    #define NAUI_PANEL_BORDER_WIDTH_TAG "naui_panel_border_width"
#endif

#ifndef NAUI_PANEL_TITLEBAR_BG_COLOR_TAG
    #define NAUI_PANEL_TITLEBAR_BG_COLOR_TAG "naui_panel_title_bg_color"
#endif

#ifndef NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG
    #define NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG "naui_panel_title_text_color"
#endif

#ifndef NAUI_PANEL_TITLEBAR_PADDING_TAG
    #define NAUI_PANEL_TITLEBAR_PADDING_TAG "naui_panel_title_padding"
#endif

#ifndef NAUI_PANEL_BODY_BG_COLOR_TAG
    #define NAUI_PANEL_BODY_BG_COLOR_TAG "naui_panel_body_bg_color"
#endif

#ifndef NAUI_PANEL_BODY_PADDING_TAG
    #define NAUI_PANEL_BODY_PADDING_TAG "naui_panel_body_padding"
#endif

#ifndef NAUI_PANEL_ROUNDING_TAG
    #define NAUI_PANEL_ROUNDING_TAG "naui_panel_rounding"
#endif

#ifndef NAUI_PANEL_SHADOW_COLOR_TAG
    #define NAUI_PANEL_SHADOW_COLOR_TAG "naui_panel_shadow_color"
#endif

#ifndef NAUI_PANEL_INNER_SHADOW_COLOR_TAG
    #define NAUI_PANEL_INNER_SHADOW_COLOR_TAG "naui_panel_inner_shadow_color"
#endif

#ifndef NAUI_PANEL_FONT_SIZE_TAG
    #define NAUI_PANEL_FONT_SIZE_TAG "naui_panel_font_size"
#endif

#ifndef NAUI_PANEL_BUTTON_HOVERED_BG_COLOR_TAG
    #define NAUI_PANEL_BUTTON_HOVERED_BG_COLOR_TAG "naui_panel_button_hovered_bg_color"
#endif

#ifndef NAUI_PANEL_CLOSE_HOVERED_BG_COLOR_TAG
    #define NAUI_PANEL_CLOSE_HOVERED_BG_COLOR_TAG "naui_panel_close_hovered_bg_color"
#endif

#ifndef NAUI_VIEWPORT_BG_COLOR_TAG
    #define NAUI_VIEWPORT_BG_COLOR_TAG "naui_viewport_bg_color"
#endif

#ifndef NAUI_DOCK_GUIDE_COLOR_TAG
    #define NAUI_DOCK_GUIDE_COLOR_TAG "naui_dock_guide_color"
#endif

#ifndef NAUI_DOCK_GUIDE_HOVERED_COLOR_TAG
    #define NAUI_DOCK_GUIDE_HOVERED_COLOR_TAG "naui_dock_guide_hovered_color"
#endif

#ifndef NAUI_DOCK_GUIDE_OUTLINE_COLOR_TAG
    #define NAUI_DOCK_GUIDE_OUTLINE_COLOR_TAG "naui_dock_guide_outline_color"
#endif

#ifndef NAUI_MINIMIZE_ICON_TAG
    #define NAUI_MINIMIZE_ICON_TAG "naui_icon_minimize"
#endif

#ifndef NAUI_MAXIMIZE_ICON_TAG
    #define NAUI_MAXIMIZE_ICON_TAG "naui_icon_maximize"
#endif

#ifndef NAUI_CLOSE_ICON_TAG
    #define NAUI_CLOSE_ICON_TAG "naui_icon_close"
#endif

typedef uint64_t Naui_PanelID;
typedef void(*NauiPanelEvent)(void *data);

typedef uint8_t Naui_DockDirection;
enum
{
    NAUI_DOCK_DIRECTION_LEFT,
    NAUI_DOCK_DIRECTION_RIGHT,
    NAUI_DOCK_DIRECTION_TOP,
    NAUI_DOCK_DIRECTION_BOTTOM,
    NAUI_DOCK_DIRECTION_CENTER
};

typedef struct
{
    NauiPanelEvent on_attach;
    NauiPanelEvent on_detach;
    NauiPanelEvent on_update;
    size_t user_data_size;
    const char *type_name;
}
Naui_PanelType;

typedef uint32_t Naui_PanelFlags;
enum
{
    NAUI_PANEL_FLAG_NONE = 0,
    NAUI_PANEL_FLAG_NO_CLOSE = 1 << 0,
    NAUI_PANEL_FLAG_NO_DOCK_TO_OTHER = 1 << 1,
    NAUI_PANEL_FLAG_NO_DOCK_FROM_OTHER = 1 << 2,
    NAUI_PANEL_FLAG_NO_DOCK = NAUI_PANEL_FLAG_NO_DOCK_TO_OTHER | NAUI_PANEL_FLAG_NO_DOCK_FROM_OTHER,
    NAUI_PANEL_FLAG_NO_UNDOCK = 1 << 3,
    NAUI_PANEL_FLAG_NO_MOVE = 1 << 4,
    NAUI_PANEL_FLAG_NO_RESIZE = 1 << 5,
    NAUI_PANEL_FLAG_NO_TITLE = 1 << 6,
    NAUI_PANEL_FLAG_ALWAYS_TO_FRONT = 1 << 7, // still not implemented
    NAUI_PANEL_FLAG_SERIALIZABLE = 1 << 8
};

#define NAUI_ATTACH_PANEL(type_name) naui_attach_panel(#type_name)
#define NAUI_FIND_PANEL_OF_TYPE(type_name) naui_find_panel_of_type(#type_name)

NAUI_API Naui_PanelID       naui_attach_panel               (const char *type_name);
NAUI_API void               naui_detach_panel               (Naui_PanelID id);
NAUI_API void               naui_register_panel_type        (const char *name, Naui_PanelType type);

NAUI_API void               naui_panel_set_title            (Naui_PanelID panel_id, const char *title);
NAUI_API void               naui_panel_set_size             (Naui_PanelID panel_id, Naui_Vec2 size);
NAUI_API void               naui_panel_set_min_size         (Naui_PanelID panel_id, Naui_Vec2 size);
NAUI_API void               naui_panel_enable_flags         (Naui_PanelID panel_id, Naui_PanelFlags flags);
NAUI_API void               naui_panel_disable_flags        (Naui_PanelID panel_id, Naui_PanelFlags flags);

NAUI_API Naui_PanelID       naui_dock_panel                 (Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio);
NAUI_API void               naui_undock_panel               (Naui_PanelID id);

NAUI_API void               naui_set_main_viewport          (Naui_PanelID id);
NAUI_API Naui_PanelID       naui_get_main_viewport          (void);

NAUI_API bool               naui_panel_hovered              (Naui_PanelID id);
NAUI_API bool               naui_any_panel_hovered          (void);

NAUI_API Naui_PanelID       naui_find_panel_of_type         (const char *type_name);

NAUI_API void               naui_render_panels_and_viewport (void);
NAUI_API Naui_PanelID       naui_current_panel              (void);

NAUI_API bool               naui_serialize_viewport         (const char *file_path);
NAUI_API bool               naui_deserialize_viewport       (const char *file_path);

#ifdef _MSC_VER
  #pragma section(".CRT$XCU", read)
  #define NAUI_CONSTRUCTOR_NAMED(fn) \
      static void fn(void); \
      __declspec(allocate(".CRT$XCU")) void(*fn##_)(void) = fn; \
      static void fn(void)
#else
  #define NAUI_CONSTRUCTOR_NAMED(fn) \
      __attribute__((constructor)) static void fn(void)
#endif

#define __NAUI_DEFINE_PANEL_TYPE(name, data_size) \
    static void name##_on_attach(void); \
    static void name##_on_detach(void); \
    static void name##_on_update(void); \
    static Naui_PanelType _##name##_events = { \
        (NauiPanelEvent)name##_on_attach, \
        (NauiPanelEvent)name##_on_detach, \
        (NauiPanelEvent)name##_on_update, \
        data_size, \
        #name \
    }; \
    NAUI_CONSTRUCTOR_NAMED(_register_##name) { \
        naui_register_panel_type(#name, _##name##_events); \
    }

#define NAUI_PANEL_WITH_DATA(name, data_type) __NAUI_DEFINE_PANEL_TYPE(name, sizeof(data_type))
#define NAUI_PANEL(name) __NAUI_DEFINE_PANEL_TYPE(name, 0)
