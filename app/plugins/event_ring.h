#define UPH_NOTE_RING_CAPACITY  256
#define UPH_PARAM_RING_CAPACITY 512

_Static_assert((UPH_NOTE_RING_CAPACITY  & (UPH_NOTE_RING_CAPACITY  - 1)) == 0, "must be power of two");
_Static_assert((UPH_PARAM_RING_CAPACITY & (UPH_PARAM_RING_CAPACITY - 1)) == 0, "must be power of two");

typedef struct
{
    clap_event_note_t buffer[UPH_NOTE_RING_CAPACITY];
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
}
Uph_NoteEventRing;

typedef struct
{
    clap_event_param_value_t buffer[UPH_PARAM_RING_CAPACITY];
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
}
Uph_ParamEventRing;

static inline void uph_note_ring_init(Uph_NoteEventRing *ring)
{
    atomic_store_explicit(&ring->head, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, 0, memory_order_relaxed);
}

static inline void uph_param_ring_init(Uph_ParamEventRing *ring)
{
    atomic_store_explicit(&ring->head, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, 0, memory_order_relaxed);
}

static inline bool uph_note_ring_push(Uph_NoteEventRing *ring, const clap_event_note_t *ev)
{
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if ((uint32_t)(tail - head) >= UPH_NOTE_RING_CAPACITY)
        return false;

    ring->buffer[tail & (UPH_NOTE_RING_CAPACITY - 1)] = *ev;
    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
    return true;
}

static inline bool uph_param_ring_push(Uph_ParamEventRing *ring, const clap_event_param_value_t *ev)
{
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if ((uint32_t)(tail - head) >= UPH_PARAM_RING_CAPACITY)
        return false;

    ring->buffer[tail & (UPH_PARAM_RING_CAPACITY - 1)] = *ev;
    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
    return true;
}

static inline uint32_t uph_note_ring_size(const Uph_NoteEventRing *ring)
{
    uint32_t tail = atomic_load_explicit((_Atomic uint32_t*)&ring->tail, memory_order_acquire);
    uint32_t head = atomic_load_explicit((_Atomic uint32_t*)&ring->head, memory_order_relaxed);
    return tail - head;
}

static inline uint32_t uph_param_ring_size(const Uph_ParamEventRing *ring)
{
    uint32_t tail = atomic_load_explicit((_Atomic uint32_t*)&ring->tail, memory_order_acquire);
    uint32_t head = atomic_load_explicit((_Atomic uint32_t*)&ring->head, memory_order_relaxed);
    return tail - head;
}

static inline const clap_event_note_t *uph_note_ring_peek(const Uph_NoteEventRing *ring, uint32_t i)
{
    uint32_t head = atomic_load_explicit((_Atomic uint32_t*)&ring->head, memory_order_relaxed);
    return &ring->buffer[(head + i) & (UPH_NOTE_RING_CAPACITY - 1)];
}

static inline const clap_event_param_value_t *uph_param_ring_peek(const Uph_ParamEventRing *ring, uint32_t i)
{
    uint32_t head = atomic_load_explicit((_Atomic uint32_t*)&ring->head, memory_order_relaxed);
    return &ring->buffer[(head + i) & (UPH_PARAM_RING_CAPACITY - 1)];
}

static inline void uph_note_ring_advance(Uph_NoteEventRing *ring, uint32_t count)
{
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    atomic_store_explicit(&ring->head, head + count, memory_order_release);
}

static inline void uph_param_ring_advance(Uph_ParamEventRing *ring, uint32_t count)
{
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    atomic_store_explicit(&ring->head, head + count, memory_order_release);
}