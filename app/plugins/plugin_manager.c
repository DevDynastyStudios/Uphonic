#if NAUI_LINUX || NAUI_WINDOWS

typedef struct
{
    clap_input_events_t iface;
    Uph_NoteEventRing *note_ring;
    Uph_ParamEventRing *param_ring;
    uint32_t note_count_snapshot;
    uint32_t param_count_snapshot;
}
Uph_ClapInEvents;

static uint32_t uph_in_events_size(const clap_input_events_t *list)
{
    const Uph_ClapInEvents *self = (const Uph_ClapInEvents*)list;
    return self->note_count_snapshot + self->param_count_snapshot;
}

static const clap_event_header_t *uph_in_events_get(const clap_input_events_t *list, uint32_t index)
{
    const Uph_ClapInEvents *self = (const Uph_ClapInEvents*)list;

    uint32_t note_count = self->note_count_snapshot;
    uint32_t param_count = self->param_count_snapshot;

    if (index >= note_count + param_count)
        return NULL;

    uint32_t ni = 0, pi = 0;
    for (uint32_t i = 0; i <= index; i++)
    {
        bool take_note;

        if (ni >= note_count)
            take_note = false;
        else if (pi >= param_count)
            take_note = true;
        else
            take_note = uph_note_ring_peek(self->note_ring, ni)->header.time
                <= uph_param_ring_peek(self->param_ring, pi)->header.time;

        if (i == index)
            return take_note
                ? &uph_note_ring_peek(self->note_ring, ni)->header
                : &uph_param_ring_peek(self->param_ring, pi)->header;

        if (take_note) ni++; else pi++;
    }

    return NULL;
}

#define UPH_MAX_PLUGIN_TIMERS 8

typedef struct
{
    clap_id id;
    uint32_t period_ms;
    float last_fire;
    bool active;
}
Uph_ClapTimer;

typedef struct
{
    union
    {
        struct
        {
            clap_host_t host;
            const clap_plugin_gui_t *gui;
            const clap_plugin_t *plugin;
            const clap_plugin_timer_support_t *timer_support;
            const clap_plugin_params_t *params;
            void *library_handle;

            Uph_ClapTimer timers[UPH_MAX_PLUGIN_TIMERS];
            Uph_NoteEventRing pending_notes;
            Uph_ParamEventRing pending_params;

            bool active_notes[128];
            int16_t active_note_channels[128];

            clap_id next_timer_id;
        }
        clap;
    };

#if NAUI_LINUX
    Window window;
    Display *display;
    Atom wm_delete_window;
#elif NAUI_WINDOWS
    HWND window;
#endif
    bool visible;
    Naui_String display_name; /* copy of desc->name, used as the window title */
}
Uph_PluginInternalHandle;

static bool uph_clap_timer_register(const clap_host_t *host, uint32_t period_ms, clap_id *timer_id)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)host->host_data;
    if (!internal_handle || period_ms == 0)
        return false;

    for (int i = 0; i < UPH_MAX_PLUGIN_TIMERS; i++)
    {
        Uph_ClapTimer *t = &internal_handle->clap.timers[i];
        if (!t->active)
        {
            t->id = internal_handle->clap.next_timer_id++;
            t->period_ms = period_ms;
            t->last_fire = naui_frame_time();
            t->active = true;
            *timer_id = t->id;
            return true;
        }
    }

    fprintf(stderr, "uph: no free timer slots\n");
    return false;
}

void uph_plugin_queue_note_event(
    Uph_Plugin *plug,
    bool note_on,
    uint8_t key,
    int16_t channel,
    uint8_t velocity,
    uint32_t sample_offset
)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    clap_event_note_t ev = {0};
    ev.header.size = sizeof(clap_event_note_t);
    ev.header.time = sample_offset;
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = note_on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
    ev.header.flags = 0;

    ev.note_id = -1;
    ev.port_index = 0;
    ev.channel = channel;
    ev.key = key;
    ev.velocity = velocity / 127.0;

    if (!uph_note_ring_push(&internal_handle->clap.pending_notes, &ev))
    {
        fprintf(stderr, "uph_plugin_queue_note_event: note ring full, dropping event (key=%u on=%d)\n",
                key, (int)note_on);
        return;
    }

    internal_handle->clap.active_notes[key] = note_on;
    if (note_on)
        internal_handle->clap.active_note_channels[key] = channel;
}

void uph_plugin_queue_param_change(
    Uph_Plugin *plug,
    clap_id param_id,
    double value,
    uint32_t sample_offset
)
{
    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;

    clap_event_param_value_t ev = {0};
    ev.header.size = sizeof(clap_event_param_value_t);
    ev.header.time = sample_offset;
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_PARAM_VALUE;
    ev.header.flags = 0;

    ev.param_id = param_id;
    ev.cookie = NULL;
    ev.note_id = -1;
    ev.port_index = -1;
    ev.channel = -1;
    ev.key = -1;
    ev.value = value;

    if (!uph_param_ring_push(&ih->clap.pending_params, &ev))
    {
        fprintf(stderr, "uph_plugin_queue_param_change: param ring full, dropping event (param_id=%u)\n",
                (unsigned)param_id);
    }
}

void uph_plugin_queue_stop_all(Uph_Plugin *plug, uint32_t sample_offset)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    for (int key = 0; key < 128; key++)
    {
        if (!internal_handle->clap.active_notes[key])
            continue;

        clap_event_note_t ev = {0};
        ev.header.size = sizeof(clap_event_note_t);
        ev.header.time = sample_offset;
        ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev.header.type = CLAP_EVENT_NOTE_OFF;
        ev.header.flags = 0;

        ev.note_id = -1;
        ev.port_index = 0;
        ev.channel = internal_handle->clap.active_note_channels[key];
        ev.key = (int16_t)key;
        ev.velocity = 0.0;

        if (!uph_note_ring_push(&internal_handle->clap.pending_notes, &ev))
        {
            fprintf(stderr, "uph_plugin_queue_stop_all: note ring full, dropping off for key %d\n", key);
            continue;
        }

        internal_handle->clap.active_notes[key] = false;
    }
}

bool uph_plugin_note_active(Uph_Plugin *plug, uint8_t key)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;
    return internal_handle->clap.active_notes[key];
}

static bool uph_clap_timer_unregister(const clap_host_t *host, clap_id timer_id)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)host->host_data;
    if (!internal_handle)
        return false;

    for (uint32_t i = 0; i < UPH_MAX_PLUGIN_TIMERS; i++)
    {
        if (internal_handle->clap.timers[i].active && internal_handle->clap.timers[i].id == timer_id)
        {
            internal_handle->clap.timers[i].active = false;
            return true;
        }
    }
    return false;
}

static const clap_host_timer_support_t uph_clap_host_timer_support = {
    .register_timer = uph_clap_timer_register,
    .unregister_timer = uph_clap_timer_unregister,
};

static void uph_clap_params_rescan(const clap_host_t *host, clap_param_rescan_flags flags) { (void)host; (void)flags; }
static void uph_clap_params_clear(const clap_host_t *host, clap_id param_id, clap_param_clear_flags flags) { (void)host; (void)param_id; (void)flags; }
static void uph_clap_params_request_flush(const clap_host_t *host) { (void)host; }

static const clap_host_params_t uph_clap_host_params = {
    .rescan = uph_clap_params_rescan,
    .clear = uph_clap_params_clear,
    .request_flush = uph_clap_params_request_flush,
};

static bool uph_clap_gui_request_resize(const clap_host_t *host, uint32_t width, uint32_t height)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)host->host_data;
    if (!internal_handle)
        return false;

#if NAUI_LINUX
    XResizeWindow(internal_handle->display, internal_handle->window, width, height);

    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PMinSize | PMaxSize;
    size_hints->min_width = width;
    size_hints->max_width = width;
    size_hints->min_height = height;
    size_hints->max_height = height;
    XSetWMNormalHints(internal_handle->display, internal_handle->window, size_hints);
    XFree(size_hints);

    XFlush(internal_handle->display);
#elif NAUI_WINDOWS
    /* width/height from CLAP are client-area size; adjust so the client
       area ends up matching exactly, same intent as the X11 min/max hints. */
    RECT rect = { 0, 0, (LONG)width, (LONG)height };
    DWORD style = (DWORD)GetWindowLongA(internal_handle->window, GWL_STYLE);
    DWORD ex_style = (DWORD)GetWindowLongA(internal_handle->window, GWL_EXSTYLE);
    AdjustWindowRectEx(&rect, style, FALSE, ex_style);

    SetWindowPos(
        internal_handle->window,
        NULL,
        0, 0,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
    );
#endif

    return true;
}

static bool uph_clap_gui_request_show(const clap_host_t *host)
{
    (void)host;
    return true;
}

static bool uph_clap_gui_request_hide(const clap_host_t *host)
{
    (void)host;
    return true;
}

static void uph_clap_gui_resize_hints_changed(const clap_host_t *host)
{
    (void)host;
}

static const clap_host_gui_t uph_clap_host_gui = {
    .resize_hints_changed = uph_clap_gui_resize_hints_changed,
    .request_resize = uph_clap_gui_request_resize,
    .request_show = uph_clap_gui_request_show,
    .request_hide = uph_clap_gui_request_hide,
};

static const void *uph_clap_get_extension(const clap_host_t *host, const char *extension_id)
{
    (void)host;
    if (strcmp(extension_id, CLAP_EXT_TIMER_SUPPORT) == 0)
        return &uph_clap_host_timer_support;
    if (strcmp(extension_id, CLAP_EXT_GUI) == 0)
        return &uph_clap_host_gui;
    return NULL;
}

static void uph_clap_request_restart(const clap_host_t *host)
{
    (void)host;
}

static void uph_clap_request_process(const clap_host_t *host)
{
    (void)host;
}

static void uph_clap_request_callback(const clap_host_t *host)
{
    (void)host;
}

static Naui_List(Uph_PluginParam) uph_clap_get_param_list(Uph_Plugin *plug)
{
    Naui_List(Uph_PluginParam) list = NULL;

    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    if (!internal_handle || !internal_handle->clap.params)
        return list;

    const clap_plugin_params_t *params = internal_handle->clap.params;
    const clap_plugin_t *plugin = internal_handle->clap.plugin;

    uint32_t count = params->count(plugin);
    naui_list_reserve(list, count);

    for (uint32_t i = 0; i < count; i++)
    {
        clap_param_info_t info;
        if (!params->get_info(plugin, i, &info))
            continue;

        double current_value = info.default_value;
        params->get_value(plugin, info.id, &current_value);

        Uph_PluginParam p = {
            .id = info.id,
            .name = naui_string_from_cstr(info.name),
            .module = naui_string_from_cstr(info.module),
            .min_value = info.min_value,
            .max_value = info.max_value,
            .default_value = info.default_value,
            .current_value = current_value
        };

        naui_list_push(list, p);
    }

    return list;
}

#if NAUI_WINDOWS
static void uph_hide_plugin_window_internal(Uph_PluginInternalHandle *internal_handle)
{
    if (!internal_handle->visible)
        return;

    internal_handle->visible = false;

    if (internal_handle->clap.gui)
        internal_handle->clap.gui->hide(internal_handle->clap.plugin);

    ShowWindow(internal_handle->window, SW_HIDE);
}

static LRESULT CALLBACK uph_plugin_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)(uintptr_t)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg)
    {
        case WM_CLOSE:
            /* internal_handle is only attached after CreateWindowExA returns
               (see uph_load_plugin_effect), so it can still be NULL here if
               something manages to close the window before then. */
            if (internal_handle)
                uph_hide_plugin_window_internal(internal_handle);
            return 0;

        default:
            return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
}

static ATOM uph_register_plugin_wnd_class(void)
{
    static ATOM cls = 0;
    if (cls)
        return cls;

    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = uph_plugin_wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "UphPluginChildWindow";

    cls = RegisterClassExA(&wc);
    if (!cls)
        fprintf(stderr, "uph: RegisterClassExA failed: %lu\n", GetLastError());

    return cls;
}
#endif

static inline void uph_load_clap_plugin_internal(Uph_Plugin *plug, uint32_t *width, uint32_t *height)
{
#if NAUI_LINUX
    void *handle = dlopen(plug->file_path.data, RTLD_LOCAL | RTLD_LAZY);
    if (!handle) { fprintf(stderr, "dlopen failed: %s\n", dlerror()); return; }

    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(handle, "clap_entry");
    if (!entry)
    {
        fprintf(stderr, "no clap_entry symbol\n");
        dlclose(handle);
        return;
    }
#elif NAUI_WINDOWS
    HMODULE handle = LoadLibraryA(plug->file_path.data);
    if (!handle) { fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError()); return; }

    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)GetProcAddress(handle, "clap_entry");
    if (!entry)
    {
        fprintf(stderr, "no clap_entry symbol\n");
        FreeLibrary(handle);
        return;
    }
#endif

    if (!entry->init(plug->file_path.data))
    {
        fprintf(stderr, "entry->init failed\n");
        return;
    }

    const clap_plugin_factory_t *factory =
        (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

    uint32_t count = factory->get_plugin_count(factory);
    if (count == 0)
    {
        fprintf(stderr, "no plugins in this bundle\n");
        return;
    }

    const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
    printf("loading plugin: %s (%s)\n", desc->name, desc->id);

    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)calloc(1, sizeof(Uph_PluginInternalHandle));

    /* desc->name is owned by the plugin bundle; copy it so we have a
       stable string to use as the window title after this call returns. */
    internal_handle->display_name = naui_string_from_cstr(desc->name);

    uph_note_ring_init(&internal_handle->clap.pending_notes);
    uph_param_ring_init(&internal_handle->clap.pending_params);

    internal_handle->clap.host = (clap_host_t){
        .clap_version = CLAP_VERSION_INIT,
        .host_data = internal_handle,
        .name = "Uphonic",
        .vendor = "Dev Dynasty Studios",
        .version = "0.1.0",
        .get_extension = uph_clap_get_extension,
        .request_restart = uph_clap_request_restart,
        .request_process = uph_clap_request_process,
        .request_callback = uph_clap_request_callback
    };

    const clap_plugin_t *plugin = factory->create_plugin(factory, &internal_handle->clap.host, desc->id);
    if (!plugin)
    {
        fprintf(stderr, "create_plugin failed\n");
        free(internal_handle);
        return;
    }
    if (!plugin->init(plugin))
    {
        fprintf(stderr, "plugin init failed\n");
        return;
    }

    const clap_plugin_gui_t *gui =
        (const clap_plugin_gui_t *)plugin->get_extension(plugin, CLAP_EXT_GUI);

    if (!gui)
    {
        fprintf(stderr, "gui init failed\n");
        return;
    }

#if NAUI_LINUX
    if (!gui->is_api_supported(plugin, CLAP_WINDOW_API_X11, false))
    {
        fprintf(stderr, "gui not supported on X11\n");
        return;
    }
    gui->create(plugin, CLAP_WINDOW_API_X11, false);
#elif NAUI_WINDOWS
    if (!gui->is_api_supported(plugin, CLAP_WINDOW_API_WIN32, false))
    {
        fprintf(stderr, "gui not supported on Win32\n");
        return;
    }
    gui->create(plugin, CLAP_WINDOW_API_WIN32, false);
#endif

    gui->get_size(plugin, width, height);

    const clap_plugin_timer_support_t *timer_support =
        (const clap_plugin_timer_support_t *)plugin->get_extension(plugin, CLAP_EXT_TIMER_SUPPORT);

    const clap_plugin_params_t *params =
        (const clap_plugin_params_t *)plugin->get_extension(plugin, CLAP_EXT_PARAMS);

    internal_handle->clap.gui = gui;
    internal_handle->clap.plugin = plugin;
    internal_handle->clap.library_handle = handle;
    internal_handle->clap.timer_support = timer_support;
    internal_handle->clap.next_timer_id = 1;
    internal_handle->clap.params = params;

    plug->internal_handle = internal_handle;
    plug->params = uph_clap_get_param_list(plug);
}

static inline void uph_assign_clap_plugin_gui_internal(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;

    const clap_plugin_t *plugin = internal_handle->clap.plugin;
    const clap_plugin_gui_t *gui = internal_handle->clap.gui;

#if NAUI_LINUX
    clap_window_t window = { .api = CLAP_WINDOW_API_X11, .x11 = internal_handle->window };
#elif NAUI_WINDOWS
    clap_window_t window = { .api = CLAP_WINDOW_API_WIN32, .win32 = internal_handle->window };
#endif

    gui->set_parent(plugin, &window);
    gui->show(plugin);

    plugin->activate(
        plugin,
        uph_state.settings.audio.sample_rate,
        1,
        UPH_SAMPLE_FRAME_COUNT
    );

    plugin->start_processing(plugin);
}

Uph_Plugin uph_load_plugin_effect(Naui_Path path)
{
    Uph_Plugin effect = { 0 };

    const Naui_StringView extension = naui_file_extension(&path);
    if (naui_string_view_equals_cstr(extension, ".clap", false))
        effect.type = UPH_PLUGIN_CLAP;

    effect.file_path = path;

    uint32_t width, height;
    switch (effect.type)
    {
        case UPH_PLUGIN_CLAP: uph_load_clap_plugin_internal(&effect, &width, &height); break;
    }

    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)effect.internal_handle;

#if NAUI_LINUX
    Window parent = (Window)mg_app_primary_handle();
    Display *dpy = (Display*)XOpenDisplay(NULL);

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    int x = 100, y = 100;
    unsigned int border_width = 0;
    unsigned long border_color = BlackPixel(dpy, screen);
    unsigned long bg_color = WhitePixel(dpy, screen);

    Window child = XCreateSimpleWindow(
        dpy, root,
        x, y, width, height,
        border_width, border_color, bg_color
    );

    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PMinSize | PMaxSize;
    size_hints->min_width = width;
    size_hints->max_width = width;
    size_hints->min_height = height;
    size_hints->max_height = height;

    XSetWMNormalHints(dpy, child, size_hints);
    XFree(size_hints);

    XSetTransientForHint(dpy, child, parent);
    XStoreName(dpy, child, internal_handle->display_name.data);

    XMapWindow(dpy, child);
    XFlush(dpy);

    Atom wm_window_type = XInternAtom(
        dpy,
        "_NET_WM_WINDOW_TYPE",
        False
    );

    Atom wm_window_type_dialog = XInternAtom(
        dpy,
        "_NET_WM_WINDOW_TYPE_DIALOG",
        False
    );

    XChangeProperty(
        dpy,
        child,
        wm_window_type,
        XA_ATOM,
        32,
        PropModeReplace,
        (unsigned char *)&wm_window_type_dialog,
        1
    );

    internal_handle->wm_delete_window = XInternAtom(
        dpy,
        "WM_DELETE_WINDOW",
        False
    );

    XSetWMProtocols(
        dpy,
        child,
        &internal_handle->wm_delete_window,
        1
    );

    internal_handle->window = child;
    internal_handle->display = dpy;
    internal_handle->visible = true;
#elif NAUI_WINDOWS
    HWND parent = (HWND)mg_app_primary_handle();

    uph_register_plugin_wnd_class();

    /* WS_POPUP + WS_CAPTION/WS_SYSMENU gives a normal-looking dialog-ish
       frame without WS_THICKFRAME, so the CLAP plugin's fixed size is
       respected (mirrors the X11 PMinSize|PMaxSize hint behavior). */
    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    DWORD ex_style = WS_EX_DLGMODALFRAME;

    RECT rect = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRectEx(&rect, style, FALSE, ex_style);

    HWND child = CreateWindowExA(
        ex_style,
        "UphPluginChildWindow",
        internal_handle->display_name.data,
        style,
        100, 100,
        rect.right - rect.left,
        rect.bottom - rect.top,
        parent,
        NULL,
        GetModuleHandleA(NULL),
        NULL
    );

    if (!child)
    {
        fprintf(stderr, "uph: CreateWindowExA failed: %lu\n", GetLastError());
        return effect;
    }

    /* Store the heap-owned internal handle here, not a pointer to the local
       `effect` — `effect` is returned by value below and its address is
       dead the moment this function returns, which was the actual bug
       behind the close button silently doing nothing. */
    SetWindowLongPtrA(child, GWLP_USERDATA, (LONG_PTR)internal_handle);

    ShowWindow(child, SW_SHOW);
    UpdateWindow(child);

    internal_handle->window = child;
    internal_handle->visible = true;
#endif

    switch (effect.type)
    {
        case UPH_PLUGIN_CLAP: uph_assign_clap_plugin_gui_internal(&effect); break;
    }

    effect.loaded = true;

    return effect;
}

void uph_unload_plugin_effect(Uph_Plugin *plug)
{
    if (!plug->loaded)
        return;

    plug->loaded = false;

    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!internal_handle)
        return;

    const clap_plugin_t *plugin = internal_handle->clap.plugin;
    if (plugin)
    {
        plugin->stop_processing(plugin);
        plugin->deactivate(plugin);

        if (internal_handle->clap.gui)
            internal_handle->clap.gui->destroy(plugin);

        plugin->destroy(plugin);
    }

#if NAUI_LINUX
    if (internal_handle->display)
    {
        XDestroyWindow(internal_handle->display, internal_handle->window);
        XFlush(internal_handle->display);
        XCloseDisplay(internal_handle->display);
    }
#elif NAUI_WINDOWS
    if (internal_handle->window)
        DestroyWindow(internal_handle->window);

    if (internal_handle->clap.library_handle)
        FreeLibrary((HMODULE)internal_handle->clap.library_handle);
#endif

    free(plug->internal_handle);
    plug->internal_handle = NULL;
}

static inline long uph_ms_since(float then)
{
    return (long)((naui_time() - then) * 1000.0f);
}

void uph_hide_plugin_window(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

#if NAUI_LINUX
    if (!internal_handle->visible)
        return;

    internal_handle->visible = false;

    if (internal_handle->clap.gui)
        internal_handle->clap.gui->hide(internal_handle->clap.plugin);

    XUnmapWindow(internal_handle->display, internal_handle->window);
    XFlush(internal_handle->display);
#elif NAUI_WINDOWS
    uph_hide_plugin_window_internal(internal_handle);
#endif
}

void uph_show_plugin_window(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    if (internal_handle->visible)
        return;

#if NAUI_LINUX
    XMapWindow(internal_handle->display, internal_handle->window);
    XFlush(internal_handle->display);
#elif NAUI_WINDOWS
    ShowWindow(internal_handle->window, SW_SHOW);
    UpdateWindow(internal_handle->window);
#endif

    if (internal_handle->clap.gui)
        internal_handle->clap.gui->show(internal_handle->clap.plugin);

    internal_handle->visible = true;
}

bool uph_plugin_window_visible(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;
    return internal_handle->visible;
}

static inline void uph_poll_plugin_window_events(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

#if NAUI_LINUX
    Display *dpy = internal_handle->display;
    Window win = internal_handle->window;

    while (XPending(dpy) > 0)
    {
        XEvent event;
        XPeekEvent(dpy, &event);

        if (event.xany.window != win)
            break;

        XNextEvent(dpy, &event);

        switch (event.type)
        {
            case ConfigureNotify:
                break;

            case FocusIn:
            case FocusOut:
                break;

            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == internal_handle->wm_delete_window)
                    uph_hide_plugin_window(plug);
                break;

            case DestroyNotify:
                break;

            default:
                break;
        }
    }
#elif NAUI_WINDOWS
    MSG msg;
    HWND win = internal_handle->window;

    /* PM_REMOVE + filtering by hwnd keeps this scoped to just the plugin
       child window, mirroring the X11 "peek then bail if not ours" loop. */
    while (PeekMessageA(&msg, win, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
#endif
}

void uph_update_plugin(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    if (!internal_handle->visible)
        return;

    uph_poll_plugin_window_events(plug);

    if (!internal_handle->clap.timer_support)
        return;

    float now = naui_frame_time();
    for (int i = 0; i < UPH_MAX_PLUGIN_TIMERS; i++)
    {
        Uph_ClapTimer *t = &internal_handle->clap.timers[i];
        if (!t->active)
            continue;

        float period_s = t->period_ms / 1000.0f;
        if (now - t->last_fire >= period_s)
        {
            t->last_fire = now;
            internal_handle->clap.timer_support->on_timer(internal_handle->clap.plugin, t->id);
        }
    }
}

static bool uph_out_events_try_push(const clap_output_events_t *list, const clap_event_header_t *event)
{
    return true;
}

void uph_process_plugin(
    Uph_Plugin *plug,
    float **inputs,
    float **outputs,
    uint32_t frame_count,
    double playhead_beat,
    bool is_playing
)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    uint32_t note_snapshot  = uph_note_ring_size(&internal_handle->clap.pending_notes);
    uint32_t param_snapshot = uph_param_ring_size(&internal_handle->clap.pending_params);

    Uph_ClapInEvents in_events = {
        .iface = {
            .ctx = NULL,
            .size = uph_in_events_size,
            .get = uph_in_events_get
        },
        .note_ring = &internal_handle->clap.pending_notes,
        .param_ring = &internal_handle->clap.pending_params,
        .note_count_snapshot = note_snapshot,
        .param_count_snapshot = param_snapshot,
    };

    clap_output_events_t out_iface = {
        .try_push = uph_out_events_try_push
    };

    clap_audio_buffer_t in = {
        .data32 = inputs,
        .data64 = NULL,
        .channel_count = 2,
        .latency = 0,
        .constant_mask = 0
    };

    clap_audio_buffer_t out = {
        .data32 = outputs,
        .data64 = NULL,
        .channel_count = 2,
        .latency = 0,
        .constant_mask = 0
    };

    clap_beattime song_pos_beats = (clap_beattime)(playhead_beat * CLAP_BEATTIME_FACTOR);

    clap_event_transport_t transport = {
        .header = {
            .size = sizeof(clap_event_transport_t),
            .time = 0,
            .space_id = CLAP_CORE_EVENT_SPACE_ID,
            .type = CLAP_EVENT_TRANSPORT,
            .flags = 0
        },
        .flags = CLAP_TRANSPORT_HAS_TEMPO
            | CLAP_TRANSPORT_HAS_BEATS_TIMELINE
            | (is_playing ? CLAP_TRANSPORT_IS_PLAYING : 0),
        .song_pos_beats = song_pos_beats,
        .song_pos_seconds = 0,
        .tempo = uph_state.project.bpm,
        .tempo_inc = 0,
        .loop_start_beats = 0,
        .loop_end_beats = 0,
        .loop_start_seconds = 0,
        .loop_end_seconds = 0,
        .bar_start = 0,
        .bar_number = 0,
        .tsig_num = 4,
        .tsig_denom = 4
    };

    clap_process_t process = {
        .steady_time = -1,
        .frames_count = frame_count,

        .transport = &transport,

        .audio_inputs = &in,
        .audio_outputs = &out,

        .audio_inputs_count = 1,
        .audio_outputs_count = 1,

        .in_events = &in_events.iface,
        .out_events = &out_iface
    };

    const clap_plugin_t *plugin = internal_handle->clap.plugin;
    plugin->process(plugin, &process);

    uph_note_ring_advance(&internal_handle->clap.pending_notes, note_snapshot);
    uph_param_ring_advance(&internal_handle->clap.pending_params, param_snapshot);
}

#endif