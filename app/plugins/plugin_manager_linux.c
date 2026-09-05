#if NAUI_LINUX

#define UPH_MAX_NOTE_EVENTS 128

typedef struct
{
    clap_event_note_t events[UPH_MAX_NOTE_EVENTS];
    uint32_t count;
}
Uph_ClapNoteEventQueue;

typedef struct
{
    clap_input_events_t iface;
    Uph_ClapNoteEventQueue *queue;
}
Uph_ClapInEvents;

static uint32_t uph_in_events_size(const clap_input_events_t *list)
{
    const Uph_ClapInEvents *self = (const Uph_ClapInEvents*)list;
    return self->queue->count;
}

static const clap_event_header_t *uph_in_events_get(const clap_input_events_t *list, uint32_t index)
{
    const Uph_ClapInEvents *self = (const Uph_ClapInEvents*)list;
    if (index >= self->queue->count)
        return NULL;
    return &self->queue->events[index].header;
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
            void *library_handle;

            Uph_ClapTimer timers[UPH_MAX_PLUGIN_TIMERS];
            Uph_ClapNoteEventQueue pending_notes;

            bool active_notes[128];
            int16_t active_note_channels[128];

            clap_id next_timer_id;
        }
        clap;
    };

    Window window;
    Display *display;
    Atom wm_delete_window;
    bool visible;
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
            t->last_fire = naui_time();
            t->active = true;
            *timer_id = t->id;
            return true;
        }
    }

    fprintf(stderr, "uph: no free timer slots\n");
    return false;
}

void uph_plugin_queue_note_event(
    Uph_PluginEffect *effect,
    bool note_on,
    uint8_t key,
    int16_t channel,
    uint8_t velocity,
    uint32_t sample_offset
)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;

    Uph_ClapNoteEventQueue *q = &internal_handle->clap.pending_notes;

    if (q->count >= UPH_MAX_NOTE_EVENTS)
        return;

    clap_event_note_t *ev = &q->events[q->count++];
    ev->header.size = sizeof(clap_event_note_t);
    ev->header.time = sample_offset;
    ev->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev->header.type = note_on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
    ev->header.flags = 0;

    ev->note_id = -1;
    ev->port_index = 0;
    ev->channel = channel;
    ev->key = key;
    ev->velocity = velocity / 127.0;

    internal_handle->clap.active_notes[key] = note_on;
    if (note_on)
        internal_handle->clap.active_note_channels[key] = channel;
}

void uph_plugin_queue_stop_all(Uph_PluginEffect *effect, uint32_t sample_offset)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;

    Uph_ClapNoteEventQueue *q = &internal_handle->clap.pending_notes;

    for (int key = 0; key < 128; key++)
    {
        if (!internal_handle->clap.active_notes[key])
            continue;

        if (q->count >= UPH_MAX_NOTE_EVENTS)
            break;

        clap_event_note_t *ev = &q->events[q->count++];
        ev->header.size = sizeof(clap_event_note_t);
        ev->header.time = sample_offset;
        ev->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev->header.type = CLAP_EVENT_NOTE_OFF;
        ev->header.flags = 0;

        ev->note_id = -1;
        ev->port_index = 0;
        ev->channel = internal_handle->clap.active_note_channels[key];
        ev->key = (int16_t)key;
        ev->velocity = 0.0;

        internal_handle->clap.active_notes[key] = false;
    }
}

bool uph_plugin_note_active(Uph_PluginEffect *effect, uint8_t key)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;
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

static bool uph_clap_gui_request_resize(const clap_host_t *host, uint32_t width, uint32_t height)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)host->host_data;
    if (!internal_handle)
        return false;

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

static inline void uph_load_clap_plugin_internal(Uph_PluginEffect *effect, uint32_t *width, uint32_t *height)
{
    void *handle = dlopen(effect->file_path.data, RTLD_LOCAL | RTLD_LAZY);
    if (!handle) { fprintf(stderr, "dlopen failed: %s\n", dlerror()); return; }

    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(handle, "clap_entry");
    if (!entry)
    {
        fprintf(stderr, "no clap_entry symbol\n");
        dlclose(handle);
        return;
    }

    if (!entry->init(effect->file_path.data))
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

    if (!gui->is_api_supported(plugin, CLAP_WINDOW_API_X11, false))
    {
        fprintf(stderr, "gui not supported on X11\n");
        return;
    }

    gui->create(plugin, CLAP_WINDOW_API_X11, false);
    gui->get_size(plugin, width, height);

    const clap_plugin_timer_support_t *timer_support =
        (const clap_plugin_timer_support_t *)plugin->get_extension(plugin, CLAP_EXT_TIMER_SUPPORT);

    internal_handle->clap.gui = gui;
    internal_handle->clap.plugin = plugin;
    internal_handle->clap.library_handle = handle;
    internal_handle->clap.timer_support = timer_support;
    internal_handle->clap.next_timer_id = 1;

    effect->internal_handle = internal_handle;
}

static inline void uph_assign_clap_plugin_gui_internal(Uph_PluginEffect *effect)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)effect->internal_handle;
    clap_window_t window = { .api = CLAP_WINDOW_API_X11, .x11 = internal_handle->window };
    
    const clap_plugin_t *plugin = internal_handle->clap.plugin;
    const clap_plugin_gui_t *gui = internal_handle->clap.gui;

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

Uph_PluginEffect uph_load_plugin_effect(Naui_Path path)
{
    Uph_PluginEffect effect = { 0 };

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
    XStoreName(dpy, child, "Child Window");

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

    switch (effect.type)
    {
        case UPH_PLUGIN_CLAP: uph_assign_clap_plugin_gui_internal(&effect); break;
    }

    XSelectInput(dpy, child, StructureNotifyMask | FocusChangeMask);
    effect.loaded = true;

    return effect;
}

void uph_unload_plugin_effect(Uph_PluginEffect *effect)
{
    if (!effect->loaded)
        return;

    effect->loaded = false;

    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;
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

    if (internal_handle->display)
    {
        XDestroyWindow(internal_handle->display, internal_handle->window);
        XFlush(internal_handle->display);
        XCloseDisplay(internal_handle->display);
    }

    free(effect->internal_handle);
    effect->internal_handle = NULL;
}

static inline long uph_ms_since(struct timespec *then)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - then->tv_sec) * 1000 + (now.tv_nsec - then->tv_nsec) / 1000000;
}

void uph_hide_plugin_window(Uph_PluginEffect *effect)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;

    if (!internal_handle->visible)
        return;

    if (internal_handle->clap.gui)
        internal_handle->clap.gui->hide(internal_handle->clap.plugin);

    XUnmapWindow(internal_handle->display, internal_handle->window);
    XFlush(internal_handle->display);

    internal_handle->visible = false;
}

void uph_show_plugin_window(Uph_PluginEffect *effect)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;

    if (internal_handle->visible)
        return;

    if (internal_handle->clap.gui)
        internal_handle->clap.gui->show(internal_handle->clap.plugin);
    
    XMapWindow(internal_handle->display, internal_handle->window);
    XFlush(internal_handle->display);

    internal_handle->visible = true;
}

bool uph_plugin_window_visible(Uph_PluginEffect *effect)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;
    return internal_handle->visible;
}

static inline void uph_poll_plugin_window_events(Uph_PluginEffect *effect)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;

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
                    uph_hide_plugin_window(effect);
                break;

            case DestroyNotify:
                break;

            default:
                break;
        }
    }
}

void uph_update_plugin_effect(Uph_PluginEffect *effect)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;

    uph_poll_plugin_window_events(effect);

    if (!internal_handle->visible)
        return;

    if (!internal_handle->clap.timer_support)
        return;

    float now = naui_time();
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

void uph_process_plugin_effect(
    Uph_PluginEffect *effect,
    float **inputs,
    float **outputs,
    uint32_t frame_count,
    double playhead_beat,
    bool is_playing
)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)effect->internal_handle;

    Uph_ClapInEvents in_events = {
        .iface = {
            .ctx = NULL,
            .size = uph_in_events_size,
            .get = uph_in_events_get
        },
        .queue = &internal_handle->clap.pending_notes
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

    internal_handle->clap.pending_notes.count = 0;
}

#endif