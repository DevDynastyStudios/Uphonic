#define NAUI_BASE_DEFERRED_ARG_ARENA_SIZE (1 << 8)

typedef struct
{
    Naui_DeferredEvent event;
    void *data;
}
Naui_DeferredEntry;

typedef struct
{
    struct
    {
        Naui_AppEvent start;
        Naui_AppEvent end;
        Naui_AppEvent update;
        Naui_AppOtherEvent event;
    }
    events;
    Naui_List(Naui_DeferredEntry) deferred_entries;
    Naui_Arena deferred_arg_arena;
}
Naui_AppState;
static Naui_AppState _naui_app_state;

extern void naui_renderer_initialize(void);
extern void naui_renderer_shutdown(void);
extern void naui_renderer_resize(int32_t width, int32_t height);
extern bool naui_renderer_begin(void);
extern void naui_renderer_end(void);

extern void naui_input_update(void);
extern void naui_input_post_update(void);

void naui_themes_initialize(void);
void naui_themes_shutdown(void);

static Leaf_Dimensions measure_text_bridge(const char *text, uint32_t length, float resolved_font_size, const Leaf_TextConfig *config)
{
    Naui_Vec2 size = naui_measure_text(text, length, resolved_font_size, config->font_id);
    return (Leaf_Dimensions){size.x, size.y};
}

static void render_leaf_cmd_list(const Leaf_RenderCmdList *list)
{
    for (Leaf_RenderCmdNode *n = list->first; n; n = n->next)
    {
        Leaf_RenderCmd cmd = n->cmd;
        switch (cmd.type)
        {
            case LEAF_RENDER_CMD_RECT:
            {
                Naui_Vec2 position = (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y};
                Naui_Vec2 size = (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height};
                if (cmd.color.type == LEAF_GRADIENT_LINEAR_COLOR_FILL)
                    naui_fill_gradient_rect(position, size, (Naui_Gradient){ cmd.color.color1, cmd.color.color2, cmd.color.percent1, cmd.color.percent2, cmd.color.angle }, cmd.rect.rounding, (Naui_CornerFlags)cmd.rect.rounding_corners);
                else naui_fill_rect(position, size, cmd.color.color1, cmd.rect.rounding, (Naui_CornerFlags)cmd.rect.rounding_corners);
                break;
            }
            case LEAF_RENDER_CMD_RECT_LINES:
            {
                Naui_Vec2 position = (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y};
                Naui_Vec2 size = (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height};
                if (cmd.color.type == LEAF_GRADIENT_LINEAR_COLOR_FILL)
                    naui_draw_gradient_rect(position, size, (Naui_Gradient){ cmd.color.color1, cmd.color.color2, cmd.color.percent1, cmd.color.percent2, cmd.color.angle }, cmd.rect.line_width, cmd.rect.rounding, (Naui_CornerFlags)cmd.rect.rounding_corners, (Naui_SideFlags)cmd.rect.sides);
                else naui_draw_rect(position, size, cmd.color.color1, cmd.rect.line_width, cmd.rect.rounding, (Naui_CornerFlags)cmd.rect.rounding_corners, (Naui_SideFlags)cmd.rect.sides);
            }
            break;
            case LEAF_RENDER_CMD_IMAGE:
            {
                Naui_Image *image = (Naui_Image*)cmd.image.handle;
                Naui_Vec2 position = (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y};
                Naui_Vec2 size = (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height};
                if (cmd.color.type == LEAF_GRADIENT_LINEAR_COLOR_FILL)
                    naui_draw_gradient_image(image, position, size, (Naui_Gradient){ cmd.color.color1, cmd.color.color2, cmd.color.percent1, cmd.color.percent2, cmd.color.angle }, cmd.image.rounding, (Naui_CornerFlags)cmd.image.rounding_corners);
                else naui_draw_image(image, position, size, cmd.color.color1, cmd.image.rounding, (Naui_CornerFlags)cmd.image.rounding_corners);
                break;
            }
            case LEAF_RENDER_CMD_TEXT:
            naui_draw_text((Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y}, cmd.text.text, cmd.text.font_size, cmd.text.font_id, cmd.color.color1);
            break;
            case LEAF_RENDER_CMD_SHADOW:
            {
                Naui_Vec2 position = {cmd.bounding_box.x + cmd.shadow.offset.x, cmd.bounding_box.y + cmd.shadow.offset.y};
                Naui_Vec2 scale = {cmd.bounding_box.width, cmd.bounding_box.height};
                if ((int)cmd.shadow.offset.x != 0 || (int)cmd.shadow.offset.y != 0)
                    naui_fill_rect(position, scale, cmd.color.color1, cmd.shadow.rounding, (Naui_CornerFlags)cmd.shadow.rounding_corners);
                naui_draw_shadow(
                    position,
                    scale,
                    cmd.shadow.blur_radius,
                    cmd.color.color1,
                    cmd.shadow.rounding,
                    (Naui_CornerFlags)cmd.shadow.rounding_corners
                    );
                break;
            }
            case LEAF_RENDER_CMD_SCISSOR_PUSH:
            naui_push_clip_rect(
                cmd.bounding_box.x, cmd.bounding_box.y,
                cmd.bounding_box.width, cmd.bounding_box.height
                );
            break;
            case LEAF_RENDER_CMD_SCISSOR_POP:
            naui_pop_clip_rect();
            break;
            case LEAF_RENDER_CMD_CUSTOM:
            cmd.custom.draw(cmd.bounding_box, cmd.custom.user_data);
            break;
        }
    }
}

static void render(void)
{
    if (!naui_renderer_begin())
        return;
    leaf_begin_frame(mg_app_width(), mg_app_height());
    leaf_set_pointer_pos((float)mg_app_mouse_x(), (float)mg_app_mouse_y());
    _naui_app_state.events.update();
    Leaf_RenderCmdList cmd_list = leaf_end_frame();
    render_leaf_cmd_list(&cmd_list);
    naui_renderer_end();
}

static void __naui_app_event(const mg_app_event* event)
{
    if (event->type == MG_APP_EVENT_RESIZE)
        naui_renderer_resize(event->window_width, event->window_height);
    _naui_app_state.events.event((const Naui_AppEventData*)event);
}

static void __naui_app_start(void)
{
    naui_arena_init(naui_arena_frame(), 2 * 1024 * 1024); // 2mb should be enough for most string operations
    naui_renderer_initialize();
    naui_asset_manager_load_images(naui_path_join(naui_directory_get(NAUI_DIR_ASSETS), NAUI_PATH("Images")).data);
    naui_themes_initialize();
    leaf_init();
    leaf_set_measure_text(measure_text_bridge);
    naui_arena_init(&_naui_app_state.deferred_arg_arena, NAUI_BASE_DEFERRED_ARG_ARENA_SIZE);
    _naui_app_state.events.start();
    render();
    mg_app_show(true);
}

static void __naui_app_end(void)
{
    _naui_app_state.events.end();
    leaf_shutdown();
    naui_renderer_shutdown();
    naui_themes_shutdown();
    naui_list_free(_naui_app_state.deferred_entries);
    naui_arena_free(&_naui_app_state.deferred_arg_arena);
    naui_arena_free(naui_arena_frame());
}

static inline void naui_process_deferred(void)
{
    if (naui_list_len(_naui_app_state.deferred_entries) > 0)
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(_naui_app_state.deferred_entries); i++)
            _naui_app_state.deferred_entries[i].event(_naui_app_state.deferred_entries[i].data);
        naui_list_clear(_naui_app_state.deferred_entries);
        naui_arena_reset(&_naui_app_state.deferred_arg_arena);
    }
}

static void __naui_app_update(void)
{
    naui_arena_reset(naui_arena_frame());
    naui_process_deferred();
    naui_input_update();
	naui_shortcut_update();
    render();
}

void naui_defer(Naui_DeferredEvent event, void *data, size_t data_size)
{
    Naui_DeferredEntry entry;
    entry.event = event;
    
    if (data_size)
    {
        entry.data = naui_arena_alloc(&_naui_app_state.deferred_arg_arena, data_size);
        memcpy(entry.data, data, data_size);
    }
    else
    {
        entry.data = NULL;
    }
    
    naui_list_push(_naui_app_state.deferred_entries, entry);
}

int32_t naui_app_width(void)
{
    return mg_app_width();
}

int32_t naui_app_height(void)
{
    return mg_app_height();
}

void naui_app_close(void)
{
    mg_app_close();
}

void naui_app_minimize(void)
{
    mg_app_minimize();
}

void naui_app_maximize(void)
{
    mg_app_maximize();
}

void naui_app_restore(void)
{
    mg_app_restore();
}

bool naui_app_maximized(void)
{
    return mg_app_maximized();
}

float naui_app_dpi_scale(void)
{
    return mg_app_dpi_scale();
}

void naui_app_set_caption_area(int32_t x, int32_t y, int32_t width, int32_t height)
{
    mg_app_set_caption_area(x, y, width, height);
}

void naui_app_run(
    const char *title,
    Naui_AppEvent start,
    Naui_AppEvent end,
    Naui_AppEvent update,
    Naui_AppOtherEvent event
)
{
    _naui_app_state.events.start = start;
    _naui_app_state.events.end = end;
    _naui_app_state.events.update = update;
    _naui_app_state.events.event = event;

    mg_app_run(&(mg_app_init_info){
        .title = title,
        .flags = MG_APP_FLAG_NO_TITLEBAR | MG_APP_FLAG_HIDDEN,
        .events = {
            .start = __naui_app_start,
            .end = __naui_app_end,
            .update = __naui_app_update,
            .event = __naui_app_event
        }
    });
}