typedef uint32_t Uph_UIDragFlags;
enum
{
    UPH_UI_DRAG_FLAG_NONE = 0,
    UPH_UI_DRAG_FLAG_CLAMPED = 1 << 0,
    UPH_UI_DRAG_FLAG_FRAMELESS = 1 << 1,
    UPH_UI_DRAG_FLAG_TINTED = 1 << 2
};

typedef uint32_t Uph_UISliderFlags;
enum
{
    UPH_UI_SLIDER_FLAG_NONE = 0,
    UPH_UI_SLIDER_FLAG_VERTICAL = 1 << 0
};

typedef uint8_t Uph_UIKnobStartType;
enum
{
    UPH_UI_KNOB_START_TYPE_LEFT,
    UPH_UI_KNOB_START_TYPE_CENTER
};

typedef enum
{
    UPH_UI_EDIT_MODE_NONE,
    UPH_UI_EDIT_MODE_TEXT,
    UPH_UI_EDIT_MODE_DRAG_FLOAT,
    UPH_UI_EDIT_MODE_DRAG_INT
}
Uph_UIEditMode;

typedef struct
{
    Leaf_ID id;
    Uph_UIEditMode mode;
    Naui_String value;
    Naui_String *target_string;
    void *target_number;
    size_t cursor;
    size_t select_start;
    bool select_active;
    float scroll_x;
}
Uph_TextfieldData;

typedef struct
{
    Leaf_ID id;
    float start_value_f;
    int32_t start_value_i;
    float accum;
}
Uph_DragData;

typedef struct
{
    Leaf_ID id;
    int32_t last_mouse_y;
}
Uph_KnobData;

typedef uint8_t Uph_UIMenuCheck;
enum
{
    UPH_UI_MENU_CHECK_NONE,
    UPH_UI_MENU_CHECK_OFF,
    UPH_UI_MENU_CHECK_ON
};

typedef struct Uph_UIMenuNode Uph_UIMenuNode;
struct Uph_UIMenuNode
{
    char *text;
    char *shortcut;
    Leaf_ID element_id;
    Uph_UIMenuNode *next_sibling;
    Uph_UIMenuNode *first_child;
    Uph_UIMenuNode *last_child;
    void (*deferred_render)(Uph_UIMenuID menu_id);
    Uph_UIMenuCheck check;
};

typedef struct
{
    Naui_String *value;
    const char *placeholder;
    float scroll_x;
    size_t cursor;
    bool composing;
}
Uph_TextfieldDrawData;

typedef struct
{
    uint32_t id_counter;
    Uph_TextfieldData textfield_data;
    Uph_DragData drag_data;
    Uph_KnobData knob_data;
    Naui_Arena menu_arena;
    Uph_UIMenuNode *current_open_menu;
    uint64_t open_menu_key;
    Naui_Vec2 current_context_menu_position;
    bool is_current_menu_context;
    bool menu_opened_last_frame;
    bool menu_dropdown_hovered;
    struct
    {
        float menu_lifetime;
    }
    visual;
    bool textfield_submitted_this_frame;
    Naui_ImeMode ime_mode;
    bool any_widget_hovered;
}
Uph_GlobalWidgetState;
Uph_GlobalWidgetState uph_global_widget_state = { 0 };

#define UPH_UI_SIZE_EXACT(v) (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_FIXED, .size.min_max = { .min = (v), .max = (v) } }

static float uph__ui_frame_height(void)
{
    return NAUI_DPI(UPH_UI_FRAME_HEIGHT);
}

static Leaf_Border uph__ui_border(const Leaf_Color color)
{
    return (Leaf_Border){
        .width = (int32_t)NAUI_MAX(1.0f, NAUI_DPI(1)),
        .sides = LEAF_SIDE_ALL,
        .color = color
    };
}

static Leaf_Border uph__ui_segment_border(const Leaf_Color color, const Naui_CornerFlags corners)
{
    Leaf_Border border = uph__ui_border(color);
    border.sides = LEAF_SIDE_TOP | LEAF_SIDE_BOTTOM;

    if (corners & (NAUI_CORNER_TL | NAUI_CORNER_BL))
        border.sides |= LEAF_SIDE_LEFT;

    if (corners & (NAUI_CORNER_TR | NAUI_CORNER_BR))
        border.sides |= LEAF_SIDE_RIGHT;

    return border;
}

static Leaf_Border uph__ui_frame_border(void)
{
    return uph__ui_border(uph_state.colors.ui_frame_border);
}

bool uph_ui_widget_hovered(const Leaf_ID id)
{
    if (uph_dialog_blocks_input())
        return false;

    const bool result = !uph_ui_any_widget_hovered() && ((!naui_panel_current() && !naui_panel_is_any_hovered()) || (naui_panel_current() && naui_panel_is_hovered(naui_panel_current()))) && leaf_hovered_clipped(id);
    if (result)
        uph_global_widget_state.any_widget_hovered = true;
    return result;
}

bool uph_ui_any_widget_hovered(void)
{
    return uph_global_widget_state.any_widget_hovered;
}
