typedef uint8_t Uph_UIScrollDirection;
enum
{
    UPH_UI_SCROLL_DIRECTION_VERTICAL,
    UPH_UI_SCROLL_DIRECTION_HORIZONTAL
};

typedef struct
{
    Uph_UIScrollDirection direction;
    Leaf_ID handle_id;
    float *out_scroll;
    float container_max_size;
    float container_current_size;
    bool initialized;
}
Uph_UIScrollContainer;

Uph_UIScrollContainer uph_ui_begin_scroll_container(Uph_UIScrollDirection direction, float *out_scroll, Leaf_ID scroll_id);
void uph_ui_end_scroll_container(Uph_UIScrollContainer *container);