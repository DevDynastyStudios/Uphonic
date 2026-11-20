typedef struct
{
    Naui_Thread thread;
    Naui_Mutex lock;
    Naui_Cond wake;
    Naui_Cond finished;
#if NAUI_WINDOWS
    HANDLE wake_event;
    HANDLE finished_event;
#endif
    Naui_List(Uph_PluginCommand) queue;
    Naui_List(Uph_PluginCommand) spare;
    Naui_List(Uph_PluginInternalHandle*) live;
    uint64_t submitted;
    uint64_t completed;
    bool frame;
    bool stop;
}
Uph_PluginThreadState;

static Uph_PluginThreadState uph__plugin_thread_state;

static void uph__plugin_thread_unload(Uph_PluginInternalHandle *ih)
{
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->live); i++)
    {
        if (state->live[i] == ih)
        {
            naui_list_remove(state->live, i);
            break;
        }
    }

    switch (ih->format)
    {
        case UPH_PLUGIN_CLAP: uph_unload_clap_plugin(ih); break;
        case UPH_PLUGIN_VST3: uph_unload_vst3_plugin(ih); break;
        case UPH_PLUGIN_VST2: uph__vst2_unload(ih); break;
    }

    uph__module_release(ih->module_path);
    free(ih);
}

static void uph__plugin_thread_load(Uph_PluginLoad *load)
{
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    Uph_Plugin *plug = &load->result;

    uph__module_acquire(load->path);
    *plug = uph__plugin_load_impl(load->path);
    if (!plug->loaded)
        uph__module_release(load->path);
    else
    {
        naui_list_push(state->live, (Uph_PluginInternalHandle*)plug->internal_handle);
        if (load->state_path.data[0])
            load->state_failed = !uph__plugin_load_state(plug, load->state_path);
    }

    Uph_PluginLoadStatus expected = UPH_PLUGIN_LOAD_RUNNING;
    if (atomic_compare_exchange_strong(&load->status, &expected, UPH_PLUGIN_LOAD_DONE))
        return;

    if (plug->loaded)
        uph__plugin_thread_unload((Uph_PluginInternalHandle*)plug->internal_handle);
    naui_list_free(plug->params);
    free(load);
}

static void uph__plugin_thread_run(const Uph_PluginCommand *command)
{
    Uph_PluginInternalHandle *ih = command->handle;
    switch (command->kind)
    {
        case UPH_PLUGIN_COMMAND_LOAD: uph__plugin_thread_load(command->load); break;
        case UPH_PLUGIN_COMMAND_UNLOAD: uph__plugin_thread_unload(ih); break;
        case UPH_PLUGIN_COMMAND_SHOW: uph__plugin_gui_set_shown(ih, true); break;
        case UPH_PLUGIN_COMMAND_HIDE: uph__plugin_gui_set_shown(ih, false); break;
        case UPH_PLUGIN_COMMAND_SAVE_STATE: *command->result = uph__plugin_save_state(ih, *command->path); break;
        case UPH_PLUGIN_COMMAND_SYNC: break;
#if NAUI_LINUX
        case UPH_PLUGIN_COMMAND_MOUSE_MOVE: uph__plugin_gui_x11_send(ih, MotionNotify, 0, command->x, command->y); break;
        case UPH_PLUGIN_COMMAND_MOUSE_BUTTON: uph__plugin_gui_x11_mouse_button(ih, command->button, command->down, command->x, command->y); break;
        case UPH_PLUGIN_COMMAND_MOUSE_SCROLL: uph__plugin_gui_x11_mouse_scroll(ih, command->delta, command->x, command->y); break;
        case UPH_PLUGIN_COMMAND_KEY: uph__plugin_gui_x11_key(ih, command->key, command->down, command->modifiers); break;
#elif NAUI_WINDOWS
        case UPH_PLUGIN_COMMAND_NATIVE_INPUT: uph__plugin_gui_win32_set_passthrough(ih, !command->down); break;
        case UPH_PLUGIN_COMMAND_KEY: uph__plugin_gui_win32_key(ih, command->key, command->down); break;
        case UPH_PLUGIN_COMMAND_TEXT: uph__plugin_gui_win32_text(ih, command->codepoint); break;
#endif
    }
}

static void uph__plugin_thread_pump_messages(void)
{
#if NAUI_WINDOWS
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
#endif
}

static void uph__plugin_thread_signal_wake(Uph_PluginThreadState *state)
{
#if NAUI_WINDOWS
    SetEvent(state->wake_event);
#else
    naui_cond_signal(state->wake);
#endif
}

static void uph__plugin_thread_wait_wake(Uph_PluginThreadState *state)
{
#if NAUI_WINDOWS
    naui_mutex_unlock(state->lock);
    MsgWaitForMultipleObjectsEx(1, &state->wake_event, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    uph__plugin_thread_pump_messages();
    naui_mutex_lock(state->lock);
#else
    naui_cond_wait(state->wake, state->lock);
#endif
}

static void uph__plugin_thread_wait_finished(Uph_PluginThreadState *state)
{
#if NAUI_WINDOWS
    naui_mutex_unlock(state->lock);
    MsgWaitForMultipleObjectsEx(1, &state->finished_event, INFINITE, QS_SENDMESSAGE, 0);
    MSG msg;
    PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
    naui_mutex_lock(state->lock);
#else
    naui_cond_wait(state->finished, state->lock);
#endif
}

static int uph__plugin_thread_main(void *arg)
{
    (void)arg;
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    uph_com_init_sta_once();

    naui_mutex_lock(state->lock);
    for (;;)
    {
        while (naui_list_len(state->queue) == 0 && !state->frame && !state->stop)
            uph__plugin_thread_wait_wake(state);
        if (naui_list_len(state->queue) == 0 && state->stop)
            break;

        const bool frame = state->frame;
        state->frame = false;
        Naui_List(Uph_PluginCommand) batch = state->queue;
        state->queue = state->spare;
        state->spare = NULL;
        naui_mutex_unlock(state->lock);

        const uint32_t count = (uint32_t)naui_list_len(batch);
        for (uint32_t i = 0; i < count; i++)
            uph__plugin_thread_run(&batch[i]);
        naui_list_clear(batch);

        for (uint32_t i = 0; frame && i < (uint32_t)naui_list_len(state->live); i++)
            uph__plugin_idle(state->live[i]);
        uph__plugin_thread_pump_messages();

        naui_mutex_lock(state->lock);
        state->spare = batch;
        state->completed += count;
        naui_cond_broadcast(state->finished);
#if NAUI_WINDOWS
        SetEvent(state->finished_event);
#endif
    }
    naui_mutex_unlock(state->lock);
    return 0;
}

void uph_plugin_thread_init(void)
{
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    state->lock = naui_mutex_create();
    state->wake = naui_cond_create();
    state->finished = naui_cond_create();
#if NAUI_WINDOWS
    state->wake_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    state->finished_event = CreateEventA(NULL, FALSE, FALSE, NULL);
#endif
    state->thread = naui_thread_create(uph__plugin_thread_main, NULL);
    if (!state->thread)
        naui_log(NAUI_LOG_ERROR, "Couldn't start the plugin thread, plugins won't load");
}

void uph_plugin_thread_shutdown(void)
{
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    if (!state->thread)
        return;

    naui_mutex_lock(state->lock);
    state->stop = true;
    uph__plugin_thread_signal_wake(state);
    naui_mutex_unlock(state->lock);

    naui_thread_join(state->thread, NULL);
    state->thread = NULL;

    naui_cond_destroy(state->finished);
    naui_cond_destroy(state->wake);
    naui_mutex_destroy(state->lock);
#if NAUI_WINDOWS
    CloseHandle(state->wake_event);
    CloseHandle(state->finished_event);
#endif
    naui_list_free(state->queue);
    naui_list_free(state->spare);
    naui_list_free(state->live);
}

void uph_plugin_thread_frame(void)
{
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    naui_mutex_lock(state->lock);
    state->frame = true;
    uph__plugin_thread_signal_wake(state);
    naui_mutex_unlock(state->lock);
}

void uph_plugin_thread_push(Uph_PluginCommand command)
{
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    naui_mutex_lock(state->lock);
    naui_list_push(state->queue, command);
    state->submitted++;
    uph__plugin_thread_signal_wake(state);
    naui_mutex_unlock(state->lock);
}

void uph_plugin_thread_call(Uph_PluginCommand command)
{
    Uph_PluginThreadState *state = &uph__plugin_thread_state;
    naui_mutex_lock(state->lock);
    naui_list_push(state->queue, command);
    const uint64_t ticket = ++state->submitted;
    uph__plugin_thread_signal_wake(state);
    while (state->thread && state->completed < ticket)
        uph__plugin_thread_wait_finished(state);
    naui_mutex_unlock(state->lock);
}
