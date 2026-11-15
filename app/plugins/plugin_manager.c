typedef uint8_t Uph_PluginType;
enum
{
	UPH_PLUGIN_INSTRUMENT,
	UPH_PLUGIN_EFFECT
};

typedef struct
{
	Naui_Path path;
	Naui_String name;
	Naui_String vendor;
	Uph_PluginType type;
	Uph_PluginFormat format;
}
Uph_PluginInfo;

#if NAUI_LINUX
#include <X11/Xlibint.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/shape.h>
#endif

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

#define UPH_VST2_MAX_EVENTS     (UPH_NOTE_RING_CAPACITY)
#define UPH_VST2_MAX_BLOCK      4096
#define UPH_VST2_MAX_CHANNELS   32
#define UPH_VST2_HOST_VERSION   2400

#define UPH_VST2_EFF_GET_CHUNK          23
#define UPH_VST2_EFF_SET_CHUNK          24
#define UPH_VST2_FLAG_PROGRAM_CHUNKS    (1 << 5)

typedef struct
{
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
}
Uph_Vst2Rect;

typedef struct
{
    int num_events;
    void *reserved;
    VstEvent *events[UPH_VST2_MAX_EVENTS];
}
Uph_Vst2Events;

typedef AEffect *(__cdecl *Uph_Vst2MainFn)(audioMasterCallback);

typedef struct
{
    uint8_t *pixels;
    uint32_t width, height;
}
Uph_PluginGuiFrame;

typedef struct
{
    Naui_Thread thread;
    Naui_Mutex lock;
    Naui_Cond wake;
    bool stop;
    bool requested;
    bool fresh;
    bool finished;
    uint32_t width, height;
#if NAUI_LINUX
    Window window;
    Display *display;
#elif NAUI_WINDOWS
    HWND window;
    HDC dc;
    HBITMAP bitmap;
    void *bits;
    uint32_t bitmap_width, bitmap_height;
#endif
    Uph_PluginGuiFrame back;
    Uph_PluginGuiFrame ready;
    Uph_PluginGuiFrame front;
}
Uph_PluginGuiCapture;

struct Uph_PluginInternalHandle_
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
            uint8_t cc_values[128];

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
            uint8_t cc_values[128];
            int64_t cc_param_ids[128];

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

        struct
        {
            AEffect *effect;
            void *library_handle;

            Uph_NoteEventRing pending_notes;
            Uph_ParamEventRing pending_params;

            bool active_notes[128];
            int16_t active_note_channels[128];
            uint8_t cc_values[128];

            Uph_Vst2Events events;
            VstMidiEvent midi_events[UPH_VST2_MAX_EVENTS];
            VstTimeInfo time_info;

            float *buffers;
            float *in_ptrs[UPH_VST2_MAX_CHANNELS];
            float *out_ptrs[UPH_VST2_MAX_CHANNELS];
            int32_t in_channels;
            int32_t out_channels;

            int64_t sample_position;
            bool was_playing;
            bool editor_open;
            volatile int ready;
        }
        vst2;
    };

#if NAUI_LINUX
    Window window;
    Window container;
    Window input_target;
    Display *display;
    uint8_t buttons_down;
#elif NAUI_WINDOWS
    HWND window;
    POINT cursor;
    bool native_input;
#endif
    Naui_PanelID panel;
    Naui_Texture texture;
    Uph_PluginGuiCapture capture;
    uint32_t gui_width, gui_height;
    uint32_t texture_width, texture_height;
    int32_t screen_x, screen_y;
    bool origin_dirty;
    bool visible;
    bool is_clap;
    Naui_String display_name;
    atomic_uint_least64_t touched_param;
};

static void _uph_plugin_mark_touched(Uph_PluginInternalHandle *ih, uint64_t param_id)
{
    if (ih)
        atomic_store_explicit(&ih->touched_param, param_id + 1, memory_order_relaxed);
}

bool uph_plugin_take_touched_param(Uph_Plugin *plug, uint64_t *out_param_id)
{
    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!ih)
        return false;

    const uint64_t touched = atomic_exchange_explicit(&ih->touched_param, 0, memory_order_relaxed);
    if (touched == 0)
        return false;

    *out_param_id = touched - 1;
    return true;
}

static void _uph_plugin_gui_release_capture(Uph_PluginInternalHandle *ih);

static void _uph_plugin_gui_resize_window(Uph_PluginInternalHandle *ih, uint32_t width, uint32_t height)
{
    ih->gui_width = width;
    ih->gui_height = height;

#if NAUI_LINUX
    XResizeWindow(ih->display, ih->window, width, height);
    XFlush(ih->display);
#elif NAUI_WINDOWS
    SetWindowPos(ih->window, NULL, 0, 0, (int)width, (int)height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

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
    Uph_Vst3Handler *h = (Uph_Vst3Handler*)self;
    _uph_plugin_mark_touched(h->owner, id);
    return Steinberg_kResultOk;
}

static Steinberg_tresult SMTG_STDMETHODCALLTYPE uph_vst3_handler_perform_edit(void *self, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue value)
{
    Uph_Vst3Handler *h = (Uph_Vst3Handler*)self;
    if (!h->owner)
        return Steinberg_kResultOk;

    _uph_plugin_mark_touched(h->owner, id);

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

    _uph_plugin_gui_resize_window(f->owner, w, h);

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
static Naui_String uph_vst3_string128_to_string(const Steinberg_Vst_String128 src)
{
    Naui_String result = { 0 };
    for (size_t i = 0; i < 128 && src[i]; i++)
    {
        uint32_t c = (uint16_t)src[i];

        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < 128 && (uint16_t)src[i + 1] >= 0xDC00 && (uint16_t)src[i + 1] <= 0xDFFF)
        {
            c = 0x10000 + ((c - 0xD800) << 10) + ((uint16_t)src[i + 1] - 0xDC00);
            i++;
        }

        if (c >= 0xD800 && c <= 0xDFFF)
            c = 0xFFFD;

        if (!naui_string_append_codepoint(&result, c))
            break;
    }
    return result;
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

        Uph_PluginParam p = {
            .id = info.id,
            .name = uph_vst3_string128_to_string(info.title),
            .module = uph_vst3_string128_to_string(info.units),
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
    lib = dlopen(binary, RTLD_LOCAL | RTLD_LAZY | RTLD_NODELETE);
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

    const uint32_t max_frame_count = 2048;
    struct Steinberg_Vst_ProcessSetup setup = {
        .processMode = Steinberg_Vst_ProcessModes_kRealtime,
        .symbolicSampleSize = Steinberg_Vst_SymbolicSampleSizes_kSample32,
        .maxSamplesPerBlock = (Steinberg_int32)max_frame_count,
        .sampleRate = (double)uph_state.settings.audio.sample_rate
    };

    ih->vst3.processor->lpVtbl->setupProcessing(ih->vst3.processor, &setup);
    ih->vst3.component->lpVtbl->setActive(ih->vst3.component, 1);
    ih->vst3.processor->lpVtbl->setProcessing(ih->vst3.processor, 1);

    for (int cc = 0; cc < 128; cc++)
        ih->vst3.cc_param_ids[cc] = -1;

    Steinberg_Vst_IMidiMapping *midi_mapping = NULL;
    if (ih->vst3.controller->lpVtbl->queryInterface(ih->vst3.controller, Steinberg_Vst_IMidiMapping_iid, (void**)&midi_mapping) == Steinberg_kResultOk && midi_mapping)
    {
        for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
        {
            const uint8_t cc = uph_midi_pedal_cc(type);
            Steinberg_Vst_ParamID param_id = 0;
            if (midi_mapping->lpVtbl->getMidiControllerAssignment(midi_mapping, 0, 0, (Steinberg_Vst_CtrlNumber)cc, &param_id) == Steinberg_kResultOk)
                ih->vst3.cc_param_ids[cc] = (int64_t)param_id;
        }
        midi_mapping->lpVtbl->release(midi_mapping);
    }

    ih->vst3.sample_position = 0;
    ih->vst3.ready = 1;
}

static void _uph_plugin_gui_destroy_window(Uph_PluginInternalHandle *ih)
{
#if NAUI_LINUX
    if (ih->display)
    {
        XDestroyWindow(ih->display, ih->container);
        XSync(ih->display, False);
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

static void uph_unload_vst3_plugin(Uph_PluginInternalHandle *ih)
{
    ih->vst3.ready = 0;
    ih->visible = false;

    if (ih->vst3.processor)
        ih->vst3.processor->lpVtbl->setProcessing(ih->vst3.processor, 0);
    if (ih->vst3.component)
        ih->vst3.component->lpVtbl->setActive(ih->vst3.component, 0);

    if (ih->vst3.view)
    {
        ih->vst3.view->lpVtbl->removed(ih->vst3.view);
        ih->vst3.view->lpVtbl->setFrame(ih->vst3.view, NULL);
        ih->vst3.view->lpVtbl->release(ih->vst3.view);
        ih->vst3.view = NULL;
    }

#if NAUI_LINUX
    for (int i = 0; i < UPH_VST3_MAX_FD_HANDLERS; i++)
        if (ih->vst3.run_loop.fds[i].handler)
            fprintf(stderr, "uph vst3: plugin left an fd handler registered after removed()\n");
    for (int i = 0; i < UPH_VST3_MAX_TIMERS; i++)
        if (ih->vst3.run_loop.timers[i].handler)
            fprintf(stderr, "uph vst3: plugin left a timer registered after removed()\n");
#endif

    uph_vst3_free_partial(ih);
    _uph_plugin_gui_destroy_window(ih);
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
        const Uph_NoteRingEvent *event = uph_note_ring_peek(&ih->vst3.pending_notes, i);
        const uint32_t event_offset = event->header.time < frame_count ? event->header.time : (frame_count ? frame_count - 1 : 0);
        if (event->header.type == CLAP_EVENT_MIDI)
        {
            const int64_t param_id = ih->vst3.cc_param_ids[event->midi.data[1] & 0x7F];
            if (param_id >= 0)
                uph_vst3_param_changes_add(in_params, (Steinberg_Vst_ParamID)param_id, (int32_t)event_offset, (double)(event->midi.data[2] & 0x7F) / 127.0);
            continue;
        }

        const clap_event_note_t *n = &event->note;
        struct Steinberg_Vst_Event *e = &in_events->events[in_events->count++];
        memset(e, 0, sizeof(*e));

        e->busIndex = 0;
        e->sampleOffset = (Steinberg_int32)event_offset;
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
    ctx.tempo = uph_tempo_current_bpm();
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

#define UPH_VST2_STATE_CHUNK  ((uint32_t)CCONST('U', 'V', '2', 'C'))
#define UPH_VST2_STATE_PARAMS ((uint32_t)CCONST('U', 'V', '2', 'P'))

static _Thread_local Uph_PluginInternalHandle *_uph_vst2_pending_owner;

static intptr_t _uph_vst2_copy_string(void *dest, const char *src)
{
    if (!dest)
        return 0;

    snprintf((char*)dest, VestigeMaxNameLen, "%s", src);
    return 1;
}

static bool _uph_vst2_host_can_do(const char *feature)
{
    static const char *const features[] = { "sendVstEvents", "sendVstMidiEvent", "sendVstTimeInfo", "sizeWindow", "supplyIdle" };

    for (size_t i = 0; i < sizeof(features) / sizeof(features[0]); i++)
    {
        if (strcmp(feature, features[i]) == 0)
            return true;
    }

    return false;
}

static intptr_t __cdecl _uph_vst2_host_callback(AEffect *effect, int32_t opcode, int32_t index, intptr_t value, void *ptr, float opt)
{
    (void)opt;

    Uph_PluginInternalHandle *ih = effect && effect->ptr2
        ? (Uph_PluginInternalHandle*)effect->ptr2
        : _uph_vst2_pending_owner;

    switch (opcode)
    {
        case audioMasterVersion:
            return UPH_VST2_HOST_VERSION;

        case audioMasterAutomate:
        case audioMasterBeginEdit:
            if (index >= 0)
                _uph_plugin_mark_touched(ih, (uint64_t)index);
            return 1;

        case audioMasterWantMidi:
        case audioMasterEndEdit:
            return 1;

        case audioMasterGetTime:
            return ih ? (intptr_t)&ih->vst2.time_info : 0;

        case audioMasterSizeWindow:
            if (!ih || !ih->window || index <= 0 || value <= 0)
                return 0;
            _uph_plugin_gui_resize_window(ih, (uint32_t)index, (uint32_t)value);
            return 1;

        case audioMasterGetSampleRate:
            return (intptr_t)uph_state.settings.audio.sample_rate;

        case audioMasterGetBlockSize:
            return UPH_VST2_MAX_BLOCK;

        case audioMasterGetVendorString:
            return _uph_vst2_copy_string(ptr, "Dev Dynasty Studios");

        case audioMasterGetProductString:
            return _uph_vst2_copy_string(ptr, "Uphonic");

        case audioMasterGetVendorVersion:
            return 1;

        case audioMasterCanDo:
            return ptr && _uph_vst2_host_can_do((const char*)ptr);

        case audioMasterGetLanguage:
            return kVstLangEnglish;

        default:
            return 0;
    }
}

static intptr_t _uph_vst2_dispatch(AEffect *effect, int32_t opcode, int32_t index, intptr_t value, void *ptr, float opt)
{
    return effect->dispatcher(effect, opcode, index, value, ptr, opt);
}

static void _uph_vst2_library_close(void *lib)
{
#if NAUI_LINUX
    dlclose(lib);
#elif NAUI_WINDOWS
    FreeLibrary((HMODULE)lib);
#endif
}

static void *_uph_vst2_library_open(const char *path, bool resident, Uph_Vst2MainFn *main_fn)
{
#if NAUI_LINUX
    void *lib = dlopen(path, RTLD_LOCAL | RTLD_LAZY | (resident ? RTLD_NODELETE : 0));
    if (!lib)
        return NULL;

    *main_fn = (Uph_Vst2MainFn)dlsym(lib, "VSTPluginMain");
    if (!*main_fn)
        *main_fn = (Uph_Vst2MainFn)dlsym(lib, "main");
#elif NAUI_WINDOWS
    (void)resident;

    HMODULE lib = LoadLibraryA(path);
    if (!lib)
        return NULL;

    *main_fn = (Uph_Vst2MainFn)(void*)GetProcAddress(lib, "VSTPluginMain");
    if (!*main_fn)
        *main_fn = (Uph_Vst2MainFn)(void*)GetProcAddress(lib, "main");
#endif

    if (!*main_fn)
    {
        _uph_vst2_library_close((void*)lib);
        return NULL;
    }

    return (void*)lib;
}

static AEffect *_uph_vst2_effect_create(Uph_Vst2MainFn main_fn, Uph_PluginInternalHandle *owner)
{
    _uph_vst2_pending_owner = owner;

    AEffect *effect = main_fn(_uph_vst2_host_callback);
    if (effect && effect->magic == kEffectMagic)
    {
        effect->ptr2 = owner;
        _uph_vst2_dispatch(effect, effOpen, 0, 0, NULL, 0.0f);
    }
    else
        effect = NULL;

    _uph_vst2_pending_owner = NULL;
    return effect;
}

static const char *_uph_vst2_unsupported_reason(AEffect *effect)
{
    if (_uph_vst2_dispatch(effect, effGetPlugCategory, 0, 0, NULL, 0.0f) == kPlugCategShell)
        return "shell plug-ins are not supported";
    if (!(effect->flags & effFlagsCanReplacing))
        return "plug-in does not support processReplacing";
    if (!(effect->flags & effFlagsHasEditor))
        return "plug-in has no editor";
    if (effect->numInputs < 0 || effect->numInputs > UPH_VST2_MAX_CHANNELS
        || effect->numOutputs < 0 || effect->numOutputs > UPH_VST2_MAX_CHANNELS)
        return "unsupported channel count";
    return NULL;
}

static Naui_String _uph_vst2_string(AEffect *effect, int32_t opcode, int32_t index)
{
    char buffer[NAUI_STRING_MAX_SIZE] = { 0 };
    _uph_vst2_dispatch(effect, opcode, index, 0, buffer, 0.0f);
    buffer[sizeof(buffer) - 1] = '\0';
    return naui_string_from_cstr(buffer);
}

static Naui_String _uph_vst2_name(AEffect *effect, const Naui_Path *path)
{
    Naui_String name = _uph_vst2_string(effect, effGetEffectName, 0);
    if (!name.length)
        name = _uph_vst2_string(effect, effGetProductString, 0);
    if (!name.length)
        name = naui_view_to_string(naui_file_stem(path));
    return name;
}

static bool _uph_vst2_editor_rect(AEffect *effect, uint32_t *width, uint32_t *height)
{
    Uph_Vst2Rect *rect = NULL;
    _uph_vst2_dispatch(effect, effEditGetRect, 0, 0, &rect, 0.0f);
    if (!rect || rect->right <= rect->left || rect->bottom <= rect->top)
        return false;

    *width = (uint32_t)(rect->right - rect->left);
    *height = (uint32_t)(rect->bottom - rect->top);
    return true;
}

static Naui_List(Uph_PluginParam) _uph_vst2_get_param_list(Uph_Plugin *plug)
{
    Naui_List(Uph_PluginParam) list = NULL;

    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!ih || !ih->vst2.effect || ih->vst2.effect->numParams <= 0)
        return list;

    AEffect *effect = ih->vst2.effect;
    naui_list_reserve(list, (size_t)effect->numParams);

    for (int32_t i = 0; i < effect->numParams; i++)
    {
        const double value = (double)effect->getParameter(effect, i);

        Uph_PluginParam p = {
            .id = (uint64_t)i,
            .name = _uph_vst2_string(effect, effGetParamName, i),
            .min_value = 0.0,
            .max_value = 1.0,
            .default_value = value,
            .current_value = value
        };

        naui_list_push(list, p);
    }

    return list;
}

static void _uph_vst2_free(Uph_PluginInternalHandle *ih)
{
    const bool active = ih->vst2.ready;
    ih->vst2.ready = 0;

    AEffect *effect = ih->vst2.effect;
    if (effect)
    {
        if (active)
        {
            _uph_vst2_dispatch(effect, effStopProcess, 0, 0, NULL, 0.0f);
            _uph_vst2_dispatch(effect, effMainsChanged, 0, 0, NULL, 0.0f);
        }

        if (ih->vst2.editor_open)
            _uph_vst2_dispatch(effect, effEditClose, 0, 0, NULL, 0.0f);

        _uph_vst2_dispatch(effect, effClose, 0, 0, NULL, 0.0f);
        ih->vst2.effect = NULL;
    }

    ih->vst2.editor_open = false;

    free(ih->vst2.buffers);
    ih->vst2.buffers = NULL;

    if (ih->vst2.library_handle)
    {
        _uph_vst2_library_close(ih->vst2.library_handle);
        ih->vst2.library_handle = NULL;
    }
}

static inline void _uph_vst2_load_internal(Uph_Plugin *plug, uint32_t *width, uint32_t *height)
{
    uph_com_init_sta_once();

    Uph_Vst2MainFn main_fn = NULL;
    void *lib = _uph_vst2_library_open(plug->file_path.data, true, &main_fn);
    if (!lib)
    {
        fprintf(stderr, "uph vst2: %s is not a loadable VST2 plug-in\n", plug->file_path.data);
        return;
    }

    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)calloc(1, sizeof(Uph_PluginInternalHandle));
    ih->vst2.library_handle = lib;
    uph_note_ring_init(&ih->vst2.pending_notes);
    uph_param_ring_init(&ih->vst2.pending_params);

    ih->vst2.effect = _uph_vst2_effect_create(main_fn, ih);
    if (!ih->vst2.effect)
    {
        fprintf(stderr, "uph vst2: plug-in entry returned no valid effect\n");
        _uph_vst2_free(ih);
        free(ih);
        return;
    }

    AEffect *effect = ih->vst2.effect;

    const char *reason = _uph_vst2_unsupported_reason(effect);
    if (reason)
    {
        fprintf(stderr, "uph vst2: %s (%s)\n", reason, plug->file_path.data);
        _uph_vst2_free(ih);
        free(ih);
        return;
    }

    ih->display_name = _uph_vst2_name(effect, &plug->file_path);
    fprintf(stderr, "loading vst2 plugin: %s\n", ih->display_name.data);

    ih->vst2.in_channels = effect->numInputs;
    ih->vst2.out_channels = effect->numOutputs;

    const size_t channel_count = (size_t)(ih->vst2.in_channels + ih->vst2.out_channels);
    ih->vst2.buffers = (float*)calloc(channel_count ? channel_count * UPH_VST2_MAX_BLOCK : 1, sizeof(float));

    for (int32_t c = 0; c < ih->vst2.in_channels; c++)
        ih->vst2.in_ptrs[c] = ih->vst2.buffers + (size_t)c * UPH_VST2_MAX_BLOCK;
    for (int32_t c = 0; c < ih->vst2.out_channels; c++)
        ih->vst2.out_ptrs[c] = ih->vst2.buffers + (size_t)(ih->vst2.in_channels + c) * UPH_VST2_MAX_BLOCK;

    for (uint32_t i = 0; i < UPH_VST2_MAX_EVENTS; i++)
        ih->vst2.events.events[i] = (VstEvent*)&ih->vst2.midi_events[i];

    _uph_vst2_dispatch(effect, effSetSampleRate, 0, 0, NULL, (float)uph_state.settings.audio.sample_rate);
    _uph_vst2_dispatch(effect, effSetBlockSize, 0, UPH_VST2_MAX_BLOCK, NULL, 0.0f);

    if (!_uph_vst2_editor_rect(effect, width, height))
    {
        *width = 640;
        *height = 480;
    }

    plug->internal_handle = ih;
    plug->params = _uph_vst2_get_param_list(plug);
}

static inline void _uph_vst2_assign_gui_internal(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;
    AEffect *effect = ih->vst2.effect;

#if NAUI_LINUX
    void *parent = (void*)(uintptr_t)ih->window;
#elif NAUI_WINDOWS
    void *parent = (void*)ih->window;
#endif

    _uph_vst2_dispatch(effect, effEditOpen, 0, 0, parent, 0.0f);
    ih->vst2.editor_open = true;

    uint32_t width = 0, height = 0;
    if (_uph_vst2_editor_rect(effect, &width, &height) && (width != ih->gui_width || height != ih->gui_height))
        _uph_plugin_gui_resize_window(ih, width, height);

    _uph_vst2_dispatch(effect, effMainsChanged, 0, 1, NULL, 0.0f);
    _uph_vst2_dispatch(effect, effStartProcess, 0, 0, NULL, 0.0f);

    ih->vst2.sample_position = 0;
    ih->vst2.ready = 1;
}

static void _uph_vst2_update_time_info(Uph_PluginInternalHandle *ih, double playhead_beat, bool is_playing)
{
    VstTimeInfo *t = &ih->vst2.time_info;
    memset(t, 0, sizeof(*t));

    t->samplePos = (double)ih->vst2.sample_position;
    t->sampleRate = (double)uph_state.settings.audio.sample_rate;
    t->ppqPos = playhead_beat;
    t->tempo = uph_tempo_current_bpm();
    t->barStartPos = floor(playhead_beat / 4.0) * 4.0;
    t->timeSigNumerator = 4;
    t->timeSigDenominator = 4;
    t->flags = kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstTimeSigValid
        | (is_playing ? kVstTransportPlaying : 0)
        | (is_playing != ih->vst2.was_playing ? kVstTransportChanged : 0);

    ih->vst2.was_playing = is_playing;
}

static void _uph_vst2_queue_midi(Uph_PluginInternalHandle *ih, uint32_t note_snapshot, uint32_t frame_count)
{
    AEffect *effect = ih->vst2.effect;
    const uint32_t first_block = frame_count < UPH_VST2_MAX_BLOCK ? frame_count : UPH_VST2_MAX_BLOCK;

    int count = 0;
    for (uint32_t i = 0; i < note_snapshot && count < UPH_VST2_MAX_EVENTS; i++)
    {
        const Uph_NoteRingEvent *event = uph_note_ring_peek(&ih->vst2.pending_notes, i);

        VstMidiEvent *midi = &ih->vst2.midi_events[count++];
        memset(midi, 0, sizeof(*midi));
        midi->type = kVstMidiType;
        midi->byteSize = (int)sizeof(VstMidiEvent);
        midi->deltaFrames = (int)(event->header.time < first_block ? event->header.time : (first_block ? first_block - 1 : 0));

        if (event->header.type == CLAP_EVENT_MIDI)
        {
            memcpy(midi->midiData, event->midi.data, 3);
            continue;
        }

        const bool note_on = event->header.type == CLAP_EVENT_NOTE_ON;
        const int16_t channel = event->note.channel < 0 ? 0 : event->note.channel;

        int velocity = (int)(event->note.velocity * 127.0 + 0.5);
        velocity = velocity < 0 ? 0 : (velocity > 127 ? 127 : velocity);
        if (note_on && velocity == 0)
            velocity = 1;

        midi->midiData[0] = (char)((note_on ? 0x90 : 0x80) | (channel & 0x0F));
        midi->midiData[1] = (char)(event->note.key & 0x7F);
        midi->midiData[2] = (char)velocity;
    }

    ih->vst2.events.num_events = count;
    if (count > 0)
        _uph_vst2_dispatch(effect, effProcessEvents, 0, 0, &ih->vst2.events, 0.0f);
}

static void _uph_vst2_process(
    Uph_PluginInternalHandle *ih,
    float **inputs,
    float **outputs,
    uint32_t frame_count,
    double playhead_beat,
    bool is_playing
)
{
    if (!ih->vst2.ready || !outputs)
        return;

    AEffect *effect = ih->vst2.effect;

    const uint32_t note_snapshot  = uph_note_ring_size(&ih->vst2.pending_notes);
    const uint32_t param_snapshot = uph_param_ring_size(&ih->vst2.pending_params);

    _uph_vst2_queue_midi(ih, note_snapshot, frame_count);

    for (uint32_t i = 0; i < param_snapshot; i++)
    {
        const clap_event_param_value_t *p = uph_param_ring_peek(&ih->vst2.pending_params, i);
        if (p->param_id < (clap_id)effect->numParams)
            effect->setParameter(effect, (int)p->param_id, (float)p->value);
    }

    _uph_vst2_update_time_info(ih, playhead_beat, is_playing);

    const int32_t in_channels = ih->vst2.in_channels;
    const int32_t out_channels = ih->vst2.out_channels;

    float *out_ptrs[UPH_VST2_MAX_CHANNELS];

    for (uint32_t offset = 0; offset < frame_count; offset += UPH_VST2_MAX_BLOCK)
    {
        const uint32_t block = frame_count - offset < UPH_VST2_MAX_BLOCK ? frame_count - offset : UPH_VST2_MAX_BLOCK;
        const size_t bytes = (size_t)block * sizeof(float);

        for (int32_t c = 0; c < in_channels; c++)
        {
            if (inputs && c < 2)
                memcpy(ih->vst2.in_ptrs[c], inputs[c] + offset, bytes);
            else
                memset(ih->vst2.in_ptrs[c], 0, bytes);
        }

        for (int32_t c = 0; c < out_channels; c++)
            out_ptrs[c] = c < 2 ? outputs[c] + offset : ih->vst2.out_ptrs[c];

        effect->processReplacing(effect, ih->vst2.in_ptrs, out_ptrs, (int)block);

        if (out_channels == 1)
            memcpy(outputs[1] + offset, outputs[0] + offset, bytes);
    }

    ih->vst2.sample_position += (int64_t)frame_count;

    uph_note_ring_advance(&ih->vst2.pending_notes, note_snapshot);
    uph_param_ring_advance(&ih->vst2.pending_params, param_snapshot);
}

static bool _uph_vst2_write_state(const Naui_Path path, uint32_t kind, const void *data, size_t size)
{
    const uint32_t header[2] = { kind, (uint32_t)size };

    uint8_t *buffer = (uint8_t*)malloc(sizeof(header) + size);
    if (!buffer)
        return false;

    memcpy(buffer, header, sizeof(header));
    if (size)
        memcpy(buffer + sizeof(header), data, size);

    const bool ok = naui_file_write_all(path, buffer, sizeof(header) + size);
    free(buffer);
    return ok;
}

static bool _uph_vst2_save_state(Uph_PluginInternalHandle *ih, const Naui_Path path)
{
    AEffect *effect = ih->vst2.effect;

    if (effect->flags & UPH_VST2_FLAG_PROGRAM_CHUNKS)
    {
        void *chunk = NULL;
        const intptr_t size = _uph_vst2_dispatch(effect, UPH_VST2_EFF_GET_CHUNK, 0, 0, &chunk, 0.0f);
        if (size > 0 && chunk)
            return _uph_vst2_write_state(path, UPH_VST2_STATE_CHUNK, chunk, (size_t)size);
    }

    const size_t count = effect->numParams > 0 ? (size_t)effect->numParams : 0;
    float *values = (float*)malloc(count ? count * sizeof(float) : 1);
    if (!values)
        return false;

    for (size_t i = 0; i < count; i++)
        values[i] = effect->getParameter(effect, (int)i);

    const bool ok = _uph_vst2_write_state(path, UPH_VST2_STATE_PARAMS, values, count * sizeof(float));
    free(values);
    return ok;
}

static bool _uph_vst2_load_state(Uph_PluginInternalHandle *ih, const Naui_Path path)
{
    size_t size = 0;
    uint8_t *data = (uint8_t*)naui_file_read_all(path, &size);
    if (!data)
    {
        fprintf(stderr, "uph_plugin_load_state: failed to read %s\n", path.data);
        return false;
    }

    bool ok = false;
    uint32_t header[2] = { 0 };

    if (size >= sizeof(header))
    {
        memcpy(header, data, sizeof(header));

        AEffect *effect = ih->vst2.effect;
        const uint8_t *payload = data + sizeof(header);

        if (header[1] > size - sizeof(header))
            fprintf(stderr, "uph_plugin_load_state: truncated VST2 state in %s\n", path.data);
        else if (header[0] == UPH_VST2_STATE_CHUNK && (effect->flags & UPH_VST2_FLAG_PROGRAM_CHUNKS))
        {
            _uph_vst2_dispatch(effect, UPH_VST2_EFF_SET_CHUNK, 0, (intptr_t)header[1], (void*)payload, 0.0f);
            ok = true;
        }
        else if (header[0] == UPH_VST2_STATE_PARAMS)
        {
            const size_t stored = header[1] / sizeof(float);
            const size_t count = effect->numParams > 0 && (size_t)effect->numParams < stored ? (size_t)effect->numParams : stored;

            for (size_t i = 0; i < count; i++)
            {
                float value;
                memcpy(&value, payload + i * sizeof(float), sizeof(float));
                effect->setParameter(effect, (int)i, value);
            }
            ok = true;
        }
    }

    free(data);
    return ok;
}

static bool _uph_get_vst2_plugin_info(Naui_Path path, Uph_PluginInfo *out)
{
    Uph_Vst2MainFn main_fn = NULL;
    void *lib = _uph_vst2_library_open(path.data, false, &main_fn);
    if (!lib)
        return false;

    bool ok = false;
    AEffect *effect = _uph_vst2_effect_create(main_fn, NULL);

    if (effect)
    {
        const char *reason = _uph_vst2_unsupported_reason(effect);
        if (reason)
            fprintf(stderr, "uph info: skipping %s: %s\n", path.data, reason);
        else
        {
            const intptr_t category = _uph_vst2_dispatch(effect, effGetPlugCategory, 0, 0, NULL, 0.0f);

            out->name = _uph_vst2_name(effect, &path);
            out->vendor = _uph_vst2_string(effect, effGetVendorString, 0);
            out->format = UPH_PLUGIN_VST2;
            out->type = (effect->flags & effFlagsIsSynth) || category == kPlugCategSynth
                ? UPH_PLUGIN_INSTRUMENT
                : UPH_PLUGIN_EFFECT;
            ok = true;
        }

        _uph_vst2_dispatch(effect, effClose, 0, 0, NULL, 0.0f);
    }
    else
        fprintf(stderr, "uph info: no valid VST2 effect in %s\n", path.data);

    _uph_vst2_library_close(lib);

    out->path = path;

    return ok;
}

static void _uph_vst2_unload(Uph_PluginInternalHandle *ih)
{
    ih->visible = false;
    _uph_vst2_free(ih);
    _uph_plugin_gui_destroy_window(ih);
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

typedef struct
{
    Uph_NoteEventRing *notes;
    Uph_ParamEventRing *params;
    bool *active_notes;
    int16_t *active_channels;
    uint8_t *cc_values;
}
Uph_PluginEventState;

#define _UPH_PLUGIN_EVENT_STATE(s) (Uph_PluginEventState){ &(s).pending_notes, &(s).pending_params, (s).active_notes, (s).active_note_channels, (s).cc_values }

static Uph_PluginEventState _uph_plugin_event_state(const Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;
    switch (plug->format)
    {
        case UPH_PLUGIN_VST3: return _UPH_PLUGIN_EVENT_STATE(ih->vst3);
        case UPH_PLUGIN_VST2: return _UPH_PLUGIN_EVENT_STATE(ih->vst2);
        default:              return _UPH_PLUGIN_EVENT_STATE(ih->clap);
    }
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
    Uph_NoteRingEvent ev = {0};
    ev.header.size = sizeof(clap_event_note_t);
    ev.header.time = sample_offset;
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = note_on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
    ev.header.flags = 0;

    ev.note.note_id = -1;
    ev.note.port_index = 0;
    ev.note.channel = channel;
    ev.note.key = key;
    ev.note.velocity = velocity / 127.0;

    const Uph_PluginEventState state = _uph_plugin_event_state(plug);

    if (!uph_note_ring_push(state.notes, &ev))
    {
        fprintf(stderr, "uph_plugin_queue_note_event: note ring full, dropping event (key=%u on=%d)\n",
                key, (int)note_on);
        return;
    }

    state.active_notes[key] = note_on;
    if (note_on)
        state.active_channels[key] = channel;
}

void uph_plugin_queue_param_change(
    Uph_Plugin *plug,
    clap_id param_id,
    double value,
    uint32_t sample_offset
)
{
    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!ih)
        return;
 
    double normalized = value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
 
    double event_value = normalized;
    void *cookie = NULL;
 
    if (plug->format == UPH_PLUGIN_CLAP)
    {
        const Uph_PluginParam *param = NULL;
        uint64_t param_count = naui_list_len(plug->params);
        for (uint64_t i = 0; i < param_count; i++)
        {
            if (plug->params[i].id == (uint64_t)param_id)
            {
                param = &plug->params[i];
                break;
            }
        }
 
        if (!param)
            return;
 
        event_value = param->min_value + normalized * (param->max_value - param->min_value);
        cookie = param->cookie;
    }
 
    clap_event_param_value_t ev = {0};
    ev.header.size = sizeof(clap_event_param_value_t);
    ev.header.time = sample_offset;
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_PARAM_VALUE;
    ev.header.flags = 0;
 
    ev.param_id = param_id;
    ev.cookie = cookie;
    ev.note_id = -1;
    ev.port_index = -1;
    ev.channel = -1;
    ev.key = -1;
    ev.value = event_value;
 
    if (!uph_param_ring_push(_uph_plugin_event_state(plug).params, &ev))
    {
        fprintf(stderr, "uph_plugin_queue_param_change: param ring full, dropping event (param_id=%u)\n",
                (unsigned)param_id);
    }
}

void uph_plugin_queue_stop_all(Uph_Plugin *plug, uint32_t sample_offset)
{
    const Uph_PluginEventState state = _uph_plugin_event_state(plug);

    for (int key = 0; key < 128; key++)
    {
        if (!state.active_notes[key])
            continue;

        Uph_NoteRingEvent ev = {0};
        ev.header.size = sizeof(clap_event_note_t);
        ev.header.time = sample_offset;
        ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        ev.header.type = CLAP_EVENT_NOTE_OFF;
        ev.header.flags = 0;

        ev.note.note_id = -1;
        ev.note.port_index = 0;
        ev.note.channel = state.active_channels[key];
        ev.note.key = (int16_t)key;
        ev.note.velocity = 0.0;

        if (!uph_note_ring_push(state.notes, &ev))
        {
            fprintf(stderr, "uph_plugin_queue_stop_all: note ring full, dropping off for key %d\n", key);
            continue;
        }

        state.active_notes[key] = false;
    }

    for (Uph_MidiPedalType type = 0; type < UPH_MIDI_PEDAL_COUNT; type++)
    {
        const uint8_t cc = uph_midi_pedal_cc(type);
        if (uph_plugin_cc_value(plug, cc) != 0)
            uph_plugin_queue_midi_cc(plug, 0, cc, 0, sample_offset);
    }
}

void uph_plugin_queue_midi_cc(Uph_Plugin *plug, uint8_t channel, uint8_t cc, uint8_t value, uint32_t sample_offset)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!internal_handle || cc > 127)
        return;

    Uph_NoteRingEvent ev = {0};
    ev.midi.header.size = sizeof(clap_event_midi_t);
    ev.midi.header.time = sample_offset;
    ev.midi.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.midi.header.type = CLAP_EVENT_MIDI;
    ev.midi.header.flags = 0;
    ev.midi.port_index = 0;
    ev.midi.data[0] = (uint8_t)(0xB0 | (channel & 0x0F));
    ev.midi.data[1] = cc;
    ev.midi.data[2] = value & 0x7F;

    const Uph_PluginEventState state = _uph_plugin_event_state(plug);

    if (!uph_note_ring_push(state.notes, &ev))
    {
        fprintf(stderr, "uph_plugin_queue_midi_cc: note ring full, dropping event (cc=%u value=%u)\n", cc, value);
        return;
    }

    state.cc_values[cc] = value & 0x7F;
}

uint8_t uph_plugin_cc_value(Uph_Plugin *plug, uint8_t cc)
{
    if (!plug->internal_handle || cc > 127)
        return 0;

    return _uph_plugin_event_state(plug).cc_values[cc];
}

bool uph_plugin_note_active(Uph_Plugin *plug, uint8_t key)
{
    return _uph_plugin_event_state(plug).active_notes[key];
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

    _uph_plugin_gui_resize_window(internal_handle, width, height);
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
    if (strcmp(extension_id, CLAP_EXT_PARAMS) == 0)
        return &uph_clap_host_params;
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
            .current_value = current_value,
            .cookie = info.cookie
        };

        naui_list_push(list, p);
    }

    return list;
}

#if NAUI_WINDOWS
static LRESULT CALLBACK uph_plugin_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_CLOSE)
        return 0;
    if (msg == WM_MOUSEACTIVATE)
        return MA_NOACTIVATE;
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
    if (internal_handle->visible)
        gui->show(plugin);

    const uint32_t max_frame_count = 2048;
    plugin->activate(
        plugin,
        uph_state.settings.audio.sample_rate,
        1,
        max_frame_count
    );

    plugin->start_processing(plugin);
    internal_handle->clap.ready = 1;
}

#define UPH_MODULE_SCAN_WAIT_MS 15000
#define UPH_PREFETCH_CHUNK_SIZE (1024 * 1024)

#pragma region Module Registry

typedef struct
{
    Naui_Mutex lock;
    Naui_List(Naui_Path) in_use;
    Naui_Path scanning;
    atomic_bool prefetch_abort;
}
Uph_PluginModuleRegistryState;

static Uph_PluginModuleRegistryState _uph_plugin_module_registry_state;

static bool _uph_module_path_equal(const Naui_Path a, const Naui_Path b)
{
    const Naui_Path left = naui_path_normalize(a);
    const Naui_Path right = naui_path_normalize(b);
#if NAUI_WINDOWS
    return _stricmp(left.data, right.data) == 0;
#else
    return strcmp(left.data, right.data) == 0;
#endif
}

// CLAP forbids init() while anything else in the DSO is running
static void _uph_module_acquire(const Naui_Path path)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    if (!state->lock)
        return;

    for (uint32_t waited = 0; ; waited += 2)
    {
        naui_mutex_lock(state->lock);
        if (waited >= UPH_MODULE_SCAN_WAIT_MS || !_uph_module_path_equal(state->scanning, path))
        {
            naui_list_push(state->in_use, path);
            naui_mutex_unlock(state->lock);
            return;
        }

        naui_mutex_unlock(state->lock);
        naui_thread_sleep_ms(2);
    }
}

static void _uph_module_release(const Naui_Path path)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    if (!state->lock)
        return;

    naui_mutex_lock(state->lock);
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->in_use); i++)
    {
        if (_uph_module_path_equal(state->in_use[i], path))
        {
            naui_list_remove(state->in_use, i);
            break;
        }
    }
    naui_mutex_unlock(state->lock);
}

void uph_plugin_modules_init(void)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    if (!state->lock)
        state->lock = naui_mutex_create();
}

void uph_plugin_modules_shutdown(void)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    if (!state->lock)
        return;

    naui_mutex_destroy(state->lock);
    state->lock = NULL;
    naui_list_free(state->in_use);
    state->in_use = NULL;
}

bool uph_plugin_module_scan_begin(Naui_Path path)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    if (!state->lock)
        return true;

    naui_mutex_lock(state->lock);
    bool free_to_scan = true;
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->in_use); i++)
    {
        if (_uph_module_path_equal(state->in_use[i], path))
        {
            free_to_scan = false;
            break;
        }
    }

    if (free_to_scan)
        state->scanning = path;

    naui_mutex_unlock(state->lock);
    return free_to_scan;
}

void uph_plugin_module_scan_end(void)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    if (!state->lock)
        return;

    naui_mutex_lock(state->lock);
    state->scanning = (Naui_Path){ 0 };
    naui_mutex_unlock(state->lock);
}

void uph_plugin_prefetch_abort(void)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    atomic_store(&state->prefetch_abort, true);
}

void uph_plugin_prefetch(Naui_Path path, const atomic_bool *cancel)
{
    Uph_PluginModuleRegistryState *state = &_uph_plugin_module_registry_state;
    char binary[1024];
    const char *file = path.data;
    if (naui_string_view_equals_cstr(naui_file_extension(&path), ".vst3", false))
    {
        if (!uph_vst3_resolve_binary(path.data, binary, sizeof(binary)))
            return;

        file = binary;
    }

    FILE *stream = fopen(file, "rb");
    if (!stream)
        return;

    char *chunk = (char*)malloc(UPH_PREFETCH_CHUNK_SIZE);
    while (chunk && !atomic_load(&state->prefetch_abort) && !(cancel && atomic_load(cancel)))
    {
        if (fread(chunk, 1, UPH_PREFETCH_CHUNK_SIZE, stream) < UPH_PREFETCH_CHUNK_SIZE)
            break;
    }

    free(chunk);
    fclose(stream);
}

#pragma endregion

static Uph_Plugin _uph_plugin_load_impl(Naui_Path path)
{
    Uph_Plugin effect = { 0 };

    uph_com_init_sta_once();

    effect.file_path = path;

    if (!uph_plugin_format_from_path(path, &effect.format))
    {
        fprintf(stderr, "uph: unrecognized plugin extension for %s\n", path.data);
        return effect;
    }

    uint32_t width = 0, height = 0;
    switch (effect.format)
    {
        case UPH_PLUGIN_CLAP: uph_load_clap_plugin_internal(&effect, &width, &height); break;
        case UPH_PLUGIN_VST3: uph_load_vst3_plugin_internal(&effect, &width, &height); break;
        case UPH_PLUGIN_VST2: _uph_vst2_load_internal(&effect, &width, &height); break;
    }

    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)effect.internal_handle;
    if (!internal_handle)
        return effect;

    internal_handle->is_clap = (effect.format == UPH_PLUGIN_CLAP);
    effect.name = internal_handle->display_name;

    internal_handle->gui_width = width;
    internal_handle->gui_height = height;

#if NAUI_LINUX
    Display *dpy = (Display*)XOpenDisplay(NULL);
    int screen = DefaultScreen(dpy);

    XSetWindowAttributes attributes = { .override_redirect = True };
    Window container = XCreateWindow(
        dpy, RootWindow(dpy, screen),
        0, 0, 1, 1, 0,
        CopyFromParent, InputOutput, CopyFromParent,
        CWOverrideRedirect, &attributes
    );
    XShapeCombineRectangles(dpy, container, ShapeBounding, 0, 0, NULL, 0, ShapeSet, Unsorted);
    XShapeCombineRectangles(dpy, container, ShapeInput, 0, 0, NULL, 0, ShapeSet, Unsorted);

    Window child = XCreateSimpleWindow(
        dpy, container,
        0, 0, width, height,
        0, BlackPixel(dpy, screen), BlackPixel(dpy, screen)
    );
    XCompositeRedirectWindow(dpy, child, CompositeRedirectManual);
    XMapWindow(dpy, container);
    XFlush(dpy);

    internal_handle->container = container;
    internal_handle->window = child;
    internal_handle->display = dpy;
#elif NAUI_WINDOWS
    uph_register_plugin_wnd_class();

    HWND child = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "UphPluginChildWindow",
        internal_handle->display_name.data,
        WS_POPUP | WS_CLIPCHILDREN,
        0, 0,
        (int)width, (int)height,
        (HWND)mgapp_primary_handle(),
        NULL,
        GetModuleHandleA(NULL),
        NULL
    );

    if (!child)
    {
        fprintf(stderr, "uph: CreateWindowExA failed: %lu\n", GetLastError());
        return effect;
    }

    SetLayeredWindowAttributes(child, 0, 1, LWA_ALPHA);
    SetWindowLongPtrA(child, GWLP_USERDATA, (LONG_PTR)internal_handle);
    internal_handle->window = child;
#endif

    switch (effect.format)
    {
        case UPH_PLUGIN_CLAP: uph_assign_clap_plugin_gui_internal(&effect); break;
        case UPH_PLUGIN_VST3: uph_assign_vst3_plugin_gui_internal(&effect); break;
        case UPH_PLUGIN_VST2: _uph_vst2_assign_gui_internal(&effect); break;
    }

    effect.loaded = true;

    return effect;
}

Uph_Plugin uph_plugin_load(Naui_Path path, bool show_window)
{
    _uph_module_acquire(path);

    Uph_Plugin plugin = _uph_plugin_load_impl(path);
    if (!plugin.loaded)
        _uph_module_release(path);
    else if (show_window)
        uph_plugin_show_window(&plugin);

    return plugin;
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
        {
            if (internal_handle->visible)
            {
                internal_handle->clap.gui->hide(plugin);
                internal_handle->visible = false;
            }
            internal_handle->clap.gui->destroy(plugin);
        }

        plugin->destroy(plugin);
        internal_handle->clap.plugin = NULL;
    }

#if NAUI_LINUX
    if (internal_handle->display)
    {
        XDestroyWindow(internal_handle->display, internal_handle->container);
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
#endif

    if (internal_handle->clap.entry)
    {
        internal_handle->clap.entry->deinit();
        internal_handle->clap.entry = NULL;
    }

    if (internal_handle->clap.library_handle)
    {
#if NAUI_WINDOWS
        FreeLibrary((HMODULE)internal_handle->clap.library_handle);
#endif
        internal_handle->clap.library_handle = NULL;
    }
}

void uph_plugin_unload(Uph_Plugin *plug)
{
    if (!plug->loaded)
        return;

    const bool was_running = uph_audio_engine_device_running();
    if (was_running)
        uph_audio_engine_stop();

    plug->loaded = false;

    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (internal_handle)
    {
        if (internal_handle->panel)
        {
            uph_plugin_editor_destroy(internal_handle->panel);
            internal_handle->panel = 0;
        }

        _uph_plugin_gui_release_capture(internal_handle);

        switch (plug->format)
        {
            case UPH_PLUGIN_CLAP: uph_unload_clap_plugin(internal_handle); break;
            case UPH_PLUGIN_VST3: uph_unload_vst3_plugin(internal_handle); break;
            case UPH_PLUGIN_VST2: _uph_vst2_unload(internal_handle); break;
        }

        free(plug->internal_handle);
        plug->internal_handle = NULL;
    }

    naui_list_free(plug->params);
    _uph_module_release(plug->file_path);

    if (was_running)
        uph_audio_engine_start();
}

static inline long uph_ms_since(float then)
{
    return (long)((naui_time() - then) * 1000.0f);
}

static void _uph_plugin_texture_destroy_deferred(Naui_Texture *texture)
{
    naui_texture_destroy(*texture);
}

static void _uph_plugin_gui_release_texture(Uph_PluginInternalHandle *ih)
{
    if (ih->texture)
        naui_defer((Naui_DeferredEvent)_uph_plugin_texture_destroy_deferred, &ih->texture, sizeof(Naui_Texture));
    ih->texture = NULL;
    ih->texture_width = ih->texture_height = 0;
}

static void _uph_plugin_gui_frame_reserve(Uph_PluginGuiFrame *frame, uint32_t width, uint32_t height)
{
    if (frame->pixels && frame->width == width && frame->height == height)
        return;

    free(frame->pixels);
    frame->pixels = (uint8_t*)malloc((size_t)width * height * 4);
    frame->width = width;
    frame->height = height;
}

static void _uph_plugin_gui_frame_free(Uph_PluginGuiFrame *frame)
{
    free(frame->pixels);
    *frame = (Uph_PluginGuiFrame){ 0 };
}

static void _uph_plugin_gui_copy_opaque_row(uint8_t *dst, const uint8_t *src, uint32_t width)
{
    uint32_t *out = (uint32_t*)dst;
    const uint32_t *in = (const uint32_t*)src;
    for (uint32_t x = 0; x < width; x++)
        out[x] = in[x] | 0xFF000000u;
}

#if NAUI_LINUX
static int _uph_plugin_x11_swallow_error(Display *display, xError *error, XExtCodes *codes, int *ret_code)
{
    (void)display;
    (void)error;
    (void)codes;
    *ret_code = 0;
    return True;
}

static void _uph_plugin_gui_capture_open(Uph_PluginGuiCapture *capture)
{
    capture->display = XOpenDisplay(NULL);
    if (!capture->display)
        return;

    XExtCodes *codes = XAddExtension(capture->display);
    if (codes)
        XESetError(capture->display, codes->extension, _uph_plugin_x11_swallow_error);
}

static void _uph_plugin_gui_capture_close(Uph_PluginGuiCapture *capture)
{
    if (capture->display)
        XCloseDisplay(capture->display);
    capture->display = NULL;
}

static bool _uph_plugin_gui_read_pixels(Uph_PluginGuiCapture *capture, Uph_PluginGuiFrame *frame)
{
    Display *display = capture->display;
    if (!display)
        return false;

    bool ok = false;
    Pixmap pixmap = XCompositeNameWindowPixmap(display, capture->window);
    XImage *image = pixmap ? XGetImage(display, pixmap, 0, 0, frame->width, frame->height, AllPlanes, ZPixmap) : NULL;

    if (image && image->bits_per_pixel == 32)
    {
        for (uint32_t y = 0; y < frame->height; y++)
            _uph_plugin_gui_copy_opaque_row(frame->pixels + (size_t)y * frame->width * 4, (const uint8_t*)image->data + (size_t)y * image->bytes_per_line, frame->width);
        ok = true;
    }

    if (image)
        XDestroyImage(image);
    if (pixmap)
        XFreePixmap(display, pixmap);

    XSync(display, False);
    return ok;
}
#elif NAUI_WINDOWS
#ifndef PW_RENDERFULLCONTENT
    #define PW_RENDERFULLCONTENT 0x00000002
#endif

static void _uph_plugin_gui_capture_open(Uph_PluginGuiCapture *capture)
{
    (void)capture;
}

static void _uph_plugin_gui_capture_close(Uph_PluginGuiCapture *capture)
{
    if (capture->dc)
        DeleteDC(capture->dc);
    if (capture->bitmap)
        DeleteObject(capture->bitmap);
    capture->dc = NULL;
    capture->bitmap = NULL;
    capture->bits = NULL;
    capture->bitmap_width = capture->bitmap_height = 0;
}

static bool _uph_plugin_gui_read_pixels(Uph_PluginGuiCapture *capture, Uph_PluginGuiFrame *frame)
{
    if (!capture->dc || capture->bitmap_width != frame->width || capture->bitmap_height != frame->height)
    {
        _uph_plugin_gui_capture_close(capture);

        HDC screen_dc = GetDC(NULL);
        BITMAPINFO info = { 0 };
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = (LONG)frame->width;
        info.bmiHeader.biHeight = -(LONG)frame->height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;

        capture->dc = CreateCompatibleDC(screen_dc);
        capture->bitmap = CreateDIBSection(screen_dc, &info, DIB_RGB_COLORS, &capture->bits, NULL, 0);
        ReleaseDC(NULL, screen_dc);

        if (!capture->dc || !capture->bitmap)
            return false;
        SelectObject(capture->dc, capture->bitmap);
        capture->bitmap_width = frame->width;
        capture->bitmap_height = frame->height;
    }

    if (!PrintWindow(capture->window, capture->dc, PW_RENDERFULLCONTENT))
        return false;
    GdiFlush();

    const size_t row_bytes = (size_t)frame->width * 4;
    for (uint32_t y = 0; y < frame->height; y++)
        _uph_plugin_gui_copy_opaque_row(frame->pixels + y * row_bytes, (const uint8_t*)capture->bits + y * row_bytes, frame->width);
    return true;
}
#endif

static int _uph_plugin_gui_capture_thread(void *arg)
{
    Uph_PluginGuiCapture *capture = (Uph_PluginGuiCapture*)arg;
    _uph_plugin_gui_capture_open(capture);

    naui_mutex_lock(capture->lock);
    for (;;)
    {
        while (!capture->stop && !capture->requested)
            naui_cond_wait(capture->wake, capture->lock);
        if (capture->stop)
            break;

        capture->requested = false;
        const uint32_t width = capture->width;
        const uint32_t height = capture->height;
        naui_mutex_unlock(capture->lock);

        _uph_plugin_gui_frame_reserve(&capture->back, width, height);
        const bool ok = capture->back.pixels && _uph_plugin_gui_read_pixels(capture, &capture->back);

        naui_mutex_lock(capture->lock);
        if (ok)
        {
            const Uph_PluginGuiFrame captured = capture->back;
            capture->back = capture->ready;
            capture->ready = captured;
            capture->fresh = true;
        }
    }
    capture->finished = true;
    naui_mutex_unlock(capture->lock);

    _uph_plugin_gui_capture_close(capture);
    return 0;
}

static bool _uph_plugin_gui_capture_start(Uph_PluginInternalHandle *ih)
{
    Uph_PluginGuiCapture *capture = &ih->capture;
    capture->lock = naui_mutex_create();
    capture->wake = naui_cond_create();
    capture->window = ih->window;
    capture->stop = capture->requested = capture->fresh = capture->finished = false;

    if (capture->lock && capture->wake)
        capture->thread = naui_thread_create(_uph_plugin_gui_capture_thread, capture);
    if (capture->thread)
        return true;

    if (capture->wake)
        naui_cond_destroy(capture->wake);
    if (capture->lock)
        naui_mutex_destroy(capture->lock);
    capture->wake = NULL;
    capture->lock = NULL;
    return false;
}

static void _uph_plugin_gui_capture_stop(Uph_PluginGuiCapture *capture)
{
    if (!capture->thread)
        return;

    naui_mutex_lock(capture->lock);
    capture->stop = true;
    naui_cond_signal(capture->wake);
    naui_mutex_unlock(capture->lock);

#if NAUI_WINDOWS
    for (;;)
    {
        naui_mutex_lock(capture->lock);
        const bool finished = capture->finished;
        naui_mutex_unlock(capture->lock);
        if (finished)
            break;

        MSG msg;
        PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
        naui_thread_sleep_ms(1);
    }
#endif

    naui_thread_join(capture->thread, NULL);
    naui_cond_destroy(capture->wake);
    naui_mutex_destroy(capture->lock);
    capture->thread = NULL;
    capture->wake = NULL;
    capture->lock = NULL;

    _uph_plugin_gui_frame_free(&capture->back);
    _uph_plugin_gui_frame_free(&capture->ready);
    _uph_plugin_gui_frame_free(&capture->front);
}

static void _uph_plugin_gui_release_capture(Uph_PluginInternalHandle *ih)
{
    _uph_plugin_gui_capture_stop(&ih->capture);
    _uph_plugin_gui_release_texture(ih);
}

static void _uph_plugin_gui_capture(Uph_PluginInternalHandle *ih)
{
    const uint32_t width = ih->gui_width;
    const uint32_t height = ih->gui_height;
    if (!width || !height)
        return;

    Uph_PluginGuiCapture *capture = &ih->capture;
    if (!capture->thread && !_uph_plugin_gui_capture_start(ih))
        return;

    naui_mutex_lock(capture->lock);
    const bool fresh = capture->fresh;
    if (fresh)
    {
        const Uph_PluginGuiFrame captured = capture->ready;
        capture->ready = capture->front;
        capture->front = captured;
        capture->fresh = false;
    }
    capture->width = width;
    capture->height = height;
    capture->requested = true;
    naui_cond_signal(capture->wake);
    naui_mutex_unlock(capture->lock);

    if (!fresh || capture->front.width != width || capture->front.height != height)
        return;

    if (ih->texture_width != width || ih->texture_height != height)
    {
        _uph_plugin_gui_release_texture(ih);
        ih->texture = naui_texture_create(width, height, NAUI_TEXTURE_FORMAT_BGRA8, NAUI_MEMORY_SHARED);
        ih->texture_width = width;
        ih->texture_height = height;
    }

    naui_texture_update(ih->texture, capture->front.pixels);
}

const char *uph_plugin_gui_title(Uph_PluginInternalHandle *handle)
{
    return handle->display_name.data;
}

void uph_plugin_gui_size(Uph_PluginInternalHandle *handle, uint32_t *width, uint32_t *height)
{
    *width = handle->gui_width;
    *height = handle->gui_height;
}

Naui_Texture uph_plugin_gui_texture(Uph_PluginInternalHandle *handle, uint32_t *width, uint32_t *height)
{
    *width = handle->texture_width;
    *height = handle->texture_height;
    return handle->texture;
}

#if NAUI_LINUX
static void _uph_plugin_gui_x11_notify_moved(Uph_PluginInternalHandle *ih)
{
    Window root, parent, *children = NULL;
    unsigned int count = 0;
    if (!XQueryTree(ih->display, ih->window, &root, &parent, &children, &count))
        return;

    for (unsigned int i = 0; i < count; i++)
    {
        XWindowAttributes attributes;
        if (!XGetWindowAttributes(ih->display, children[i], &attributes))
            continue;

        XEvent event = { 0 };
        XConfigureEvent *e = &event.xconfigure;
        e->type = ConfigureNotify;
        e->display = ih->display;
        e->event = children[i];
        e->window = children[i];
        e->x = ih->screen_x + attributes.x;
        e->y = ih->screen_y + attributes.y;
        e->width = attributes.width;
        e->height = attributes.height;
        e->border_width = attributes.border_width;
        e->above = None;
        e->override_redirect = False;
        XSendEvent(ih->display, children[i], False, StructureNotifyMask, &event);
    }

    if (children)
        XFree(children);
}
#endif

void uph_plugin_gui_set_origin(Uph_PluginInternalHandle *handle, int32_t x, int32_t y)
{
#if NAUI_LINUX
    Window root = DefaultRootWindow(handle->display);
    Window child;
    int root_x = 0, root_y = 0;
    XTranslateCoordinates(handle->display, (Window)mgapp_primary_handle(), root, x, y, &root_x, &root_y, &child);
#elif NAUI_WINDOWS
    POINT point = { x, y };
    ClientToScreen((HWND)mgapp_primary_handle(), &point);
    const int32_t root_x = point.x, root_y = point.y;
#endif

    if (root_x == handle->screen_x && root_y == handle->screen_y && !handle->origin_dirty)
        return;

    handle->screen_x = root_x;
    handle->screen_y = root_y;
    handle->origin_dirty = false;

#if NAUI_LINUX
    XMoveWindow(handle->display, handle->container, root_x, root_y);
    _uph_plugin_gui_x11_notify_moved(handle);
    XFlush(handle->display);
#elif NAUI_WINDOWS
    SetWindowPos(handle->window, NULL, root_x, root_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

#if NAUI_LINUX
static Window _uph_plugin_gui_target_at(Uph_PluginInternalHandle *ih, int32_t x, int32_t y, int *out_x, int *out_y)
{
    Window target = ih->window;
    int tx = x, ty = y;

    if (ih->buttons_down && ih->input_target)
    {
        Window child;
        XTranslateCoordinates(ih->display, ih->window, ih->input_target, x, y, out_x, out_y, &child);
        return ih->input_target;
    }

    for (;;)
    {
        Window child = None, unused;
        int cx, cy;
        if (!XTranslateCoordinates(ih->display, target, target, tx, ty, &cx, &cy, &child) || child == None)
            break;
        XTranslateCoordinates(ih->display, target, child, tx, ty, &cx, &cy, &unused);
        target = child;
        tx = cx;
        ty = cy;
    }

    *out_x = tx;
    *out_y = ty;
    return target;
}

static Window _uph_plugin_gui_x11_listener(Uph_PluginInternalHandle *ih, Window target, long mask, int *x, int *y)
{
    for (;;)
    {
        XWindowAttributes attributes;
        if (!XGetWindowAttributes(ih->display, target, &attributes))
            return None;
        if ((attributes.all_event_masks & mask) || target == ih->window || (attributes.do_not_propagate_mask & mask))
            return target;

        Window root, parent, *children = NULL;
        unsigned int count = 0;
        if (!XQueryTree(ih->display, target, &root, &parent, &children, &count))
            return None;
        if (children)
            XFree(children);
        if (parent == None)
            return target;

        *x += attributes.x + attributes.border_width;
        *y += attributes.y + attributes.border_width;
        target = parent;
    }
}

static unsigned int _uph_plugin_gui_x11_state(uint8_t buttons_down)
{
    unsigned int state = 0;
    if (buttons_down & (1 << NAUI_MOUSE_LEFT)) state |= Button1Mask;
    if (buttons_down & (1 << NAUI_MOUSE_MIDDLE)) state |= Button2Mask;
    if (buttons_down & (1 << NAUI_MOUSE_RIGHT)) state |= Button3Mask;
    return state;
}

static void _uph_plugin_gui_x11_send(Uph_PluginInternalHandle *ih, int type, unsigned int button, int32_t x, int32_t y)
{
    long mask;
    if (type == MotionNotify)
        mask = ih->buttons_down ? (PointerMotionMask | ButtonMotionMask) : PointerMotionMask;
    else
        mask = type == ButtonPress ? ButtonPressMask : ButtonReleaseMask;

    const bool grabbed = ih->buttons_down && ih->input_target;
    int tx, ty;
    Window target = _uph_plugin_gui_target_at(ih, x, y, &tx, &ty);
    if (target != None && !grabbed)
        target = _uph_plugin_gui_x11_listener(ih, target, mask, &tx, &ty);
    if (target == None)
        return;

    XEvent event = { 0 };

    if (type == MotionNotify)
    {
        XMotionEvent *e = &event.xmotion;
        e->type = MotionNotify;
        e->display = ih->display;
        e->window = target;
        e->root = DefaultRootWindow(ih->display);
        e->time = CurrentTime;
        e->x = tx;
        e->y = ty;
        e->x_root = ih->screen_x + x;
        e->y_root = ih->screen_y + y;
        e->state = _uph_plugin_gui_x11_state(ih->buttons_down);
        e->same_screen = True;
    }
    else
    {
        XButtonEvent *e = &event.xbutton;
        e->type = type;
        e->display = ih->display;
        e->window = target;
        e->root = DefaultRootWindow(ih->display);
        e->time = CurrentTime;
        e->x = tx;
        e->y = ty;
        e->x_root = ih->screen_x + x;
        e->y_root = ih->screen_y + y;
        e->state = _uph_plugin_gui_x11_state(ih->buttons_down);
        e->button = button;
        e->same_screen = True;
    }

    if (type == ButtonPress)
        ih->input_target = target;

    XSendEvent(ih->display, target, False, mask, &event);
    XFlush(ih->display);
}
#endif

#if NAUI_LINUX
void uph_plugin_gui_mouse_move(Uph_PluginInternalHandle *handle, int32_t x, int32_t y)
{
    _uph_plugin_gui_x11_send(handle, MotionNotify, 0, x, y);
}

void uph_plugin_gui_mouse_button(Uph_PluginInternalHandle *handle, Naui_MouseButton button, bool down, int32_t x, int32_t y)
{
    if (down)
        handle->buttons_down |= (uint8_t)(1 << button);

    static const unsigned int x11_buttons[NAUI_MOUSE_MAX] = { Button1, Button3, Button2 };
    _uph_plugin_gui_x11_send(handle, down ? ButtonPress : ButtonRelease, x11_buttons[button], x, y);

    if (!down)
        handle->buttons_down &= (uint8_t)~(1 << button);
    if (!handle->buttons_down)
        handle->input_target = 0;
}

void uph_plugin_gui_mouse_scroll(Uph_PluginInternalHandle *handle, int32_t delta, int32_t x, int32_t y)
{
    if (!delta)
        return;

    const unsigned int button = delta > 0 ? Button4 : Button5;
    for (int32_t i = 0; i < abs(delta); i++)
    {
        _uph_plugin_gui_x11_send(handle, ButtonPress, button, x, y);
        _uph_plugin_gui_x11_send(handle, ButtonRelease, button, x, y);
    }
    if (!handle->buttons_down)
        handle->input_target = 0;
}
#elif NAUI_WINDOWS
static void _uph_plugin_gui_win32_set_passthrough(Uph_PluginInternalHandle *ih, bool passthrough)
{
    if (ih->native_input == !passthrough)
        return;

    ih->native_input = !passthrough;
    const LONG_PTR style = GetWindowLongPtrA(ih->window, GWL_EXSTYLE);
    SetWindowLongPtrA(ih->window, GWL_EXSTYLE, passthrough ? (style | WS_EX_TRANSPARENT) : (style & ~(LONG_PTR)WS_EX_TRANSPARENT));
}

void uph_plugin_gui_native_input(Uph_PluginInternalHandle *handle, bool enabled)
{
    _uph_plugin_gui_win32_set_passthrough(handle, !enabled);
    if (!enabled)
        return;

    HWND main_window = (HWND)mgapp_primary_handle();
    POINT cursor;
    if (!GetCursorPos(&cursor) || !ScreenToClient(main_window, &cursor))
        return;
    if (cursor.x == handle->cursor.x && cursor.y == handle->cursor.y)
        return;

    handle->cursor = cursor;
    PostMessageA(main_window, WM_MOUSEMOVE, 0, MAKELPARAM(cursor.x, cursor.y));
}
#endif

static void _uph_plugin_gui_set_visible(Uph_PluginInternalHandle *ih, bool visible)
{
    if (ih->visible == visible)
        return;

    ih->visible = visible;
    ih->origin_dirty = visible;
#if NAUI_LINUX
    ih->buttons_down = 0;
    ih->input_target = 0;
#elif NAUI_WINDOWS
    _uph_plugin_gui_win32_set_passthrough(ih, true);
#endif

    if (!visible && ih->is_clap && ih->clap.gui)
        ih->clap.gui->hide(ih->clap.plugin);

#if NAUI_LINUX
    if (visible)
        XMapWindow(ih->display, ih->window);
    else
        XUnmapWindow(ih->display, ih->window);
    XFlush(ih->display);
#elif NAUI_WINDOWS
    ShowWindow(ih->window, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
#endif

    if (visible && ih->is_clap && ih->clap.gui)
        ih->clap.gui->show(ih->clap.plugin);

    if (!visible)
        _uph_plugin_gui_release_capture(ih);
}

void uph_plugin_gui_closed(Uph_PluginInternalHandle *handle)
{
    _uph_plugin_gui_set_visible(handle, false);
}

void uph_plugin_hide_window(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    if (internal_handle->panel)
        naui_panel_close(internal_handle->panel);
    _uph_plugin_gui_set_visible(internal_handle, false);
}

void uph_plugin_show_window(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    if (internal_handle->visible)
        return;

    if (!internal_handle->panel)
        internal_handle->panel = uph_plugin_editor_create(internal_handle);
    naui_panel_open(internal_handle->panel);
    _uph_plugin_gui_set_visible(internal_handle, true);
}

bool uph_plugin_window_visible(Uph_Plugin *plug)
{
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;
    return internal_handle->visible;
}

static inline void uph_poll_plugin_window_events(Uph_Plugin *plug)
{
#if NAUI_LINUX
    Uph_PluginInternalHandle *internal_handle =
        (Uph_PluginInternalHandle*)plug->internal_handle;

    while (XPending(internal_handle->display) > 0)
    {
        XEvent event;
        XNextEvent(internal_handle->display, &event);
    }
#else
    (void)plug;
#endif
}

void uph_plugin_update(Uph_Plugin *plug)
{
	if (!plug->loaded)
		return;

    uph_poll_plugin_window_events(plug);

    if (!plug->loaded || !plug->internal_handle)
        return;

    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;

    if (!internal_handle->visible)
        return;

    _uph_plugin_gui_capture(internal_handle);

    if (plug->format == UPH_PLUGIN_VST3)
    {
        uph_vst3_pump_run_loop(internal_handle);
        return;
    }

    if (plug->format == UPH_PLUGIN_VST2)
    {
        if (internal_handle->vst2.editor_open)
            _uph_vst2_dispatch(internal_handle->vst2.effect, effEditIdle, 0, 0, NULL, 0.0f);
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
    if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return true;

    Uph_PluginInternalHandle *ih = (Uph_PluginInternalHandle*)list->ctx;
    if (event->type == CLAP_EVENT_PARAM_GESTURE_BEGIN)
        _uph_plugin_mark_touched(ih, ((const clap_event_param_gesture_t*)event)->param_id);
    else if (event->type == CLAP_EVENT_PARAM_VALUE)
        _uph_plugin_mark_touched(ih, ((const clap_event_param_value_t*)event)->param_id);

    return true;
}

void uph_plugin_process(
    Uph_Plugin *plug,
    float **inputs,
    float **outputs,
    uint32_t frame_count,
    double playhead_beat,
    bool is_playing
)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;

    if (!internal_handle)
        return;

    if (plug->format == UPH_PLUGIN_VST3)
    {
        uph_vst3_process(internal_handle, inputs, outputs, frame_count, playhead_beat, is_playing);
        return;
    }

    if (plug->format == UPH_PLUGIN_VST2)
    {
        _uph_vst2_process(internal_handle, inputs, outputs, frame_count, playhead_beat, is_playing);
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
        .ctx = internal_handle,
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
        .tempo = uph_tempo_current_bpm(),
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

bool uph_plugin_save_state(const Uph_Plugin *plug, const Naui_Path path)
{
    Uph_PluginInternalHandle *internal_handle = (Uph_PluginInternalHandle*)plug->internal_handle;
    if (!internal_handle)
        return false;
    if (plug->format == UPH_PLUGIN_VST3)
        return uph_vst3_save_state(internal_handle, path);
    if (plug->format == UPH_PLUGIN_VST2)
        return _uph_vst2_save_state(internal_handle, path);

    if (!internal_handle->clap.plugin)
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
    if (!internal_handle)
        return false;

    if (plug->format == UPH_PLUGIN_VST3)
    {
        bool ok = uph_vst3_load_state(internal_handle, path);
        if (ok)
            plug->params = uph_vst3_get_param_list(plug);
        return ok;
    }

    if (plug->format == UPH_PLUGIN_VST2)
    {
        bool ok = _uph_vst2_load_state(internal_handle, path);
        if (ok)
        {
            AEffect *effect = internal_handle->vst2.effect;
            const uint64_t param_count = (uint64_t)naui_list_len(plug->params);
            for (uint64_t i = 0; i < param_count; i++)
                plug->params[i].current_value = (double)effect->getParameter(effect, (int)plug->params[i].id);
        }
        return ok;
    }

    if (!internal_handle->clap.plugin)
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

    out->path = path;

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

    out->path = path;

    return ok;
}

bool uph_plugin_format_from_path(Naui_Path path, Uph_PluginFormat *format)
{
    static const char *const extensions[UPH_PLUGIN_FORMAT_COUNT] = {
        [UPH_PLUGIN_CLAP] = ".clap",
        [UPH_PLUGIN_VST3] = ".vst3",
        [UPH_PLUGIN_VST2] = UPH_VST2_EXTENSION
    };

    const Naui_StringView extension = naui_file_extension(&path);
    for (Uph_PluginFormat f = 0; f < UPH_PLUGIN_FORMAT_COUNT; f++)
    {
        if (naui_string_view_equals_cstr(extension, extensions[f], false))
        {
            if (format)
                *format = f;
            return true;
        }
    }

    return false;
}

bool uph_get_plugin_info(Naui_Path path, Uph_PluginInfo *out)
{
    memset(out, 0, sizeof(*out));

    uph_com_init_sta_once();

    Uph_PluginFormat format;
    if (!uph_plugin_format_from_path(path, &format))
    {
        fprintf(stderr, "uph info: unrecognized plugin extension for %s\n", path.data);
        return false;
    }

    switch (format)
    {
        case UPH_PLUGIN_CLAP: return uph_get_clap_plugin_info(path, out);
        case UPH_PLUGIN_VST3: return uph_get_vst3_plugin_info_guarded(path, out);
        case UPH_PLUGIN_VST2: return _uph_get_vst2_plugin_info(path, out);
        default:              return false;
    }
}
