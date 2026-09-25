typedef struct
{
    clap_input_events_t iface;
    Uph_NoteEventRing *note_ring;
    Uph_ParamEventRing *param_ring;
    uint32_t note_count_snapshot;
    uint32_t param_count_snapshot;
}
Uph_ClapInEvents;

static uint32_t uph_clap_in_events_size(const clap_input_events_t *list)
{
    const Uph_ClapInEvents *self = (const Uph_ClapInEvents*)list;
    return self->note_count_snapshot + self->param_count_snapshot;
}

static const clap_event_header_t *uph_clap_in_events_get(const clap_input_events_t *list, uint32_t index)
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

static bool uph_vst3_iid_equal(const Steinberg_TUID a, const Steinberg_TUID b)
{
    return memcmp(a, b, sizeof(Steinberg_TUID)) == 0;
}

#if NAUI_WINDOWS
typedef HRESULT (WINAPI *Uph_OleInitializeFn)(LPVOID);

static void uph_com_init_sta_once(void)
{
    static _Thread_local bool done = false;
    if (done)
        return;
    done = true;

    HMODULE ole32 = LoadLibraryA("ole32.dll");
    if (!ole32)
    {
        fprintf(stderr, "uph: could not load ole32.dll\n");
        return;
    }

    Uph_OleInitializeFn ole_init = (Uph_OleInitializeFn)(void*)GetProcAddress(ole32, "OleInitialize");
    if (!ole_init)
    {
        fprintf(stderr, "uph: OleInitialize not found\n");
        return;
    }

    HRESULT hr = ole_init(NULL);
    if (hr == RPC_E_CHANGED_MODE)
        fprintf(stderr, "uph: thread is already MTA, plugin GUIs that need STA may misbehave\n");
    else if (FAILED(hr))
        fprintf(stderr, "uph: OleInitialize failed: 0x%08lx\n", (unsigned long)hr);
}
#else
static void uph_com_init_sta_once(void) {}
#endif

typedef struct
{
    Steinberg_Vst_IHostApplication iface;
}
Uph_Vst3Host;

typedef struct
{
    Steinberg_Vst_IComponentHandler iface;
    struct Uph_PluginInternalHandle_ *owner;
}
Uph_Vst3Handler;

#define UPH_VST3_MAX_FD_HANDLERS 8
#define UPH_VST3_MAX_TIMERS      8

typedef struct
{
    Steinberg_Linux_IEventHandler *handler;
    int fd;
}
Uph_Vst3FdHandler;

typedef struct
{
    Steinberg_Linux_ITimerHandler *handler;
    uint64_t period_ms;
    float last_fire;
}
Uph_Vst3Timer;

typedef struct
{
    Steinberg_Linux_IRunLoop iface;
    Uph_Vst3FdHandler fds[UPH_VST3_MAX_FD_HANDLERS];
    Uph_Vst3Timer timers[UPH_VST3_MAX_TIMERS];
}
Uph_Vst3RunLoop;

typedef struct
{
    Steinberg_IPlugFrame iface;
    Uph_Vst3RunLoop *run_loop;
    struct Uph_PluginInternalHandle_ *owner;
}
Uph_Vst3Frame;

#define UPH_VST3_MAX_EVENTS (UPH_NOTE_RING_CAPACITY)
#define UPH_VST3_MAX_PARAM_QUEUES 64
#define UPH_VST3_MAX_POINTS_PER_QUEUE 64
#define UPH_VST3_MAX_ATTRS 32
#define UPH_VST3_ATTR_ID_LEN 64
#define UPH_VST3_ATTR_BINARY_MAX 4096
#define UPH_VST3_ATTR_STRING_MAX 256
#define UPH_VST3_MAX_MESSAGES 16

typedef struct
{
    Steinberg_Vst_IEventList iface;
    struct Steinberg_Vst_Event events[UPH_VST3_MAX_EVENTS];
    int32_t count;
}
Uph_Vst3EventList;

typedef struct
{
    Steinberg_Vst_IParamValueQueue iface;
    Steinberg_Vst_ParamID id;
    int32_t count;
    int32_t offsets[UPH_VST3_MAX_POINTS_PER_QUEUE];
    double values[UPH_VST3_MAX_POINTS_PER_QUEUE];
}
Uph_Vst3ParamQueue;

typedef struct
{
    Steinberg_Vst_IParameterChanges iface;
    Uph_Vst3ParamQueue queues[UPH_VST3_MAX_PARAM_QUEUES];
    int32_t count;
}
Uph_Vst3ParamChanges;

typedef struct
{
    Steinberg_IBStream iface;
    uint8_t *data;
    int64_t size;
    int64_t capacity;
    int64_t pos;
    bool owns_data;
}
Uph_Vst3MemStream;

typedef enum
{
    UPH_VST3_ATTR_NONE = 0,
    UPH_VST3_ATTR_INT,
    UPH_VST3_ATTR_FLOAT,
    UPH_VST3_ATTR_STRING,
    UPH_VST3_ATTR_BINARY
}
Uph_Vst3AttrKind;

typedef struct
{
    char id[UPH_VST3_ATTR_ID_LEN];
    Uph_Vst3AttrKind kind;
    int64_t int_value;
    double float_value;
    Steinberg_Vst_TChar string_value[UPH_VST3_ATTR_STRING_MAX];
    uint8_t binary_value[UPH_VST3_ATTR_BINARY_MAX];
    uint32_t binary_size;
}
Uph_Vst3Attr;

typedef struct
{
    Steinberg_Vst_IAttributeList iface;
    Uph_Vst3Attr attrs[UPH_VST3_MAX_ATTRS];
    int32_t count;
}
Uph_Vst3AttrList;

typedef struct
{
    Steinberg_Vst_IMessage iface;
    char message_id[UPH_VST3_ATTR_ID_LEN];
    Uph_Vst3AttrList attributes;
    bool in_use;
}
Uph_Vst3Message;

#define UPH_MAX_PLUGIN_TIMERS 8

typedef struct
{
    clap_id id;
    uint32_t period_ms;
    float last_fire;
    bool active;
}
Uph_ClapTimer;

typedef struct Uph_PluginInternalHandle_
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
            const clap_plugin_entry_t *entry;

            Uph_ClapTimer timers[UPH_MAX_PLUGIN_TIMERS];
            Uph_NoteEventRing pending_notes;
            Uph_ParamEventRing pending_params;

            bool active_notes[128];
            int16_t active_note_channels[128];

            clap_id next_timer_id;
            volatile int ready;
        }
        clap;

        struct
        {
            Steinberg_Vst_IComponent *component;
            Steinberg_Vst_IAudioProcessor *processor;
            Steinberg_Vst_IEditController *controller;
            Steinberg_Vst_IConnectionPoint *component_cp;
            Steinberg_Vst_IConnectionPoint *controller_cp;
            Steinberg_IPlugView *view;
            Steinberg_IPluginFactory *factory;
            void *library_handle;
            bool controller_is_component;
            bool connected;
            bool component_initialized;
            bool controller_initialized;

            Uph_Vst3Host host_app;
            Uph_Vst3Handler handler;
            Uph_Vst3Frame frame;
            Uph_Vst3RunLoop run_loop;

            Uph_NoteEventRing pending_notes;
            Uph_ParamEventRing pending_params;

            bool active_notes[128];
            int16_t active_note_channels[128];

            uint32_t width, height;
            bool has_event_input;
            int32_t audio_in_buses;
            int32_t audio_out_buses;
            int32_t in_channels;
            int32_t out_channels;
            int32_t next_note_id;
            int32_t note_ids[128];
            int64_t sample_position;
            volatile int ready;

            Uph_Vst3EventList in_events;
            Uph_Vst3EventList out_events;
            Uph_Vst3ParamChanges in_params;
            Uph_Vst3ParamChanges out_params;
        }
        vst3;
    };

#if NAUI_LINUX
    Window window;
    Display *display;
    Atom wm_delete_window;
#elif NAUI_WINDOWS
    HWND window;
    bool dragging;
    POINT drag_start_cursor;
    POINT drag_start_origin;
#endif
    bool visible;
    bool is_clap;
    Naui_String display_name;
}
Uph_PluginInternalHandle;

static Uph_Vst3Message uph_vst3_message_pool[UPH_VST3_MAX_MESSAGES];
static Uph_Vst3AttrList uph_vst3_attr_list_pool[UPH_VST3_MAX_MESSAGES];
static bool uph_vst3_attr_list_pool_used[UPH_VST3_MAX_MESSAGES];

static Steinberg_uint32 SMTG_STDMETHODCALLTYPE uph_vst3_static_add_ref(void *self)  { (void)self; return 1; }
static Steinberg_uint32 SMTG_STDMETHODCALLTYPE uph_vst3_static_release(void *self)  { (void)self; return 1; }

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_stream_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_IBStream_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_stream_read(void *self, void *buffer, Steinberg_int32 num_bytes, Steinberg_int32 *num_read)
{
    Uph_Vst3MemStream *s = (Uph_Vst3MemStream*)self;
    int64_t remaining = s->size - s->pos;
    int64_t n = num_bytes < remaining ? num_bytes : remaining;
    if (n < 0) n = 0;

    if (n > 0)
        memcpy(buffer, s->data + s->pos, (size_t)n);
    s->pos += n;

    if (num_read)
        *num_read = (Steinberg_int32)n;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_stream_write(void *self, void *buffer, Steinberg_int32 num_bytes, Steinberg_int32 *num_written)
{
    Uph_Vst3MemStream *s = (Uph_Vst3MemStream*)self;
    if (num_bytes < 0)
        return Steinberg_kInvalidArgument;

    int64_t end = s->pos + num_bytes;
    if (end > s->capacity)
    {
        int64_t new_cap = s->capacity ? s->capacity * 2 : 4096;
        while (new_cap < end)
            new_cap *= 2;

        uint8_t *p = (uint8_t*)realloc(s->data, (size_t)new_cap);
        if (!p)
            return Steinberg_kOutOfMemory;
        s->data = p;
        s->capacity = new_cap;
    }

    memcpy(s->data + s->pos, buffer, (size_t)num_bytes);
    s->pos = end;
    if (end > s->size)
        s->size = end;

    if (num_written)
        *num_written = num_bytes;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_stream_seek(void *self, Steinberg_int64 pos, Steinberg_int32 mode, Steinberg_int64 *result)
{
    Uph_Vst3MemStream *s = (Uph_Vst3MemStream*)self;
    int64_t np;

    switch (mode)
    {
        case Steinberg_IBStream_IStreamSeekMode_kIBSeekSet: np = pos; break;
        case Steinberg_IBStream_IStreamSeekMode_kIBSeekCur: np = s->pos + pos; break;
        case Steinberg_IBStream_IStreamSeekMode_kIBSeekEnd: np = s->size + pos; break;
        default: return Steinberg_kInvalidArgument;
    }

    if (np < 0 || np > s->size)
        return Steinberg_kResultFalse;

    s->pos = np;
    if (result)
        *result = np;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_stream_tell(void *self, Steinberg_int64 *pos)
{
    Uph_Vst3MemStream *s = (Uph_Vst3MemStream*)self;
    if (pos)
        *pos = s->pos;
    return Steinberg_kResultOk;
}

static Steinberg_IBStreamVtbl uph_vst3_stream_vtbl = {
    uph_vst3_stream_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_stream_read,
    uph_vst3_stream_write,
    uph_vst3_stream_seek,
    uph_vst3_stream_tell,
};

static void uph_vst3_stream_init_write(Uph_Vst3MemStream *s)
{
    memset(s, 0, sizeof(*s));
    s->iface.lpVtbl = &uph_vst3_stream_vtbl;
    s->owns_data = true;
}

static void uph_vst3_stream_init_read(Uph_Vst3MemStream *s, const uint8_t *data, int64_t size)
{
    memset(s, 0, sizeof(*s));
    s->iface.lpVtbl = &uph_vst3_stream_vtbl;
    s->data = (uint8_t*)data;
    s->size = size;
    s->capacity = size;
}

static Uph_Vst3Attr *uph_vst3_attr_find(Uph_Vst3AttrList *l, const char *id, bool create)
{
    for (int32_t i = 0; i < l->count; i++)
        if (strncmp(l->attrs[i].id, id, UPH_VST3_ATTR_ID_LEN) == 0)
            return &l->attrs[i];

    if (!create || l->count >= UPH_VST3_MAX_ATTRS)
        return NULL;

    Uph_Vst3Attr *a = &l->attrs[l->count++];
    memset(a, 0, sizeof(*a));
    snprintf(a->id, sizeof(a->id), "%s", id);
    return a;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Vst_IAttributeList_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_set_int(void *self, Steinberg_Vst_IAttributeList_AttrID id, Steinberg_int64 value)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, true);
    if (!a)
        return Steinberg_kOutOfMemory;
    a->kind = UPH_VST3_ATTR_INT;
    a->int_value = value;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_get_int(void *self, Steinberg_Vst_IAttributeList_AttrID id, Steinberg_int64 *value)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, false);
    if (!a || a->kind != UPH_VST3_ATTR_INT || !value)
        return Steinberg_kResultFalse;
    *value = a->int_value;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_set_float(void *self, Steinberg_Vst_IAttributeList_AttrID id, double value)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, true);
    if (!a)
        return Steinberg_kOutOfMemory;
    a->kind = UPH_VST3_ATTR_FLOAT;
    a->float_value = value;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_get_float(void *self, Steinberg_Vst_IAttributeList_AttrID id, double *value)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, false);
    if (!a || a->kind != UPH_VST3_ATTR_FLOAT || !value)
        return Steinberg_kResultFalse;
    *value = a->float_value;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_set_string(void *self, Steinberg_Vst_IAttributeList_AttrID id, const Steinberg_Vst_TChar *string)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, true);
    if (!a || !string)
        return Steinberg_kOutOfMemory;

    size_t i = 0;
    for (; i + 1 < UPH_VST3_ATTR_STRING_MAX && string[i]; i++)
        a->string_value[i] = string[i];
    a->string_value[i] = 0;
    a->kind = UPH_VST3_ATTR_STRING;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_get_string(void *self, Steinberg_Vst_IAttributeList_AttrID id, Steinberg_Vst_TChar *string, Steinberg_uint32 size_in_bytes)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, false);
    if (!a || a->kind != UPH_VST3_ATTR_STRING || !string || size_in_bytes < sizeof(Steinberg_Vst_TChar))
        return Steinberg_kResultFalse;

    size_t capacity = size_in_bytes / sizeof(Steinberg_Vst_TChar);
    size_t i = 0;
    for (; i + 1 < capacity && a->string_value[i]; i++)
        string[i] = a->string_value[i];
    string[i] = 0;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_set_binary(void *self, Steinberg_Vst_IAttributeList_AttrID id, const void *data, Steinberg_uint32 size_in_bytes)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    if (size_in_bytes > UPH_VST3_ATTR_BINARY_MAX)
        return Steinberg_kOutOfMemory;

    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, true);
    if (!a)
        return Steinberg_kOutOfMemory;

    if (size_in_bytes && data)
        memcpy(a->binary_value, data, size_in_bytes);
    a->binary_size = size_in_bytes;
    a->kind = UPH_VST3_ATTR_BINARY;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_attrs_get_binary(void *self, Steinberg_Vst_IAttributeList_AttrID id, const void **data, Steinberg_uint32 *size_in_bytes)
{
    Uph_Vst3AttrList *l = (Uph_Vst3AttrList*)self;
    Uph_Vst3Attr *a = uph_vst3_attr_find(l, id, false);
    if (!a || a->kind != UPH_VST3_ATTR_BINARY || !data || !size_in_bytes)
        return Steinberg_kResultFalse;
    *data = a->binary_value;
    *size_in_bytes = a->binary_size;
    return Steinberg_kResultOk;
}

static Steinberg_Vst_IAttributeListVtbl uph_vst3_attrs_vtbl = {
    uph_vst3_attrs_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_attrs_set_int,
    uph_vst3_attrs_get_int,
    uph_vst3_attrs_set_float,
    uph_vst3_attrs_get_float,
    uph_vst3_attrs_set_string,
    uph_vst3_attrs_get_string,
    uph_vst3_attrs_set_binary,
    uph_vst3_attrs_get_binary,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_message_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Vst_IMessage_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_uint32 SMTG_STDMETHODCALLTYPE uph_vst3_message_release(void *self)
{
    Uph_Vst3Message *m = (Uph_Vst3Message*)self;
    m->in_use = false;
    return 1;
}

static const char *SMTG_STDMETHODCALLTYPE uph_vst3_message_get_id(void *self)
{
    return ((Uph_Vst3Message*)self)->message_id;
}

static void SMTG_STDMETHODCALLTYPE uph_vst3_message_set_id(void *self, const char *id)
{
    Uph_Vst3Message *m = (Uph_Vst3Message*)self;
    snprintf(m->message_id, sizeof(m->message_id), "%s", id ? id : "");
}

static struct Steinberg_Vst_IAttributeList *SMTG_STDMETHODCALLTYPE uph_vst3_message_get_attributes(void *self)
{
    return &((Uph_Vst3Message*)self)->attributes.iface;
}

static Steinberg_Vst_IMessageVtbl uph_vst3_message_vtbl = {
    uph_vst3_message_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_message_release,
    uph_vst3_message_get_id,
    uph_vst3_message_set_id,
    uph_vst3_message_get_attributes,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_host_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Vst_IHostApplication_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_host_get_name(void *self, Steinberg_Vst_String128 name)
{
    (void)self;
    static const char host_name[] = "Uphonic";
    for (size_t i = 0; i < sizeof(host_name); i++)
        name[i] = (Steinberg_Vst_TChar)host_name[i];
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_host_create_instance(void *self, Steinberg_TUID cid, Steinberg_TUID iid, void **obj)
{
    (void)self;

    if (!obj)
        return Steinberg_kInvalidArgument;

    *obj = NULL;

    if (uph_vst3_iid_equal(cid, Steinberg_Vst_IMessage_iid) && uph_vst3_iid_equal(iid, Steinberg_Vst_IMessage_iid))
    {
        for (int i = 0; i < UPH_VST3_MAX_MESSAGES; i++)
        {
            Uph_Vst3Message *m = &uph_vst3_message_pool[i];
            if (m->in_use)
                continue;

            memset(m, 0, sizeof(*m));
            m->in_use = true;
            m->iface.lpVtbl = &uph_vst3_message_vtbl;
            m->attributes.iface.lpVtbl = &uph_vst3_attrs_vtbl;
            *obj = &m->iface;
            return Steinberg_kResultOk;
        }
        return Steinberg_kOutOfMemory;
    }

    if (uph_vst3_iid_equal(cid, Steinberg_Vst_IAttributeList_iid) && uph_vst3_iid_equal(iid, Steinberg_Vst_IAttributeList_iid))
    {
        for (int i = 0; i < UPH_VST3_MAX_MESSAGES; i++)
        {
            if (uph_vst3_attr_list_pool_used[i])
                continue;

            uph_vst3_attr_list_pool_used[i] = true;
            Uph_Vst3AttrList *l = &uph_vst3_attr_list_pool[i];
            memset(l, 0, sizeof(*l));
            l->iface.lpVtbl = &uph_vst3_attrs_vtbl;
            *obj = &l->iface;
            return Steinberg_kResultOk;
        }
        return Steinberg_kOutOfMemory;
    }

    return Steinberg_kNotImplemented;
}

static Steinberg_Vst_IHostApplicationVtbl uph_vst3_host_vtbl = {
    uph_vst3_host_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_host_get_name,
    uph_vst3_host_create_instance,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_handler_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Vst_IComponentHandler_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_handler_begin_edit(void *self, Steinberg_Vst_ParamID id)
{
    (void)self; (void)id;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_handler_perform_edit(void *self, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue value)
{
    Uph_Vst3Handler *h = (Uph_Vst3Handler*)self;
    if (!h->owner)
        return Steinberg_kResultOk;

    clap_event_param_value_t ev = {0};
    ev.header.size = sizeof(ev);
    ev.header.time = 0;
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_PARAM_VALUE;
    ev.param_id = id;
    ev.note_id = -1;
    ev.port_index = -1;
    ev.channel = -1;
    ev.key = -1;
    ev.value = value;

    uph_param_ring_push(&h->owner->vst3.pending_params, &ev);
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_handler_end_edit(void *self, Steinberg_Vst_ParamID id)
{
    (void)self; (void)id;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_handler_restart_component(void *self, Steinberg_int32 flags)
{
    (void)self; (void)flags;
    return Steinberg_kResultOk;
}

static Steinberg_Vst_IComponentHandlerVtbl uph_vst3_handler_vtbl = {
    uph_vst3_handler_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_handler_begin_edit,
    uph_vst3_handler_perform_edit,
    uph_vst3_handler_end_edit,
    uph_vst3_handler_restart_component,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_runloop_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Linux_IRunLoop_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_runloop_register_event_handler(void *self, Steinberg_Linux_IEventHandler *handler, Steinberg_Linux_FileDescriptor fd)
{
    Uph_Vst3RunLoop *rl = (Uph_Vst3RunLoop*)self;
    for (int i = 0; i < UPH_VST3_MAX_FD_HANDLERS; i++)
    {
        if (!rl->fds[i].handler)
        {
            rl->fds[i].handler = handler;
            rl->fds[i].fd = fd;
            return Steinberg_kResultOk;
        }
    }
    return Steinberg_kOutOfMemory;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_runloop_unregister_event_handler(void *self, Steinberg_Linux_IEventHandler *handler)
{
    Uph_Vst3RunLoop *rl = (Uph_Vst3RunLoop*)self;
    for (int i = 0; i < UPH_VST3_MAX_FD_HANDLERS; i++)
        if (rl->fds[i].handler == handler)
            rl->fds[i].handler = NULL;
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_runloop_register_timer(void *self, Steinberg_Linux_ITimerHandler *handler, Steinberg_Linux_TimerInterval ms)
{
    Uph_Vst3RunLoop *rl = (Uph_Vst3RunLoop*)self;
    for (int i = 0; i < UPH_VST3_MAX_TIMERS; i++)
    {
        if (!rl->timers[i].handler)
        {
            rl->timers[i].handler = handler;
            rl->timers[i].period_ms = ms ? ms : 1;
            rl->timers[i].last_fire = naui_frame_time();
            return Steinberg_kResultOk;
        }
    }
    return Steinberg_kOutOfMemory;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_runloop_unregister_timer(void *self, Steinberg_Linux_ITimerHandler *handler)
{
    Uph_Vst3RunLoop *rl = (Uph_Vst3RunLoop*)self;
    for (int i = 0; i < UPH_VST3_MAX_TIMERS; i++)
        if (rl->timers[i].handler == handler)
            rl->timers[i].handler = NULL;
    return Steinberg_kResultOk;
}

static Steinberg_Linux_IRunLoopVtbl uph_vst3_runloop_vtbl = {
    uph_vst3_runloop_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_runloop_register_event_handler,
    uph_vst3_runloop_unregister_event_handler,
    uph_vst3_runloop_register_timer,
    uph_vst3_runloop_unregister_timer,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_frame_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    Uph_Vst3Frame *f = (Uph_Vst3Frame*)self;

    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_IPlugFrame_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
#if NAUI_LINUX
    if (uph_vst3_iid_equal(iid, Steinberg_Linux_IRunLoop_iid))
    {
        *obj = &f->run_loop->iface;
        return Steinberg_kResultOk;
    }
#else
    (void)f;
#endif
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_frame_resize_view(void *self, struct Steinberg_IPlugView *view, struct Steinberg_ViewRect *new_size)
{
    Uph_Vst3Frame *f = (Uph_Vst3Frame*)self;
    if (!f->owner || !new_size)
        return Steinberg_kInvalidArgument;

    uint32_t w = (uint32_t)(new_size->right - new_size->left);
    uint32_t h = (uint32_t)(new_size->bottom - new_size->top);

#if NAUI_LINUX
    XResizeWindow(f->owner->display, f->owner->window, w, h);

    XSizeHints *hints = XAllocSizeHints();
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = hints->max_width = (int)w;
    hints->min_height = hints->max_height = (int)h;
    XSetWMNormalHints(f->owner->display, f->owner->window, hints);
    XFree(hints);
    XFlush(f->owner->display);
#elif NAUI_WINDOWS
    RECT rect = { 0, 0, (LONG)w, (LONG)h };
    DWORD style = (DWORD)GetWindowLongA(f->owner->window, GWL_STYLE);
    DWORD ex_style = (DWORD)GetWindowLongA(f->owner->window, GWL_EXSTYLE);
    AdjustWindowRectEx(&rect, style, FALSE, ex_style);
    SetWindowPos(f->owner->window, NULL, 0, 0,
        rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif

    f->owner->vst3.width = w;
    f->owner->vst3.height = h;

    view->lpVtbl->onSize(view, new_size);
    return Steinberg_kResultOk;
}

static Steinberg_IPlugFrameVtbl uph_vst3_frame_vtbl = {
    uph_vst3_frame_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_frame_resize_view,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_events_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Vst_IEventList_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_int32 SMTG_STDMETHODCALLTYPE uph_vst3_events_get_count(void *self)
{
    return ((Uph_Vst3EventList*)self)->count;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_events_get_event(void *self, Steinberg_int32 index, struct Steinberg_Vst_Event *e)
{
    Uph_Vst3EventList *l = (Uph_Vst3EventList*)self;
    if (index < 0 || index >= l->count)
        return Steinberg_kInvalidArgument;
    *e = l->events[index];
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_events_add_event(void *self, struct Steinberg_Vst_Event *e)
{
    (void)self; (void)e;
    return Steinberg_kResultOk;
}

static Steinberg_Vst_IEventListVtbl uph_vst3_events_vtbl = {
    uph_vst3_events_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_events_get_count,
    uph_vst3_events_get_event,
    uph_vst3_events_add_event,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_pq_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Vst_IParamValueQueue_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_Vst_ParamID SMTG_STDMETHODCALLTYPE uph_vst3_pq_get_parameter_id(void *self)
{
    return ((Uph_Vst3ParamQueue*)self)->id;
}

static Steinberg_int32 SMTG_STDMETHODCALLTYPE uph_vst3_pq_get_point_count(void *self)
{
    return ((Uph_Vst3ParamQueue*)self)->count;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_pq_get_point(void *self, Steinberg_int32 index, Steinberg_int32 *sample_offset, Steinberg_Vst_ParamValue *value)
{
    Uph_Vst3ParamQueue *q = (Uph_Vst3ParamQueue*)self;
    if (index < 0 || index >= q->count)
        return Steinberg_kInvalidArgument;
    *sample_offset = q->offsets[index];
    *value = q->values[index];
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_pq_add_point(void *self, Steinberg_int32 sample_offset, Steinberg_Vst_ParamValue value, Steinberg_int32 *index)
{
    (void)self; (void)sample_offset; (void)value; (void)index;
    return Steinberg_kResultFalse;
}

static Steinberg_Vst_IParamValueQueueVtbl uph_vst3_pq_vtbl = {
    uph_vst3_pq_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_pq_get_parameter_id,
    uph_vst3_pq_get_point_count,
    uph_vst3_pq_get_point,
    uph_vst3_pq_add_point,
};

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_pc_query_interface(void *self, const Steinberg_TUID iid, void **obj)
{
    if (uph_vst3_iid_equal(iid, Steinberg_FUnknown_iid) || uph_vst3_iid_equal(iid, Steinberg_Vst_IParameterChanges_iid))
    {
        *obj = self;
        return Steinberg_kResultOk;
    }
    *obj = NULL;
    return Steinberg_kNoInterface;
}

static Steinberg_int32 SMTG_STDMETHODCALLTYPE uph_vst3_pc_get_parameter_count(void *self)
{
    return ((Uph_Vst3ParamChanges*)self)->count;
}

static struct Steinberg_Vst_IParamValueQueue* SMTG_STDMETHODCALLTYPE uph_vst3_pc_get_parameter_data(void *self, Steinberg_int32 index)
{
    Uph_Vst3ParamChanges *c = (Uph_Vst3ParamChanges*)self;
    if (index < 0 || index >= c->count)
        return NULL;
    return &c->queues[index].iface;
}

static struct Steinberg_Vst_IParamValueQueue* SMTG_STDMETHODCALLTYPE uph_vst3_pc_add_parameter_data(void *self, const Steinberg_Vst_ParamID *id, Steinberg_int32 *index)
{
    Uph_Vst3ParamChanges *c = (Uph_Vst3ParamChanges*)self;

    for (int32_t i = 0; i < c->count; i++)
    {
        if (c->queues[i].id == *id)
        {
            if (index) *index = i;
            return &c->queues[i].iface;
        }
    }

    if (c->count >= UPH_VST3_MAX_PARAM_QUEUES)
        return NULL;

    Uph_Vst3ParamQueue *q = &c->queues[c->count];
    q->iface.lpVtbl = &uph_vst3_pq_vtbl;
    q->id = *id;
    q->count = 0;
    if (index) *index = c->count;
    c->count++;
    return &q->iface;
}

static Steinberg_Vst_IParameterChangesVtbl uph_vst3_pc_vtbl = {
    uph_vst3_pc_query_interface,
    uph_vst3_static_add_ref,
    uph_vst3_static_release,
    uph_vst3_pc_get_parameter_count,
    uph_vst3_pc_get_parameter_data,
    uph_vst3_pc_add_parameter_data,
};

static void uph_vst3_param_changes_reset(Uph_Vst3ParamChanges *c)
{
    c->iface.lpVtbl = &uph_vst3_pc_vtbl;
    c->count = 0;
}

static void uph_vst3_param_changes_add(Uph_Vst3ParamChanges *c, Steinberg_Vst_ParamID id, int32_t offset, double value)
{
    Uph_Vst3ParamQueue *q = NULL;
    for (int32_t i = 0; i < c->count; i++)
        if (c->queues[i].id == id) { q = &c->queues[i]; break; }

    if (!q)
    {
        if (c->count >= UPH_VST3_MAX_PARAM_QUEUES)
            return;
        q = &c->queues[c->count++];
        q->iface.lpVtbl = &uph_vst3_pq_vtbl;
        q->id = id;
        q->count = 0;
    }

    if (q->count >= UPH_VST3_MAX_POINTS_PER_QUEUE)
    {
        q->offsets[q->count - 1] = offset;
        q->values[q->count - 1] = value;
        return;
    }

    q->offsets[q->count] = offset;
    q->values[q->count] = value;
    q->count++;
}
static void uph_vst3_string128_to_utf8(const Steinberg_Vst_String128 src, char *dst, size_t dst_size)
{
    size_t o = 0;
    for (size_t i = 0; i < 128 && src[i] && o + 4 < dst_size; i++)
    {
        uint32_t c = (uint16_t)src[i];

        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < 128 && (uint16_t)src[i + 1] >= 0xDC00 && (uint16_t)src[i + 1] <= 0xDFFF)
        {
            c = 0x10000 + ((c - 0xD800) << 10) + ((uint16_t)src[i + 1] - 0xDC00);
            i++;
        }

        if (c < 0x80) dst[o++] = (char)c;
        else if (c < 0x800)
        {
            dst[o++] = (char)(0xC0 | (c >> 6));
            dst[o++] = (char)(0x80 | (c & 0x3F));
        }
        else if (c < 0x10000)
        {
            dst[o++] = (char)(0xE0 | (c >> 12));
            dst[o++] = (char)(0x80 | ((c >> 6) & 0x3F));
            dst[o++] = (char)(0x80 | (c & 0x3F));
        }
        else
        {
            dst[o++] = (char)(0xF0 | (c >> 18));
            dst[o++] = (char)(0x80 | ((c >> 12) & 0x3F));
            dst[o++] = (char)(0x80 | ((c >> 6) & 0x3F));
            dst[o++] = (char)(0x80 | (c & 0x3F));
        }
    }
    dst[o] = '\0';
}

static Naui_List(Uph_PluginParam) uph_vst3_get_param_list(Uph_Plugin *plug)
{
    Naui_List(Uph_PluginParam) list = NULL;

    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!ih || !ih->vst3.controller)
        return list;

    Steinberg_Vst_IEditController *ctrl = ih->vst3.controller;
    Steinberg_int32 count = ctrl->lpVtbl->getParameterCount(ctrl);
    naui_list_reserve(list, (size_t)count);

    for (Steinberg_int32 i = 0; i < count; i++)
    {
        struct Steinberg_Vst_ParameterInfo info;
        if (ctrl->lpVtbl->getParameterInfo(ctrl, i, &info) != Steinberg_kResultOk)
            continue;

        if (info.flags & Steinberg_Vst_ParameterInfo_ParameterFlags_kIsHidden)
            continue;

        char name[256], module[256];
        uph_vst3_string128_to_utf8(info.title, name, sizeof(name));
        uph_vst3_string128_to_utf8(info.units, module, sizeof(module));

        Uph_PluginParam p = {
            .id = info.id,
            .name = naui_string_from_cstr(name),
            .module = naui_string_from_cstr(module),
            .min_value = 0.0,
            .max_value = 1.0,
            .default_value = info.defaultNormalizedValue,
            .current_value = ctrl->lpVtbl->getParamNormalized(ctrl, info.id)
        };

        naui_list_push(list, p);
    }

    return list;
}

typedef Steinberg_IPluginFactory* (*Uph_Vst3GetFactoryFn)(void);
typedef bool (*Uph_Vst3ModuleEntryFn)(void *handle);
typedef bool (*Uph_Vst3ModuleExitFn)(void);
typedef bool (*Uph_Vst3InitDllFn)(void);
typedef bool (*Uph_Vst3ExitDllFn)(void);

static bool uph_vst3_resolve_binary(const char *path, char *out, size_t out_size)
{
#if NAUI_LINUX
    struct stat st;
    if (stat(path, &st) != 0)
        return false;

    if (!S_ISDIR(st.st_mode))
    {
        snprintf(out, out_size, "%s", path);
        return true;
    }

    const char *end = path + strlen(path);
    while (end > path && (end[-1] == '/' || end[-1] == '\\')) end--;
    const char *base = end;
    while (base > path && base[-1] != '/' && base[-1] != '\\') base--;

    char stem[256];
    size_t n = (size_t)(end - base);
    if (n >= sizeof(stem)) n = sizeof(stem) - 1;
    memcpy(stem, base, n);
    stem[n] = '\0';
    char *dot = strrchr(stem, '.');
    if (dot) *dot = '\0';

    snprintf(out, out_size, "%.*s/Contents/x86_64-linux/%s.so", (int)(end - path), path, stem);
    return stat(out, &st) == 0;
#elif NAUI_WINDOWS
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return false;

    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        snprintf(out, out_size, "%s", path);
        return true;
    }

    const char *end = path + strlen(path);
    while (end > path && (end[-1] == '/' || end[-1] == '\\')) end--;
    const char *base = end;
    while (base > path && base[-1] != '/' && base[-1] != '\\') base--;

    snprintf(out, out_size, "%.*s\\Contents\\x86_64-win\\%.*s",
        (int)(end - path), path, (int)(end - base), base);
    return GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES;
#endif
}

static void uph_vst3_free_partial(Uph_PluginInternalHandle *ih)
{
    ih->vst3.ready = 0;

    if (ih->vst3.view)
    {
        ih->vst3.view->lpVtbl->release(ih->vst3.view);
        ih->vst3.view = NULL;
    }

    if (ih->vst3.connected && ih->vst3.component_cp && ih->vst3.controller_cp)
    {
        ih->vst3.component_cp->lpVtbl->disconnect(ih->vst3.component_cp, ih->vst3.controller_cp);
        ih->vst3.controller_cp->lpVtbl->disconnect(ih->vst3.controller_cp, ih->vst3.component_cp);
        ih->vst3.connected = false;
    }

    if (ih->vst3.controller_cp)
    {
        ih->vst3.controller_cp->lpVtbl->release(ih->vst3.controller_cp);
        ih->vst3.controller_cp = NULL;
    }

    if (ih->vst3.component_cp)
    {
        ih->vst3.component_cp->lpVtbl->release(ih->vst3.component_cp);
        ih->vst3.component_cp = NULL;
    }

    if (ih->vst3.controller)
    {
        ih->vst3.controller->lpVtbl->setComponentHandler(ih->vst3.controller, NULL);

        if (!ih->vst3.controller_is_component && ih->vst3.controller_initialized)
            ((Steinberg_IPluginBase*)ih->vst3.controller)->lpVtbl->terminate(ih->vst3.controller);

        ih->vst3.controller->lpVtbl->release(ih->vst3.controller);
        ih->vst3.controller = NULL;
        ih->vst3.controller_initialized = false;
    }

    if (ih->vst3.processor)
    {
        ih->vst3.processor->lpVtbl->release(ih->vst3.processor);
        ih->vst3.processor = NULL;
    }

    if (ih->vst3.component)
    {
        if (ih->vst3.component_initialized)
            ((Steinberg_IPluginBase*)ih->vst3.component)->lpVtbl->terminate(ih->vst3.component);

        ih->vst3.component->lpVtbl->release(ih->vst3.component);
        ih->vst3.component = NULL;
        ih->vst3.component_initialized = false;
    }

    if (ih->vst3.factory)
    {
        ih->vst3.factory->lpVtbl->release(ih->vst3.factory);
        ih->vst3.factory = NULL;
    }

    if (ih->vst3.library_handle)
    {
#if NAUI_LINUX
        Uph_Vst3ModuleExitFn exit_fn = (Uph_Vst3ModuleExitFn)dlsym(ih->vst3.library_handle, "ModuleExit");
        if (exit_fn) exit_fn();
        dlclose(ih->vst3.library_handle);
#elif NAUI_WINDOWS
        Uph_Vst3ExitDllFn exit_fn = (Uph_Vst3ExitDllFn)GetProcAddress((HMODULE)ih->vst3.library_handle, "ExitDll");
        if (exit_fn) exit_fn();
        FreeLibrary((HMODULE)ih->vst3.library_handle);
#endif
        ih->vst3.library_handle = NULL;
    }
}

static inline void uph_load_vst3_plugin_internal(Uph_Plugin *plug, uint32_t *width, uint32_t *height)
{
    uph_com_init_sta_once();

    char binary[1024];
    if (!uph_vst3_resolve_binary(plug->file_path.data, binary, sizeof(binary)))
    {
        fprintf(stderr, "uph vst3: could not resolve binary for %s\n", plug->file_path.data);
        return;
    }

    void *lib = NULL;
    Uph_Vst3GetFactoryFn get_factory = NULL;

#if NAUI_LINUX
    lib = dlopen(binary, RTLD_LOCAL | RTLD_LAZY);
    if (!lib) { fprintf(stderr, "uph vst3: dlopen failed: %s\n", dlerror()); return; }

    Uph_Vst3ModuleEntryFn entry = (Uph_Vst3ModuleEntryFn)dlsym(lib, "ModuleEntry");
    if (entry) entry(lib);
    get_factory = (Uph_Vst3GetFactoryFn)dlsym(lib, "GetPluginFactory");
#elif NAUI_WINDOWS
    lib = (void*)LoadLibraryA(binary);
    if (!lib) { fprintf(stderr, "uph vst3: LoadLibraryA failed: %lu\n", GetLastError()); return; }

    Uph_Vst3InitDllFn init_dll = (Uph_Vst3InitDllFn)GetProcAddress((HMODULE)lib, "InitDll");
    if (init_dll) init_dll();
    get_factory = (Uph_Vst3GetFactoryFn)GetProcAddress((HMODULE)lib, "GetPluginFactory");
#endif

    if (!get_factory)
    {
        fprintf(stderr, "uph vst3: no GetPluginFactory export\n");
#if NAUI_LINUX
        Uph_Vst3ModuleExitFn exit_fn = (Uph_Vst3ModuleExitFn)dlsym(lib, "ModuleExit");
        if (exit_fn) exit_fn();
        dlclose(lib);
#elif NAUI_WINDOWS
        Uph_Vst3ExitDllFn exit_fn = (Uph_Vst3ExitDllFn)GetProcAddress((HMODULE)lib, "ExitDll");
        if (exit_fn) exit_fn();
        FreeLibrary((HMODULE)lib);
#endif
        return;
    }

    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)calloc(1, sizeof(Uph_PluginInternalHandle));
    ih->vst3.library_handle = lib;
    ih->vst3.factory = get_factory();
    if (!ih->vst3.factory)
    {
        fprintf(stderr, "uph vst3: GetPluginFactory returned NULL\n");
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }

    uph_note_ring_init(&ih->vst3.pending_notes);
    uph_param_ring_init(&ih->vst3.pending_params);

    ih->vst3.host_app.iface.lpVtbl = &uph_vst3_host_vtbl;
    ih->vst3.handler.iface.lpVtbl = &uph_vst3_handler_vtbl;
    ih->vst3.handler.owner = ih;
    ih->vst3.run_loop.iface.lpVtbl = &uph_vst3_runloop_vtbl;
    ih->vst3.frame.iface.lpVtbl = &uph_vst3_frame_vtbl;
    ih->vst3.frame.run_loop = &ih->vst3.run_loop;
    ih->vst3.frame.owner = ih;
    ih->vst3.in_events.iface.lpVtbl = &uph_vst3_events_vtbl;
    ih->vst3.out_events.iface.lpVtbl = &uph_vst3_events_vtbl;
    uph_vst3_param_changes_reset(&ih->vst3.in_params);
    uph_vst3_param_changes_reset(&ih->vst3.out_params);

    Steinberg_IPluginFactory3 *factory3 = NULL;
    if (ih->vst3.factory->lpVtbl->queryInterface(ih->vst3.factory, Steinberg_IPluginFactory3_iid, (void**)&factory3) == Steinberg_kResultOk && factory3)
    {
        factory3->lpVtbl->setHostContext(factory3, (struct Steinberg_FUnknown*)&ih->vst3.host_app);
        factory3->lpVtbl->release(factory3);
    }

    Steinberg_int32 class_count = ih->vst3.factory->lpVtbl->countClasses(ih->vst3.factory);
    struct Steinberg_PClassInfo class_info;
    memset(&class_info, 0, sizeof(class_info));
    bool found = false;

    for (Steinberg_int32 i = 0; i < class_count; i++)
    {
        if (ih->vst3.factory->lpVtbl->getClassInfo(ih->vst3.factory, i, &class_info) != Steinberg_kResultOk)
            continue;
        if (strcmp(class_info.category, "Audio Module Class") == 0)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        fprintf(stderr, "uph vst3: no Audio Module Class in %s\n", plug->file_path.data);
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }

    fprintf(stderr, "loading vst3 plugin: %s\n", class_info.name);
    ih->display_name = naui_string_from_cstr(class_info.name);

    if (ih->vst3.factory->lpVtbl->createInstance(
            ih->vst3.factory, class_info.cid, Steinberg_Vst_IComponent_iid, (void**)&ih->vst3.component) != Steinberg_kResultOk
        || !ih->vst3.component)
    {
        ih->vst3.component = NULL;
        fprintf(stderr, "uph vst3: createInstance(IComponent) failed\n");
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }

    if (((Steinberg_IPluginBase*)ih->vst3.component)->lpVtbl->initialize(
            ih->vst3.component, (struct Steinberg_FUnknown*)&ih->vst3.host_app) != Steinberg_kResultOk)
    {
        fprintf(stderr, "uph vst3: component initialize failed\n");
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }
    ih->vst3.component_initialized = true;

    if (ih->vst3.component->lpVtbl->queryInterface(
            ih->vst3.component, Steinberg_Vst_IAudioProcessor_iid, (void**)&ih->vst3.processor) != Steinberg_kResultOk
        || !ih->vst3.processor)
    {
        ih->vst3.processor = NULL;
        fprintf(stderr, "uph vst3: component has no IAudioProcessor\n");
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }

    if (ih->vst3.component->lpVtbl->queryInterface(
            ih->vst3.component, Steinberg_Vst_IEditController_iid, (void**)&ih->vst3.controller) == Steinberg_kResultOk
        && ih->vst3.controller)
    {
        ih->vst3.controller_is_component = true;
    }
    else
    {
        ih->vst3.controller = NULL;
        Steinberg_TUID controller_cid;
        if (ih->vst3.component->lpVtbl->getControllerClassId(ih->vst3.component, controller_cid) == Steinberg_kResultOk)
        {
            if (ih->vst3.factory->lpVtbl->createInstance(
                    ih->vst3.factory, controller_cid, Steinberg_Vst_IEditController_iid, (void**)&ih->vst3.controller) == Steinberg_kResultOk
                && ih->vst3.controller)
            {
                if (((Steinberg_IPluginBase*)ih->vst3.controller)->lpVtbl->initialize(
                        ih->vst3.controller, (struct Steinberg_FUnknown*)&ih->vst3.host_app) != Steinberg_kResultOk)
                {
                    fprintf(stderr, "uph vst3: controller initialize failed\n");
                    ih->vst3.controller->lpVtbl->release(ih->vst3.controller);
                    ih->vst3.controller = NULL;
                }
                else
                    ih->vst3.controller_initialized = true;
            }
            else
                ih->vst3.controller = NULL;
        }
    }

    if (!ih->vst3.controller)
    {
        fprintf(stderr, "uph vst3: plug-in has no edit controller\n");
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }

    ih->vst3.controller->lpVtbl->setComponentHandler(
        ih->vst3.controller, (struct Steinberg_Vst_IComponentHandler*)&ih->vst3.handler);

    if (!ih->vst3.controller_is_component)
    {
        ih->vst3.component->lpVtbl->queryInterface(
            ih->vst3.component, Steinberg_Vst_IConnectionPoint_iid, (void**)&ih->vst3.component_cp);
        ih->vst3.controller->lpVtbl->queryInterface(
            ih->vst3.controller, Steinberg_Vst_IConnectionPoint_iid, (void**)&ih->vst3.controller_cp);

        if (ih->vst3.component_cp && ih->vst3.controller_cp)
        {
            ih->vst3.component_cp->lpVtbl->connect(ih->vst3.component_cp, ih->vst3.controller_cp);
            ih->vst3.controller_cp->lpVtbl->connect(ih->vst3.controller_cp, ih->vst3.component_cp);
            ih->vst3.connected = true;
        }

        Uph_Vst3MemStream st;
        uph_vst3_stream_init_write(&st);
        if (ih->vst3.component->lpVtbl->getState(ih->vst3.component, &st.iface) == Steinberg_kResultOk)
        {
            st.pos = 0;
            ih->vst3.controller->lpVtbl->setComponentState(ih->vst3.controller, &st.iface);
        }
        free(st.data);
    }

    Steinberg_int32 audio_in  = ih->vst3.component->lpVtbl->getBusCount(ih->vst3.component, Steinberg_Vst_MediaTypes_kAudio, Steinberg_Vst_BusDirections_kInput);
    Steinberg_int32 audio_out = ih->vst3.component->lpVtbl->getBusCount(ih->vst3.component, Steinberg_Vst_MediaTypes_kAudio, Steinberg_Vst_BusDirections_kOutput);
    Steinberg_int32 event_in  = ih->vst3.component->lpVtbl->getBusCount(ih->vst3.component, Steinberg_Vst_MediaTypes_kEvent, Steinberg_Vst_BusDirections_kInput);

    ih->vst3.audio_in_buses = audio_in;
    ih->vst3.audio_out_buses = audio_out;

    Steinberg_Vst_SpeakerArrangement stereo = Steinberg_Vst_SpeakerArr_kStereo;
    Steinberg_Vst_SpeakerArrangement in_arr[1] = { stereo };
    Steinberg_Vst_SpeakerArrangement out_arr[1] = { stereo };

    ih->vst3.processor->lpVtbl->setBusArrangements(
        ih->vst3.processor,
        audio_in  > 0 ? in_arr  : NULL, audio_in  > 0 ? 1 : 0,
        audio_out > 0 ? out_arr : NULL, audio_out > 0 ? 1 : 0);

    ih->vst3.in_channels = 0;
    ih->vst3.out_channels = 0;

    if (audio_in > 0)
    {
        Steinberg_Vst_SpeakerArrangement arr = 0;
        if (ih->vst3.processor->lpVtbl->getBusArrangement(ih->vst3.processor, Steinberg_Vst_BusDirections_kInput, 0, &arr) == Steinberg_kResultOk)
        {
            int32_t ch = 0;
            for (uint64_t bits = (uint64_t)arr; bits; bits >>= 1)
                ch += (int32_t)(bits & 1);
            ih->vst3.in_channels = ch > 0 ? ch : 2;
        }
        else
            ih->vst3.in_channels = 2;

        ih->vst3.component->lpVtbl->activateBus(ih->vst3.component, Steinberg_Vst_MediaTypes_kAudio, Steinberg_Vst_BusDirections_kInput, 0, 1);
    }

    if (audio_out > 0)
    {
        Steinberg_Vst_SpeakerArrangement arr = 0;
        if (ih->vst3.processor->lpVtbl->getBusArrangement(ih->vst3.processor, Steinberg_Vst_BusDirections_kOutput, 0, &arr) == Steinberg_kResultOk)
        {
            int32_t ch = 0;
            for (uint64_t bits = (uint64_t)arr; bits; bits >>= 1)
                ch += (int32_t)(bits & 1);
            ih->vst3.out_channels = ch > 0 ? ch : 2;
        }
        else
            ih->vst3.out_channels = 2;

        ih->vst3.component->lpVtbl->activateBus(ih->vst3.component, Steinberg_Vst_MediaTypes_kAudio, Steinberg_Vst_BusDirections_kOutput, 0, 1);
    }

    if (event_in > 0)
    {
        ih->vst3.component->lpVtbl->activateBus(ih->vst3.component, Steinberg_Vst_MediaTypes_kEvent, Steinberg_Vst_BusDirections_kInput, 0, 1);
        ih->vst3.has_event_input = true;
    }

    ih->vst3.view = ih->vst3.controller->lpVtbl->createView(ih->vst3.controller, Steinberg_Vst_ViewType_kEditor);
    if (!ih->vst3.view)
    {
        fprintf(stderr, "uph vst3: plug-in has no editor view\n");
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }

#if NAUI_LINUX
    const char *platform = Steinberg_kPlatformTypeX11EmbedWindowID;
#elif NAUI_WINDOWS
    const char *platform = Steinberg_kPlatformTypeHWND;
#endif

    if (ih->vst3.view->lpVtbl->isPlatformTypeSupported(ih->vst3.view, platform) != Steinberg_kResultTrue)
    {
        fprintf(stderr, "uph vst3: view doesn't support %s\n", platform);
        uph_vst3_free_partial(ih);
        free(ih);
        return;
    }

    struct Steinberg_ViewRect rect = {0};
    if (ih->vst3.view->lpVtbl->getSize(ih->vst3.view, &rect) == Steinberg_kResultOk)
    {
        *width  = (uint32_t)(rect.right - rect.left);
        *height = (uint32_t)(rect.bottom - rect.top);
    }
    else
    {
        *width = 800;
        *height = 600;
    }

    ih->vst3.width = *width;
    ih->vst3.height = *height;

    plug->internal_handle = ih;
    plug->params = uph_vst3_get_param_list(plug);
}

static inline void uph_assign_vst3_plugin_gui_internal(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;

    Steinberg_IPlugView *view = ih->vst3.view;
    view->lpVtbl->setFrame(view, (struct Steinberg_IPlugFrame*)&ih->vst3.frame);

#if NAUI_LINUX
    view->lpVtbl->attached(view, (void*)(uintptr_t)ih->window, Steinberg_kPlatformTypeX11EmbedWindowID);
#elif NAUI_WINDOWS
    view->lpVtbl->attached(view, (void*)ih->window, Steinberg_kPlatformTypeHWND);
#endif

    struct Steinberg_Vst_ProcessSetup setup = {
        .processMode = Steinberg_Vst_ProcessModes_kRealtime,
        .symbolicSampleSize = Steinberg_Vst_SymbolicSampleSizes_kSample32,
        .maxSamplesPerBlock = (Steinberg_int32)UPH_SAMPLE_FRAME_COUNT,
        .sampleRate = (double)uph_state.settings.audio.sample_rate
    };

    ih->vst3.processor->lpVtbl->setupProcessing(ih->vst3.processor, &setup);
    ih->vst3.component->lpVtbl->setActive(ih->vst3.component, 1);
    ih->vst3.processor->lpVtbl->setProcessing(ih->vst3.processor, 1);

    ih->vst3.sample_position = 0;
    ih->vst3.ready = 1;
}

static void uph_unload_vst3_plugin(Uph_PluginInternalHandle *ih)
{
    ih->vst3.ready = 0;

    if (ih->vst3.processor)
        ih->vst3.processor->lpVtbl->setProcessing(ih->vst3.processor, 0);
    if (ih->vst3.component)
        ih->vst3.component->lpVtbl->setActive(ih->vst3.component, 0);

    if (ih->vst3.view)
    {
        ih->vst3.view->lpVtbl->removed(ih->vst3.view);
        ih->vst3.view->lpVtbl->setFrame(ih->vst3.view, NULL);
    }

    uph_vst3_free_partial(ih);

#if NAUI_LINUX
    if (ih->display)
    {
        XDestroyWindow(ih->display, ih->window);
        XFlush(ih->display);
        XCloseDisplay(ih->display);
        ih->display = NULL;
    }
#elif NAUI_WINDOWS
    if (ih->window)
    {
        DestroyWindow(ih->window);
        ih->window = NULL;
    }
#endif
}

static void uph_vst3_process(
    Uph_PluginInternalHandle *ih,
    float **inputs,
    float **outputs,
    uint32_t frame_count,
    double playhead_beat,
    bool is_playing
)
{
    if (!ih->vst3.ready || !outputs)
        return;

    uint32_t note_snapshot  = uph_note_ring_size(&ih->vst3.pending_notes);
    uint32_t param_snapshot = uph_param_ring_size(&ih->vst3.pending_params);

    Uph_Vst3EventList *in_events = &ih->vst3.in_events;
    Uph_Vst3EventList *out_events = &ih->vst3.out_events;
    Uph_Vst3ParamChanges *in_params = &ih->vst3.in_params;
    Uph_Vst3ParamChanges *out_params = &ih->vst3.out_params;

    in_events->iface.lpVtbl = &uph_vst3_events_vtbl;
    in_events->count = 0;
    out_events->iface.lpVtbl = &uph_vst3_events_vtbl;
    out_events->count = 0;
    uph_vst3_param_changes_reset(in_params);
    uph_vst3_param_changes_reset(out_params);

    for (uint32_t i = 0; i < note_snapshot && in_events->count < UPH_VST3_MAX_EVENTS; i++)
    {
        const clap_event_note_t *n = uph_note_ring_peek(&ih->vst3.pending_notes, i);
        struct Steinberg_Vst_Event *e = &in_events->events[in_events->count++];
        memset(e, 0, sizeof(*e));

        e->busIndex = 0;
        e->sampleOffset = (Steinberg_int32)(n->header.time < frame_count ? n->header.time : (frame_count ? frame_count - 1 : 0));
        e->ppqPosition = 0;
        e->flags = Steinberg_Vst_Event_EventFlags_kIsLive;

        int key = n->key & 0x7F;

        if (n->header.type == CLAP_EVENT_NOTE_ON)
        {
            int32_t id = ih->vst3.next_note_id++;
            if (ih->vst3.next_note_id < 0) ih->vst3.next_note_id = 0;
            ih->vst3.note_ids[key] = id;

            e->type = Steinberg_Vst_Event_EventTypes_kNoteOnEvent;
            e->Steinberg_Vst_Event_noteOn.channel = (Steinberg_int16)(n->channel < 0 ? 0 : n->channel);
            e->Steinberg_Vst_Event_noteOn.pitch = (Steinberg_int16)key;
            e->Steinberg_Vst_Event_noteOn.velocity = (float)n->velocity;
            e->Steinberg_Vst_Event_noteOn.tuning = 0.0f;
            e->Steinberg_Vst_Event_noteOn.length = 0;
            e->Steinberg_Vst_Event_noteOn.noteId = id;
        }
        else
        {
            e->type = Steinberg_Vst_Event_EventTypes_kNoteOffEvent;
            e->Steinberg_Vst_Event_noteOff.channel = (Steinberg_int16)(n->channel < 0 ? 0 : n->channel);
            e->Steinberg_Vst_Event_noteOff.pitch = (Steinberg_int16)key;
            e->Steinberg_Vst_Event_noteOff.velocity = (float)n->velocity;
            e->Steinberg_Vst_Event_noteOff.noteId = ih->vst3.note_ids[key];
            e->Steinberg_Vst_Event_noteOff.tuning = 0.0f;
        }
    }

    for (uint32_t i = 0; i < param_snapshot; i++)
    {
        const clap_event_param_value_t *p = uph_param_ring_peek(&ih->vst3.pending_params, i);
        uph_vst3_param_changes_add(
            in_params,
            (Steinberg_Vst_ParamID)p->param_id,
            (int32_t)(p->header.time < frame_count ? p->header.time : (frame_count ? frame_count - 1 : 0)),
            p->value);
    }

    if (ih->vst3.controller)
    {
        for (int32_t i = 0; i < in_params->count; i++)
        {
            Uph_Vst3ParamQueue *q = &in_params->queues[i];
            if (q->count > 0)
                ih->vst3.controller->lpVtbl->setParamNormalized(ih->vst3.controller, q->id, q->values[q->count - 1]);
        }
    }

    struct Steinberg_Vst_ProcessContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = Steinberg_Vst_ProcessContext_StatesAndFlags_kTempoValid
        | Steinberg_Vst_ProcessContext_StatesAndFlags_kProjectTimeMusicValid
        | Steinberg_Vst_ProcessContext_StatesAndFlags_kTimeSigValid
        | (is_playing ? Steinberg_Vst_ProcessContext_StatesAndFlags_kPlaying : 0);
    ctx.sampleRate = (double)uph_state.settings.audio.sample_rate;
    ctx.projectTimeSamples = ih->vst3.sample_position;
    ctx.projectTimeMusic = playhead_beat;
    ctx.tempo = uph_state.project.bpm;
    ctx.timeSigNumerator = 4;
    ctx.timeSigDenominator = 4;

    bool pass_input = inputs && ih->vst3.audio_in_buses > 0;

    Steinberg_Vst_Sample32 *in_channels[2]  = { pass_input ? inputs[0]  : NULL, pass_input ? inputs[1]  : NULL };
    Steinberg_Vst_Sample32 *out_channels[2] = { outputs[0], outputs[1] };

    struct Steinberg_Vst_AudioBusBuffers in_bus = {0};
    in_bus.numChannels = ih->vst3.in_channels > 0 && ih->vst3.in_channels <= 2 ? ih->vst3.in_channels : 2;
    in_bus.silenceFlags = 0;
    in_bus.Steinberg_Vst_AudioBusBuffers_channelBuffers32 = in_channels;

    struct Steinberg_Vst_AudioBusBuffers out_bus = {0};
    out_bus.numChannels = ih->vst3.out_channels > 0 && ih->vst3.out_channels <= 2 ? ih->vst3.out_channels : 2;
    out_bus.silenceFlags = 0;
    out_bus.Steinberg_Vst_AudioBusBuffers_channelBuffers32 = out_channels;

    struct Steinberg_Vst_ProcessData data;
    memset(&data, 0, sizeof(data));
    data.processMode = Steinberg_Vst_ProcessModes_kRealtime;
    data.symbolicSampleSize = Steinberg_Vst_SymbolicSampleSizes_kSample32;
    data.numSamples = (Steinberg_int32)frame_count;
    data.numInputs  = pass_input ? 1 : 0;
    data.numOutputs = ih->vst3.audio_out_buses > 0 ? 1 : 0;
    data.inputs  = pass_input ? &in_bus : NULL;
    data.outputs = ih->vst3.audio_out_buses > 0 ? &out_bus : NULL;
    data.inputParameterChanges  = &in_params->iface;
    data.outputParameterChanges = &out_params->iface;
    data.inputEvents  = ih->vst3.has_event_input ? &in_events->iface : NULL;
    data.outputEvents = &out_events->iface;
    data.processContext = &ctx;

    ih->vst3.processor->lpVtbl->process(ih->vst3.processor, &data);

    ih->vst3.sample_position += (int64_t)frame_count;

    uph_note_ring_advance(&ih->vst3.pending_notes, note_snapshot);
    uph_param_ring_advance(&ih->vst3.pending_params, param_snapshot);
}

static void uph_vst3_pump_run_loop(Uph_PluginInternalHandle *ih)
{
    Uph_Vst3RunLoop *rl = &ih->vst3.run_loop;

#if NAUI_LINUX
    struct pollfd pfds[UPH_VST3_MAX_FD_HANDLERS];
    int idx[UPH_VST3_MAX_FD_HANDLERS];
    int n = 0;

    for (int i = 0; i < UPH_VST3_MAX_FD_HANDLERS; i++)
    {
        if (!rl->fds[i].handler)
            continue;
        pfds[n].fd = rl->fds[i].fd;
        pfds[n].events = POLLIN;
        pfds[n].revents = 0;
        idx[n] = i;
        n++;
    }

    if (n > 0 && poll(pfds, (nfds_t)n, 0) > 0)
    {
        for (int k = 0; k < n; k++)
        {
            if (!(pfds[k].revents & (POLLIN | POLLERR | POLLHUP)))
                continue;
            Uph_Vst3FdHandler *h = &rl->fds[idx[k]];
            if (h->handler)
                h->handler->lpVtbl->onFDIsSet(h->handler, h->fd);
        }
    }
#endif

    float now = naui_frame_time();
    for (int i = 0; i < UPH_VST3_MAX_TIMERS; i++)
    {
        Uph_Vst3Timer *t = &rl->timers[i];
        if (!t->handler)
            continue;
        if ((now - t->last_fire) * 1000.0f >= (float)t->period_ms)
        {
            t->last_fire = now;
            t->handler->lpVtbl->onTimer(t->handler);
        }
    }
}

static bool uph_vst3_save_state(Uph_PluginInternalHandle *ih, const Naui_Path path)
{
    Uph_Vst3MemStream comp, ctrl;
    uph_vst3_stream_init_write(&comp);
    uph_vst3_stream_init_write(&ctrl);

    bool ok = ih->vst3.component->lpVtbl->getState(ih->vst3.component, &comp.iface) == Steinberg_kResultOk;

    if (ok && !ih->vst3.controller_is_component)
        ih->vst3.controller->lpVtbl->getState(ih->vst3.controller, &ctrl.iface);

    if (ok)
    {
        size_t total = 8 + (size_t)comp.size + (size_t)ctrl.size;
        uint8_t *buf = (uint8_t*)malloc(total);
        if (buf)
        {
            uint32_t cs = (uint32_t)comp.size, ks = (uint32_t)ctrl.size;
            memcpy(buf, &cs, 4);
            if (cs) memcpy(buf + 4, comp.data, cs);
            memcpy(buf + 4 + cs, &ks, 4);
            if (ks) memcpy(buf + 8 + cs, ctrl.data, ks);

            ok = naui_file_write_all(path, buf, total);
            free(buf);
        }
        else ok = false;
    }

    free(comp.data);
    free(ctrl.data);
    return ok;
}

static bool uph_vst3_load_state(Uph_PluginInternalHandle *ih, const Naui_Path path)
{
    size_t size = 0;
    uint8_t *data = (uint8_t*)naui_file_read_all(path, &size);
    if (!data)
    {
        fprintf(stderr, "uph_plugin_load_state: failed to read %s\n", path.data);
        return false;
    }

    bool ok = false;
    if (size >= 8)
    {
        uint32_t cs = 0, ks = 0;
        memcpy(&cs, data, 4);
        if ((size_t)cs + 8 <= size)
        {
            memcpy(&ks, data + 4 + cs, 4);
            if ((size_t)cs + (size_t)ks + 8 <= size)
            {
                Uph_Vst3MemStream comp;
                uph_vst3_stream_init_read(&comp, data + 4, cs);
                ok = ih->vst3.component->lpVtbl->setState(ih->vst3.component, &comp.iface) == Steinberg_kResultOk;

                comp.pos = 0;
                ih->vst3.controller->lpVtbl->setComponentState(ih->vst3.controller, &comp.iface);

                if (ks && !ih->vst3.controller_is_component)
                {
                    Uph_Vst3MemStream ctrl;
                    uph_vst3_stream_init_read(&ctrl, data + 8 + cs, ks);
                    ih->vst3.controller->lpVtbl->setState(ih->vst3.controller, &ctrl.iface);
                }
            }
        }
    }

    free(data);
    return ok;
}
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

    Uph_NoteEventRing *note_ring = plug->format == UPH_PLUGIN_VST3
        ? &internal_handle->vst3.pending_notes
        : &internal_handle->clap.pending_notes;
    bool *active_notes = plug->format == UPH_PLUGIN_VST3
        ? internal_handle->vst3.active_notes
        : internal_handle->clap.active_notes;
    int16_t *active_channels = plug->format == UPH_PLUGIN_VST3
        ? internal_handle->vst3.active_note_channels
        : internal_handle->clap.active_note_channels;

    if (!uph_note_ring_push(note_ring, &ev))
    {
        fprintf(stderr, "uph_plugin_queue_note_event: note ring full, dropping event (key=%u on=%d)\n",
                key, (int)note_on);
        return;
    }

    active_notes[key] = note_on;
    if (note_on)
        active_channels[key] = channel;
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

    Uph_ParamEventRing *param_ring = plug->format == UPH_PLUGIN_VST3
        ? &ih->vst3.pending_params
        : &ih->clap.pending_params;

    if (!uph_param_ring_push(param_ring, &ev))
    {
        fprintf(stderr, "uph_plugin_queue_param_change: param ring full, dropping event (param_id=%u)\n",
                (unsigned)param_id);
    }
}

void uph_plugin_queue_stop_all(Uph_Plugin *plug, uint32_t sample_offset)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    bool *active_notes = plug->format == UPH_PLUGIN_VST3
        ? internal_handle->vst3.active_notes
        : internal_handle->clap.active_notes;
    int16_t *active_channels = plug->format == UPH_PLUGIN_VST3
        ? internal_handle->vst3.active_note_channels
        : internal_handle->clap.active_note_channels;
    Uph_NoteEventRing *note_ring = plug->format == UPH_PLUGIN_VST3
        ? &internal_handle->vst3.pending_notes
        : &internal_handle->clap.pending_notes;

    for (int key = 0; key < 128; key++)
    {
        if (!active_notes[key])
            continue;

        clap_event_note_t ev = {0};
        ev.header.size = sizeof(clap_event_note_t);
        ev.header.time = sample_offset;
        ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev.header.type = CLAP_EVENT_NOTE_OFF;
        ev.header.flags = 0;

        ev.note_id = -1;
        ev.port_index = 0;
        ev.channel = active_channels[key];
        ev.key = (int16_t)key;
        ev.velocity = 0.0;

        if (!uph_note_ring_push(note_ring, &ev))
        {
            fprintf(stderr, "uph_plugin_queue_stop_all: note ring full, dropping off for key %d\n", key);
            continue;
        }

        active_notes[key] = false;
    }
}

bool uph_plugin_note_active(Uph_Plugin *plug, uint8_t key)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;
    return plug->format == UPH_PLUGIN_VST3
        ? internal_handle->vst3.active_notes[key]
        : internal_handle->clap.active_notes[key];
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
    Uph_PluginInternalHandle *ih =
        (Uph_PluginInternalHandle*)(uintptr_t)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg)
    {
        case WM_NCLBUTTONDOWN:
            if (ih && wparam == HTCAPTION)
            {
                GetCursorPos(&ih->drag_start_cursor);

                RECT r;
                GetWindowRect(hwnd, &r);
                ih->drag_start_origin.x = r.left;
                ih->drag_start_origin.y = r.top;

                ih->dragging = true;
                SetCapture(hwnd);

                SetForegroundWindow(hwnd);
                return 0;
            }
            break;

        case WM_MOUSEMOVE:
            if (ih && ih->dragging)
            {
                POINT p;
                GetCursorPos(&p);

                SetWindowPos(
                    hwnd, NULL,
                    ih->drag_start_origin.x + (p.x - ih->drag_start_cursor.x),
                    ih->drag_start_origin.y + (p.y - ih->drag_start_cursor.y),
                    0, 0,
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
                );
                return 0;
            }
            break;

        case WM_LBUTTONUP:
            if (ih && ih->dragging)
            {
                ih->dragging = false;
                ReleaseCapture();
                return 0;
            }
            break;

        case WM_CAPTURECHANGED:
            if (ih)
                ih->dragging = false;
            break;

        case WM_KEYDOWN:
            if (ih && ih->dragging && wparam == VK_ESCAPE)
            {
                SetWindowPos(
                    hwnd, NULL,
                    ih->drag_start_origin.x, ih->drag_start_origin.y,
                    0, 0,
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
                );
                ih->dragging = false;
                ReleaseCapture();
                return 0;
            }
            break;

        case WM_CLOSE:
            if (ih)
            {
                ih->dragging = false;
                if (ih->visible)
                {
                    ih->visible = false;
                    if (ih->clap.gui && ih->clap.plugin && ih->is_clap)
                        ih->clap.gui->hide(ih->clap.plugin);
                    ShowWindow(hwnd, SW_HIDE);
                }
            }
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
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

static void uph_clap_abort_load(void *handle, const clap_plugin_entry_t *entry, bool entry_inited, Uph_PluginInternalHandle *ih, const clap_plugin_t *plugin, bool plugin_inited)
{
    if (plugin)
        plugin->destroy(plugin);

    if (ih)
        free(ih);

    if (entry && entry_inited)
        entry->deinit();

#if NAUI_LINUX
    if (handle)
        dlclose(handle);
#elif NAUI_WINDOWS
    if (handle)
        FreeLibrary((HMODULE)handle);
#endif
}

static inline void uph_load_clap_plugin_internal(Uph_Plugin *plug, uint32_t *width, uint32_t *height)
{
    uph_com_init_sta_once();

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
        uph_clap_abort_load((void*)handle, entry, false, NULL, NULL, false);
        return;
    }

    const clap_plugin_factory_t *factory =
        (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

    if (!factory)
    {
        fprintf(stderr, "no plugin factory\n");
        uph_clap_abort_load((void*)handle, entry, true, NULL, NULL, false);
        return;
    }

    uint32_t count = factory->get_plugin_count(factory);
    if (count == 0)
    {
        fprintf(stderr, "no plugins in this bundle\n");
        uph_clap_abort_load((void*)handle, entry, true, NULL, NULL, false);
        return;
    }

    const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
    fprintf(stderr, "loading plugin: %s (%s)\n", desc->name, desc->id);

    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)calloc(1, sizeof(Uph_PluginInternalHandle));

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
        uph_clap_abort_load((void*)handle, entry, true, internal_handle, NULL, false);
        return;
    }
    if (!plugin->init(plugin))
    {
        fprintf(stderr, "plugin init failed\n");
        uph_clap_abort_load((void*)handle, entry, true, internal_handle, plugin, false);
        return;
    }

    const clap_plugin_gui_t *gui =
        (const clap_plugin_gui_t *)plugin->get_extension(plugin, CLAP_EXT_GUI);

    if (!gui)
    {
        fprintf(stderr, "gui init failed\n");
        uph_clap_abort_load((void*)handle, entry, true, internal_handle, plugin, true);
        return;
    }

#if NAUI_LINUX
    if (!gui->is_api_supported(plugin, CLAP_WINDOW_API_X11, false))
    {
        fprintf(stderr, "gui not supported on X11\n");
        uph_clap_abort_load((void*)handle, entry, true, internal_handle, plugin, true);
        return;
    }
    gui->create(plugin, CLAP_WINDOW_API_X11, false);
#elif NAUI_WINDOWS
    if (!gui->is_api_supported(plugin, CLAP_WINDOW_API_WIN32, false))
    {
        fprintf(stderr, "gui not supported on Win32\n");
        uph_clap_abort_load((void*)handle, entry, true, internal_handle, plugin, true);
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
    internal_handle->clap.entry = entry;
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
    internal_handle->clap.ready = 1;
}

Uph_Plugin uph_load_plugin(Naui_Path path)
{
    Uph_Plugin effect = { 0 };

    uph_com_init_sta_once();

    const Naui_StringView extension = naui_file_extension(&path);
    if (naui_string_view_equals_cstr(extension, ".clap", false))
        effect.format = UPH_PLUGIN_CLAP;
    else if (naui_string_view_equals_cstr(extension, ".vst3", false))
        effect.format = UPH_PLUGIN_VST3;

    effect.file_path = path;

    uint32_t width = 0, height = 0;
    if (effect.format == UPH_PLUGIN_CLAP)
        uph_load_clap_plugin_internal(&effect, &width, &height);
    else if (effect.format == UPH_PLUGIN_VST3)
        uph_load_vst3_plugin_internal(&effect, &width, &height);

    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)effect.internal_handle;
    if (!internal_handle)
        return effect;

    internal_handle->is_clap = (effect.format == UPH_PLUGIN_CLAP);

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

    SetWindowLongPtrA(child, GWLP_USERDATA, (LONG_PTR)internal_handle);

    ShowWindow(child, SW_SHOW);
    UpdateWindow(child);

    internal_handle->window = child;
    internal_handle->visible = true;
#endif

    if (effect.format == UPH_PLUGIN_CLAP)
        uph_assign_clap_plugin_gui_internal(&effect);
    else if (effect.format == UPH_PLUGIN_VST3)
        uph_assign_vst3_plugin_gui_internal(&effect);

    effect.loaded = true;

    return effect;
}

static void uph_unload_clap_plugin(Uph_PluginInternalHandle *internal_handle)
{
    internal_handle->clap.ready = 0;

    const clap_plugin_t *plugin = internal_handle->clap.plugin;
    if (plugin)
    {
        plugin->stop_processing(plugin);
        plugin->deactivate(plugin);

        if (internal_handle->clap.gui)
            internal_handle->clap.gui->destroy(plugin);

        plugin->destroy(plugin);
        internal_handle->clap.plugin = NULL;
    }

#if NAUI_LINUX
    if (internal_handle->display)
    {
        XDestroyWindow(internal_handle->display, internal_handle->window);
        XFlush(internal_handle->display);
        XCloseDisplay(internal_handle->display);
        internal_handle->display = NULL;
    }
#elif NAUI_WINDOWS
    if (internal_handle->window)
    {
        DestroyWindow(internal_handle->window);
        internal_handle->window = NULL;
    }

    if (internal_handle->clap.entry)
        internal_handle->clap.entry->deinit();

    if (internal_handle->clap.library_handle)
        FreeLibrary((HMODULE)internal_handle->clap.library_handle);
#endif
}

void uph_unload_plugin(Uph_Plugin *plug)
{
    if (!plug->loaded)
        return;

    plug->loaded = false;

    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!internal_handle)
        return;

    if (plug->format == UPH_PLUGIN_CLAP)
        uph_unload_clap_plugin(internal_handle);
    else if (plug->format == UPH_PLUGIN_VST3)
        uph_unload_vst3_plugin(internal_handle);

    free(plug->internal_handle);
	naui_list_free(plug->params);

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

    if (plug->format == UPH_PLUGIN_CLAP && internal_handle->clap.gui)
        internal_handle->clap.gui->hide(internal_handle->clap.plugin);

    XUnmapWindow(internal_handle->display, internal_handle->window);
    XFlush(internal_handle->display);
#elif NAUI_WINDOWS
    if (plug->format == UPH_PLUGIN_VST3)
    {
        if (internal_handle->visible)
        {
            internal_handle->visible = false;
            ShowWindow(internal_handle->window, SW_HIDE);
        }
    }
    else
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

    if (plug->format == UPH_PLUGIN_CLAP)
    {
        if (internal_handle->clap.gui)
            internal_handle->clap.gui->show(internal_handle->clap.plugin);
    }

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

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
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

    if (plug->format == UPH_PLUGIN_VST3)
    {
        uph_vst3_pump_run_loop(internal_handle);
        return;
    }

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

static bool uph_clap_out_events_try_push(const clap_output_events_t *list, const clap_event_header_t *event)
{
    (void)list; (void)event;
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

    if (!internal_handle)
        return;

    if (plug->format == UPH_PLUGIN_VST3)
    {
        uph_vst3_process(internal_handle, inputs, outputs, frame_count, playhead_beat, is_playing);
        return;
    }

    if (!internal_handle->clap.ready || !internal_handle->clap.plugin)
        return;

    uint32_t note_snapshot  = uph_note_ring_size(&internal_handle->clap.pending_notes);
    uint32_t param_snapshot = uph_param_ring_size(&internal_handle->clap.pending_params);

    Uph_ClapInEvents in_events = {
        .iface = {
            .ctx = NULL,
            .size = uph_clap_in_events_size,
            .get = uph_clap_in_events_get
        },
        .note_ring = &internal_handle->clap.pending_notes,
        .param_ring = &internal_handle->clap.pending_params,
        .note_count_snapshot = note_snapshot,
        .param_count_snapshot = param_snapshot,
    };

    clap_output_events_t out_iface = {
        .try_push = uph_clap_out_events_try_push
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

typedef struct
{
    clap_ostream_t iface;
    uint8_t *data;
    size_t size;
    size_t capacity;
}
Uph_ClapSaveStream;

static int64_t uph_clap_stream_write(const clap_ostream_t *stream, const void *buffer, uint64_t size)
{
    Uph_ClapSaveStream *self = (Uph_ClapSaveStream*)stream;

    if (self->size + size > self->capacity)
    {
        size_t new_capacity = self->capacity ? self->capacity * 2 : 4096;
        while (new_capacity < self->size + size)
            new_capacity *= 2;

        uint8_t *new_data = (uint8_t*)realloc(self->data, new_capacity);
        if (!new_data)
            return -1;

        self->data = new_data;
        self->capacity = new_capacity;
    }

    memcpy(self->data + self->size, buffer, size);
    self->size += size;
    return (int64_t)size;
}

typedef struct
{
    clap_istream_t iface;
    const uint8_t *data;
    size_t size;
    size_t pos;
}
Uph_ClapLoadStream;

static int64_t uph_clap_stream_read(const clap_istream_t *stream, void *buffer, uint64_t size)
{
    Uph_ClapLoadStream *self = (Uph_ClapLoadStream*)stream;

    size_t remaining = self->size - self->pos;
    size_t to_read = size < remaining ? (size_t)size : remaining;

    memcpy(buffer, self->data + self->pos, to_read);
    self->pos += to_read;
    return (int64_t)to_read;
}

bool uph_plugin_save_state(Uph_Plugin *plug, const Naui_Path path)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (internal_handle && plug->format == UPH_PLUGIN_VST3)
        return uph_vst3_save_state(internal_handle, path);

    if (!internal_handle || !internal_handle->clap.plugin)
        return false;

    const clap_plugin_state_t *state =
        (const clap_plugin_state_t*)internal_handle->clap.plugin->get_extension(
            internal_handle->clap.plugin, CLAP_EXT_STATE);

    if (!state)
    {
        fprintf(stderr, "uph_plugin_save_state: plugin has no state extension\n");
        return false;
    }

    Uph_ClapSaveStream stream = {
        .iface = { .ctx = &stream, .write = uph_clap_stream_write },
        .data = NULL,
        .size = 0,
        .capacity = 0
    };

    bool ok = state->save(internal_handle->clap.plugin, &stream.iface);
    if (ok)
        ok = naui_file_write_all(path, stream.data, stream.size);

    free(stream.data);
    return ok;
}

bool uph_plugin_load_state(Uph_Plugin *plug, const Naui_Path path)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (internal_handle && plug->format == UPH_PLUGIN_VST3)
    {
        bool ok = uph_vst3_load_state(internal_handle, path);
        if (ok)
            plug->params = uph_vst3_get_param_list(plug);
        return ok;
    }

    if (!internal_handle || !internal_handle->clap.plugin)
        return false;

    const clap_plugin_state_t *state =
        (const clap_plugin_state_t*)internal_handle->clap.plugin->get_extension(
            internal_handle->clap.plugin, CLAP_EXT_STATE);

    if (!state)
    {
        fprintf(stderr, "uph_plugin_load_state: plugin has no state extension\n");
        return false;
    }

    size_t size = 0;
    char *data = naui_file_read_all(path, &size);
    if (!data)
    {
        fprintf(stderr, "uph_plugin_load_state: failed to read %s\n", path.data);
        return false;
    }

    Uph_ClapLoadStream stream = {
        .iface = { .ctx = &stream, .read = uph_clap_stream_read },
        .data = (const uint8_t*)data,
        .size = size,
        .pos = 0
    };

    bool ok = state->load(internal_handle->clap.plugin, &stream.iface);

    free(data);

    if (ok)
        plug->params = uph_clap_get_param_list(plug);

    return ok;
}

static bool uph_clap_features_have(const char *const *features, const char *needle)
{
    if (!features)
        return false;
    for (; *features; features++)
        if (strcmp(*features, needle) == 0)
            return true;
    return false;
}

static bool uph_get_clap_plugin_info(Naui_Path path, Uph_PluginInfo *out)
{
    uph_com_init_sta_once();

#if NAUI_LINUX
    void *handle = dlopen(path.data, RTLD_LOCAL | RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "uph info: dlopen failed: %s\n", dlerror());
        return false;
    }

    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t*)dlsym(handle, "clap_entry");
    if (!entry)
    {
        fprintf(stderr, "uph info: no clap_entry symbol\n");
        dlclose(handle);
        return false;
    }
#elif NAUI_WINDOWS
    HMODULE handle = LoadLibraryA(path.data);
    if (!handle)
    {
        fprintf(stderr, "uph info: LoadLibraryA failed: %lu\n", GetLastError());
        return false;
    }

    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t*)GetProcAddress(handle, "clap_entry");
    if (!entry)
    {
        fprintf(stderr, "uph info: no clap_entry symbol\n");
        FreeLibrary(handle);
        return false;
    }
#endif

    bool ok = false;

    if (entry->init(path.data))
    {
        const clap_plugin_factory_t *factory =
            (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

        if (factory && factory->get_plugin_count(factory) > 0)
        {
            const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
            if (desc)
            {
                out->name = naui_string_from_cstr(desc->name ? desc->name : "");
                out->vendor = naui_string_from_cstr(desc->vendor ? desc->vendor : "");
                out->format = UPH_PLUGIN_CLAP;
                out->type = uph_clap_features_have(desc->features, CLAP_PLUGIN_FEATURE_INSTRUMENT)
                    ? UPH_PLUGIN_INSTRUMENT
                    : UPH_PLUGIN_EFFECT;
                ok = true;
            }
        }

        entry->deinit();
    }

#if NAUI_LINUX
    dlclose(handle);
#elif NAUI_WINDOWS
    FreeLibrary(handle);
#endif

    return ok;
}

static bool uph_get_vst3_plugin_info_guarded(Naui_Path path, Uph_PluginInfo *out)
{
    char binary[1024];
    if (!uph_vst3_resolve_binary(path.data, binary, sizeof(binary)))
    {
        fprintf(stderr, "uph info: could not resolve binary for %s\n", path.data);
        return false;
    }

    void *lib = NULL;
    Uph_Vst3GetFactoryFn get_factory = NULL;

#if NAUI_LINUX
    lib = dlopen(binary, RTLD_LOCAL | RTLD_LAZY);
    if (!lib)
    {
        fprintf(stderr, "uph info: dlopen failed: %s\n", dlerror());
        return false;
    }

    Uph_Vst3ModuleEntryFn entry = (Uph_Vst3ModuleEntryFn)dlsym(lib, "ModuleEntry");
    if (entry) entry(lib);
    get_factory = (Uph_Vst3GetFactoryFn)dlsym(lib, "GetPluginFactory");
#elif NAUI_WINDOWS
    lib = (void*)LoadLibraryA(binary);
    if (!lib)
    {
        fprintf(stderr, "uph info: LoadLibraryA failed: %lu\n", GetLastError());
        return false;
    }

    Uph_Vst3InitDllFn init_dll = (Uph_Vst3InitDllFn)GetProcAddress((HMODULE)lib, "InitDll");
    if (init_dll) init_dll();
    get_factory = (Uph_Vst3GetFactoryFn)GetProcAddress((HMODULE)lib, "GetPluginFactory");
#endif

    bool ok = false;
    Steinberg_IPluginFactory *factory = get_factory ? get_factory() : NULL;

    if (factory)
    {
        struct Steinberg_PFactoryInfo factory_info;
        memset(&factory_info, 0, sizeof(factory_info));
        factory->lpVtbl->getFactoryInfo(factory, &factory_info);

        Steinberg_IPluginFactory2 *factory2 = NULL;
        factory->lpVtbl->queryInterface(factory, Steinberg_IPluginFactory2_iid, (void**)&factory2);

        Steinberg_int32 class_count = factory->lpVtbl->countClasses(factory);
        for (Steinberg_int32 i = 0; i < class_count; i++)
        {
            struct Steinberg_PClassInfo class_info;
            memset(&class_info, 0, sizeof(class_info));
            if (factory->lpVtbl->getClassInfo(factory, i, &class_info) != Steinberg_kResultOk)
                continue;
            if (strcmp(class_info.category, "Audio Module Class") != 0)
                continue;

            const char *vendor = factory_info.vendor;
            bool is_instrument = false;

            if (factory2)
            {
                struct Steinberg_PClassInfo2 info2;
                memset(&info2, 0, sizeof(info2));
                if (factory2->lpVtbl->getClassInfo2(factory2, i, &info2) == Steinberg_kResultOk)
                {
                    if (info2.vendor[0])
                        vendor = info2.vendor;

                    is_instrument = strstr(info2.subCategories, "Instrument") != NULL;
                }
            }

            out->name = naui_string_from_cstr(class_info.name);
            out->vendor = naui_string_from_cstr(vendor);
            out->format = UPH_PLUGIN_VST3;
            out->type = is_instrument ? UPH_PLUGIN_INSTRUMENT : UPH_PLUGIN_EFFECT;
            ok = true;
            break;
        }

        if (factory2)
            factory2->lpVtbl->release(factory2);
        factory->lpVtbl->release(factory);
    }
    else
        fprintf(stderr, "uph info: no usable plugin factory in %s\n", path.data);

#if NAUI_LINUX
    Uph_Vst3ModuleExitFn exit_fn = (Uph_Vst3ModuleExitFn)dlsym(lib, "ModuleExit");
    if (exit_fn) exit_fn();
    dlclose(lib);
#elif NAUI_WINDOWS
    Uph_Vst3ExitDllFn exit_dll = (Uph_Vst3ExitDllFn)GetProcAddress((HMODULE)lib, "ExitDll");
    if (exit_dll) exit_dll();
    FreeLibrary((HMODULE)lib);
#endif

    return ok;
}

#if NAUI_WINDOWS && defined(_MSC_VER)
static int uph_seh_filter(unsigned int code)
{
    (void)code;
    return EXCEPTION_EXECUTE_HANDLER;
}

static bool uph_get_vst3_plugin_info(Naui_Path path, Uph_PluginInfo *out)
{
    bool ok = false;
    __try
    {
        ok = uph_get_vst3_plugin_info_guarded(path, out);
    }
    __except (uph_seh_filter(GetExceptionCode()))
    {
        fprintf(stderr, "uph info: plugin faulted while scanning %s\n", path.data);
        ok = false;
    }
    return ok;
}
#else
static bool uph_get_vst3_plugin_info(Naui_Path path, Uph_PluginInfo *out)
{
    uph_com_init_sta_once();
    return uph_get_vst3_plugin_info_guarded(path, out);
}
#endif

bool uph_get_plugin_info(Naui_Path path, Uph_PluginInfo *out)
{
    memset(out, 0, sizeof(*out));

    uph_com_init_sta_once();

    const Naui_StringView extension = naui_file_extension(&path);
    if (naui_string_view_equals_cstr(extension, ".clap", false))
        return uph_get_clap_plugin_info(path, out);
    if (naui_string_view_equals_cstr(extension, ".vst3", false))
        return uph_get_vst3_plugin_info(path, out);

    fprintf(stderr, "uph info: unrecognized plugin extension for %s\n", path.data);
    return false;
}