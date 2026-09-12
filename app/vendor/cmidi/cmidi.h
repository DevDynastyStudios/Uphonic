#ifndef CMIDI_H
#define CMIDI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define CMIDI_CC_MODULATION         1
#define CMIDI_CC_VOLUME             7
#define CMIDI_CC_PAN                10
#define CMIDI_CC_EXPRESSION         11
#define CMIDI_CC_SUSTAIN            64
#define CMIDI_CC_PORTAMENTO         65
#define CMIDI_CC_SOSTENUTO          66
#define CMIDI_CC_SOFT_PEDAL         67
#define CMIDI_CC_LOCAL_CONTROL      122
#define CMIDI_CC_ALL_NOTES_OFF      123
#define CMIDI_CC_ALL_SOUND_OFF      120
#define CMIDI_CC_RESET_CONTROLLERS  121

#define CMIDI_MAX_DEVICE_NAME       256
#define CMIDI_MAX_DEVICES           64
#define CMIDI_MAX_ERROR             512

#define CMIDI_CODE_NOTE_OFF         0x80
#define CMIDI_CODE_NOTE_ON          0x90
#define CMIDI_CODE_POLY_PRESSURE    0xA0
#define CMIDI_CODE_CONTROL_CHANGE   0xB0
#define CMIDI_CODE_PROGRAM_CHANGE   0xC0
#define CMIDI_CODE_CHANNEL_PRESSURE 0xD0
#define CMIDI_CODE_PITCH_BEND       0xE0

#define CMIDI_CODE_SYSEX            0xF0
#define CMIDI_CODE_SYSEX_END        0xF7
#define CMIDI_CODE_CLOCK            0xF8
#define CMIDI_CODE_START            0xFA
#define CMIDI_CODE_CONTINUE         0xFB
#define CMIDI_CODE_STOP             0xFC

typedef struct
{
    uint32_t id;
    char name[CMIDI_MAX_DEVICE_NAME];
    int can_input;
    int can_output;
} cmidi_device_t;

typedef enum
{
    CMIDI_NOTE_OFF          = 0x80,
    CMIDI_NOTE_ON           = 0x90,
    CMIDI_POLY_PRESSURE     = 0xA0,
    CMIDI_CONTROL_CHANGE    = 0xB0,
    CMIDI_PROGRAM_CHANGE    = 0xC0,
    CMIDI_CHANNEL_PRESSURE  = 0xD0,
    CMIDI_PITCH_BEND        = 0xE0,
    CMIDI_SYSEX             = 0xF0,
    CMIDI_CLOCK             = 0xF8,
    CMIDI_START             = 0xFA,
    CMIDI_CONTINUE          = 0xFB,
    CMIDI_STOP              = 0xFC,
    CMIDI_UNKNOWN           = 0x00,
    CMIDI_TEMPO_CHANGE      = 0x01,
    CMIDI_END_OF_TRACK      = 0x02,
} cmidi_event_type_t;

typedef struct
{
    const uint8_t* sysex_data;  // valid when type == CMIDI_SYSEX.
    double timestamp_ms;        // time since port was opened
    uint32_t sysex_length;      // includes leading 0xF0 and trailing 0xF7.
    cmidi_event_type_t type;
    uint16_t pitch_bend;
    uint8_t channel;            // 1--16
    uint8_t note;
    uint8_t velocity;
    uint8_t cc_number;
    uint8_t cc_value;
    uint8_t program;
    uint8_t pressure;
} cmidi_event_t;

typedef struct cmidi_file cmidi_file_t;

typedef enum
{
    CMIDI_SCHED_NOTE_OFF = 0x80,
    CMIDI_SCHED_NOTE_ON = 0x90,
    CMIDI_SCHED_POLY_PRESSURE = 0xA0,
    CMIDI_SCHED_CONTROL_CHANGE = 0xB0,
    CMIDI_SCHED_PROGRAM_CHANGE = 0xC0,
    CMIDI_SCHED_CHANNEL_PRESSURE = 0xD0,
    CMIDI_SCHED_PITCH_BEND = 0xE0,
    CMIDI_SCHED_SYSEX = 0xF0,
    CMIDI_SCHED_TEMPO_CHANGE = 0x01,
    CMIDI_SCHED_END_OF_TRACK = 0x02,
} cmidi_sched_type_t;

typedef struct
{
    uint32_t tick;
    cmidi_sched_type_t type;
    uint8_t channel;
    uint8_t data0;
    uint8_t data1;
    uint32_t tempo_us;
    uint32_t sysex_offset;  // Valid for SYSEX events.
    uint32_t sysex_length;  // Includes 0xF0 and 0xF7.
} cmidi_sched_event_t;

typedef struct
{
    uint32_t tick;
    double timestamp_ms;
    cmidi_sched_type_t type;
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
    uint8_t cc_number;
    uint8_t cc_value;
    int pitch_bend;
    uint8_t program;
    uint32_t tempo_us;
    const uint8_t* sysex_data;  // Valid during the callback for SYSEX events.
    uint32_t sysex_length;
} cmidi_fired_event_t;

// Input callback. Fires on a background thread.
typedef void (*cmidi_input_callback_t)(const cmidi_event_t* event, void* userdata);

// Opaque port handle
typedef struct cmidi_port cmidi_port_t;

int cmidi_get_devices(cmidi_device_t* out, int max_count);

// Open a MIDI input port. Returns NULL on failure.
cmidi_port_t* cmidi_open_input(uint32_t device_id, cmidi_input_callback_t callback, void* userdata);

// Open a MIDI output port. Returns NULL on failure.
cmidi_port_t* cmidi_open_output(uint32_t device_id);

// Close a port. Blocks until the read thread exits for input ports. Safe with NULL.
void cmidi_close(cmidi_port_t* port);

const char* cmidi_port_name(const cmidi_port_t* port);
 
// Returns 1 if this is an input port, 0 if output.
int cmidi_port_is_input(const cmidi_port_t* port);

// Connect input -> output for automatic thru forwarding.
void cmidi_set_thru(cmidi_port_t* input, cmidi_port_t* output);

// Remove the thru connection from an input port.
void cmidi_clear_thru(cmidi_port_t* input);

// Play a note. channel: 1-16, note: 0-127, velocity: 1-127.
int cmidi_play_note(cmidi_port_t* port, uint8_t channel, uint8_t note, uint8_t velocity);

// Stop a note. channel: 1-16, note: 0-127.
int cmidi_stop_note(cmidi_port_t* port, uint8_t channel, uint8_t note);

// Send CC 123 (All Notes Off) on all 16 channels.
void cmidi_stop_all_notes(cmidi_port_t* port);

int cmidi_send(cmidi_port_t* port, const uint8_t* bytes, uint8_t length);

// Send a SysEx message. `data` must start with 0xF0 and end with 0xF7.
// Returns 1 on success, 0 if the message is malformed or the send fails.
int cmidi_send_sysex(cmidi_port_t* port, const uint8_t* data, uint32_t length);

// Send a Control Change message. channel: 1-16, cc: 0-127, value: 0-127.
int cmidi_control_change(cmidi_port_t* port, uint8_t channel, uint8_t cc, uint8_t value);

// Send a Program Change message. channel: 1-16, program: 0-127.
int cmidi_program_change(cmidi_port_t* port, uint8_t channel, uint8_t program);

const char* cmidi_last_error(void);

// Load MIDI file from disk. Returns NULL on failure.
// Free with cmidi_file_free().
cmidi_file_t* cmidi_file_load(const char* path);

// Load from a buffer already in memory.
cmidi_file_t* cmidi_file_load_memory(const uint8_t* data, size_t size);

// Event array for cmidi_scheduler_set_events()
const cmidi_sched_event_t* cmidi_file_events(const cmidi_file_t* file);

// Returns the number of events in file
int cmidi_file_event_count(const cmidi_file_t* file);

uint16_t cmidi_file_ticks_per_beat(const cmidi_file_t* file);

// Initial tempo in us/beat. Defaults to 500000 (120 BPM).
uint32_t cmidi_file_initial_tempo(const cmidi_file_t* file);

// SMF format of the source file: 0, 1, or 2.
// Files made with cmidi_file_create() report 0 until cmidi_file_set_format() is called.
uint16_t cmidi_file_format(const cmidi_file_t* file);

// Select the SMF format cmidi_file_save() writes.
// Note: Format 2 is not fully supported, will get merged down to format 1.
void cmidi_file_set_format(cmidi_file_t* file, uint16_t format);

// Create an empty file. ticks_per_beat typically 480.
cmidi_file_t* cmidi_file_create(uint16_t ticks_per_beat);

// Set tempo in us/beat. Defaults to 500000.
void cmidi_file_set_tempo(cmidi_file_t* file, uint32_t tempo_us);

// Add a note as NoteOn + NoteOff pair.
// start_tick: absolute tick position, channel: 0-15, note: 0-127, velocity: 1-127.
void cmidi_file_add_note(cmidi_file_t* file, uint8_t channel, uint32_t start_tick, uint8_t note, uint8_t velocity, uint32_t duration_ticks);

// Add a raw scheduler event
void cmidi_file_add_event(cmidi_file_t* file, cmidi_sched_event_t event);

// Add a complete SysEx message starting with 0xF0 and ending with 0xF7.
// Bytes are copied into the file's own pool.
void cmidi_file_add_sysex(cmidi_file_t* file, uint32_t tick, const uint8_t* data, uint32_t length);

// Byte pool backing sysex_data in cmidi_file_events().
// May move when events or SysEx data are added, re-fetch as needed.
const uint8_t* cmidi_file_sysex_pool(const cmidi_file_t* file);

// Write as an SMF file, in the format selected by cmidi_file_set_format()
// (format 0 by default). Returns 1 on success, 0 on failure.
int cmidi_file_save(const cmidi_file_t* file, const char* path);

// Free. Safe with NULL.
void cmidi_file_free(cmidi_file_t* file);

// Convert BPM to microseconds per beat.
uint32_t cmidi_bpm_to_tempo_us(double bpm);

// Convert microseconds per beat to BPM.
double cmidi_tempo_us_to_bpm(uint32_t tempo_us);

// Convert ticks to milliseconds at a fixed tempo.
double cmidi_ticks_to_ms(uint32_t ticks, uint16_t ticks_per_beat, uint32_t tempo_us);

// Convert milliseconds to ticks at a fixed tempo.
uint32_t cmidi_ms_to_ticks(double ms, uint16_t ticks_per_beat, uint32_t tempo_us);

typedef void (*cmidi_scheduler_callback_t)(const cmidi_fired_event_t* event, void* userdata);

// Fires when playback finishes naturally, not when stopped manually.
typedef void (*cmidi_scheduler_end_callback_t)(void* userdata);

typedef struct cmidi_scheduler cmidi_scheduler_t;

// Create a scheduler. port may be NULL for callback-only mode.
cmidi_scheduler_t* cmidi_scheduler_create(cmidi_port_t* port);

// Destroy the scheduler, stopping playback if needed.
void cmidi_scheduler_destroy(cmidi_scheduler_t* sched);

// Load events from a MIDI file.
int cmidi_scheduler_load_file(cmidi_scheduler_t* sched, const struct cmidi_file* file);

// Load and copy events from an array.
int cmidi_scheduler_set_events(cmidi_scheduler_t* sched, const cmidi_sched_event_t* events, int count, uint16_t ticks_per_beat, const uint8_t* sysex_pool);

// Get the scheduler's internal SysEx byte pool.
const uint8_t* cmidi_scheduler_sysex_pool(const cmidi_scheduler_t* sched);

// Set the per-event callback. Pass NULL to disable.
void cmidi_scheduler_set_callback(cmidi_scheduler_t* sched, cmidi_scheduler_callback_t callback, void* userdata);

// Set the natural-completion callback.
void cmidi_scheduler_set_end_callback(cmidi_scheduler_t* sched, cmidi_scheduler_end_callback_t callback, void* userdata);

// Enable looping and set the loop length in ticks.
void cmidi_scheduler_set_loop(cmidi_scheduler_t* sched, int enabled, uint32_t loop_end_tick);

// Start playback on a background thread.
int cmidi_scheduler_start(cmidi_scheduler_t* sched);

// Stop playback and send All Notes Off.
void cmidi_scheduler_stop(cmidi_scheduler_t* sched);

// Wait for playback to finish naturally.
void cmidi_scheduler_wait(cmidi_scheduler_t* sched);

// Return 1 if playing, 0 if stopped.
int cmidi_scheduler_is_playing(const cmidi_scheduler_t* sched);

// Return the current playback position in ticks, or 0 if stopped.
uint32_t cmidi_scheduler_current_tick(const cmidi_scheduler_t* sched);

// Returns wall-clock time in milliseconds.
double cmidi_time_now_ms(void);

// Sleep for approximately `ms` milliseconds. 
void cmidi_time_sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif // CMIDI_H

#ifdef CMIDI_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void cmidi_set_error(const char* fmt, ...);
void cmidi_dispatch_input(cmidi_port_t* port, cmidi_input_callback_t callback, void* userdata, const uint8_t* bytes, uint8_t length, double timestamp_ms);
void cmidi_dispatch_sysex_input(cmidi_port_t* port, cmidi_input_callback_t callback, void* userdata, const uint8_t* bytes, uint32_t length, double timestamp_ms);

int cmidi_platform_enumerate(cmidi_device_t* out, int max_count);
cmidi_port_t* cmidi_platform_open_input(uint32_t device_id, cmidi_input_callback_t callback, void* userdata);
cmidi_port_t* cmidi_platform_open_output(uint32_t device_id);
int cmidi_platform_send(cmidi_port_t* port, const uint8_t* bytes, uint8_t length);

// Send a complete SysEx message (0xF0 ... 0xF7). Returns 1 on success.
int cmidi_platform_send_sysex(cmidi_port_t* port, const uint8_t* bytes, uint32_t length);

void cmidi_platform_close(cmidi_port_t* port);
const char* cmidi_platform_port_name(const cmidi_port_t* port);
int cmidi_platform_port_is_input(const cmidi_port_t* port);

static char s_error[CMIDI_MAX_ERROR];

void cmidi_set_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_error, sizeof(s_error), fmt, args);
    va_end(args);
}

#define CMIDI_MAX_THRU 16

static struct
{
    cmidi_port_t* input;
    cmidi_port_t* output;
} s_thru[CMIDI_MAX_THRU];

static int s_thru_count = 0;

static cmidi_port_t* find_thru_output(cmidi_port_t* input)
{
    for (int i = 0; i < s_thru_count; ++i)
    {
        if (s_thru[i].input == input)
            return s_thru[i].output;
    }

    return NULL;
}

void cmidi_dispatch_input(cmidi_port_t* port, cmidi_input_callback_t callback, void* userdata, const uint8_t* bytes, uint8_t length, double timestamp_ms)
{
    if (length == 0)
        return;

    cmidi_port_t* thru = find_thru_output(port);
    if (thru)
        cmidi_platform_send(thru, bytes, length);

    if (!callback)
        return;

    cmidi_event_t event = {
        .timestamp_ms = timestamp_ms,
        .channel = (bytes[0] & 0x0F) + 1
    };

    switch (bytes[0])
    {
        case CMIDI_CODE_CLOCK:
            event.type = CMIDI_CLOCK;
            break;

        case CMIDI_CODE_START:
            event.type = CMIDI_START;
            break;

        case CMIDI_CODE_CONTINUE:
            event.type = CMIDI_CONTINUE;
            break;

        case CMIDI_CODE_STOP:
            event.type = CMIDI_STOP;    
            break;

        default:
        {
            uint8_t status = bytes[0] & CMIDI_CODE_SYSEX;
            uint8_t data1 = length >= 2 ? bytes[1] : 0;
            uint8_t data2 = length >= 3 ? bytes[2] : 0;
            switch (status)
            {
                case CMIDI_CODE_NOTE_ON:
                    event.type = data2 > 0 ? CMIDI_NOTE_ON : CMIDI_NOTE_OFF;
                    event.note = data1;
                    event.velocity = data2;
                    break;
            
                case CMIDI_CODE_NOTE_OFF:
                    event.type = CMIDI_NOTE_OFF;
                    event.note = data1;
                    event.velocity = data2;
                    break;
            
                case CMIDI_CODE_CONTROL_CHANGE:
                    event.type = CMIDI_CONTROL_CHANGE;
                    event.cc_number = data1;
                    event.cc_value = data2;
                    break;
            
                case CMIDI_CODE_PROGRAM_CHANGE:
                    event.type = CMIDI_PROGRAM_CHANGE;
                    event.program = data1;
                    break;
            
                case CMIDI_CODE_CHANNEL_PRESSURE:
                    event.type = CMIDI_CHANNEL_PRESSURE;
                    event.pressure = data1;
                    break;
            
                case CMIDI_CODE_POLY_PRESSURE:
                    event.type = CMIDI_POLY_PRESSURE;
                    event.note = data1;
                    event.pressure = data2;
                    break;
            
                case CMIDI_CODE_PITCH_BEND:
                    event.type = CMIDI_PITCH_BEND;
                    event.pitch_bend = (int)(data1 | ((int)data2 << 7)) - 8192;
                    break;
            
                default:
                    event.type = CMIDI_UNKNOWN;
                    break;
            }

            break;
        }
    }

    callback(&event, userdata);
}

void cmidi_dispatch_sysex_input(cmidi_port_t* port, cmidi_input_callback_t callback, void* userdata, const uint8_t* bytes, uint32_t length, double timestamp_ms)
{
    if (!bytes || length == 0)
        return;

    cmidi_port_t* thru = find_thru_output(port);
    if (thru)
        cmidi_platform_send_sysex(thru, bytes, length);

    if (!callback)
        return;

    cmidi_event_t event = {
        .type = CMIDI_SYSEX,
        .timestamp_ms = timestamp_ms,
        .sysex_data = bytes,
        .sysex_length = length
    };

    callback(&event, userdata);
}

#pragma region Public API
int cmidi_get_devices(cmidi_device_t* out, int max_count)
{
    if (!out || max_count <= 0)
        return 0;

    s_error[0] = '\0';
    return cmidi_platform_enumerate(out, max_count);
}

cmidi_port_t* cmidi_open_input(uint32_t device_id, cmidi_input_callback_t callback, void* userdata)
{
    s_error[0] = '\0';
    if (!callback)
    {
        cmidi_set_error("cmidi_open_input: callback must not be NULL");
        return NULL;
    }

    return cmidi_platform_open_input(device_id, callback, userdata);
}

cmidi_port_t* cmidi_open_output(uint32_t device_id)
{
    s_error[0] = '\0';
    return cmidi_platform_open_output(device_id);
}

void cmidi_close(cmidi_port_t* port)
{
    if (!port)
        return;

    cmidi_clear_thru(port);
    cmidi_platform_close(port);
}

const char* cmidi_port_name(const cmidi_port_t* port)
{
    return cmidi_platform_port_name(port);
}

int cmidi_port_is_input(const cmidi_port_t* port)
{
    return cmidi_platform_port_is_input(port);
}

void cmidi_set_thru(cmidi_port_t* input, cmidi_port_t* output)
{
    if (!input || !output)
        return;

    for (int i = 0; i < s_thru_count; ++i)
    {
        if (s_thru[i].input == input)
        {
            s_thru[i].output = output;
            return;
        }
    }
    if (s_thru_count < CMIDI_MAX_THRU)
    {
        s_thru[s_thru_count].input = input;
        s_thru[s_thru_count].output = output;
        ++s_thru_count;
    }
}

void cmidi_clear_thru(cmidi_port_t* input)
{
    for (int i = 0; i < s_thru_count; ++i)
    {
        if (s_thru[i].input != input)
            continue;

        for (int j = i; j < s_thru_count - 1; ++j)
        {
            s_thru[j] = s_thru[j + 1];
        }

        --s_thru_count;
        return;
    }
}

int cmidi_play_note(cmidi_port_t* port, uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (!port || channel < 1 || channel > 16)
        return 0;

    uint8_t b[3] = { (uint8_t)(CMIDI_CODE_NOTE_ON | ((channel - 1) & 0x0F)), note, velocity };
    return cmidi_send(port, b, 3);
}

int cmidi_stop_note(cmidi_port_t* port, uint8_t channel, uint8_t note)
{
    if (!port || channel < 1 || channel > 16)
        return 0;

    uint8_t b[3] = { (uint8_t)(CMIDI_CODE_NOTE_OFF | ((channel - 1) & 0x0F)), note, 0 };
    return cmidi_send(port, b, 3);
}

void cmidi_stop_all_notes(cmidi_port_t* port)
{
    if (!port)
        return;

    uint8_t ch;
    for (ch = 0; ch < 16; ++ch)
    {
        uint8_t b[3] = { (uint8_t)(CMIDI_CODE_CONTROL_CHANGE | ch), 0x7B, 0x00 };
        cmidi_send(port, b, 3);
    }
}

int cmidi_send(cmidi_port_t* port, const uint8_t* bytes, uint8_t length)
{
    if (!port || !bytes || length == 0 || length > 3)
        return 0;

    return cmidi_platform_send(port, bytes, length);
}

int cmidi_send_sysex(cmidi_port_t* port, const uint8_t* data, uint32_t length)
{
    if (!port || !data || length < 2 || data[0] != CMIDI_CODE_SYSEX || data[length - 1] != CMIDI_CODE_SYSEX_END)
        return 0;

    return cmidi_platform_send_sysex(port, data, length);
}

int cmidi_control_change(cmidi_port_t* port, uint8_t channel, uint8_t cc, uint8_t value)
{
    if (!port || channel < 1 || channel > 16)
        return 0;

    uint8_t b[3] = { (uint8_t)(CMIDI_CODE_CONTROL_CHANGE | ((channel - 1) & 0x0F)), cc, value };
    return cmidi_send(port, b, 3);
}

int cmidi_program_change(cmidi_port_t* port, uint8_t channel, uint8_t program)
{
    if (!port || channel < 1 || channel > 16)
        return 0;

    uint8_t b[2] = { (uint8_t)(CMIDI_CODE_PROGRAM_CHANGE | ((channel - 1) & 0x0F)), program };
    return cmidi_send(port, b, 2);
}

const char* cmidi_last_error(void)
{
    return s_error[0] ? s_error : NULL;
}

#if defined(LINUX) || defined(__linux__)

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

struct cmidi_port
{
    cmidi_input_callback_t callback;
    void* userdata;
    snd_seq_t* seq;
    pthread_t thread;
    int is_input;
    int local_port;
    volatile int running;
    char name[CMIDI_MAX_DEVICE_NAME];
};

static double alsa_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

typedef struct
{
    int client;
    int port;
    char name[CMIDI_MAX_DEVICE_NAME];
} AlsaAddr;

static int find_alsa_addr(snd_seq_t* seq, uint32_t target_id, AlsaAddr* out, int want_input)
{
    snd_seq_client_info_t* cinfo;
    snd_seq_port_info_t* pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    uint32_t idx = 0;
    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(seq, cinfo) >= 0)
    {
        int client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0)
        {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            int is_in = (caps & SND_SEQ_PORT_CAP_READ) && (caps & SND_SEQ_PORT_CAP_SUBS_READ);
            int is_out = (caps & SND_SEQ_PORT_CAP_WRITE) && (caps & SND_SEQ_PORT_CAP_SUBS_WRITE);

            if ((want_input && is_in) || (!want_input && is_out))
            {
                if (idx == target_id)
                {
                    out->client = client;
                    out->port = snd_seq_port_info_get_port(pinfo);
                    snprintf(out->name, CMIDI_MAX_DEVICE_NAME, "%s: %s", snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo));
                    return 1;
                }

                ++idx;
            }
        }
    }

    return 0;
}

static void* alsa_read_thread(void* arg)
{
    cmidi_port_t* port = (cmidi_port_t*)arg;
    struct pollfd pfds[16];
    int npfds = snd_seq_poll_descriptors_count(port->seq, POLLIN);
    if (npfds > 16)
        npfds = 16;

    snd_seq_poll_descriptors(port->seq, pfds, (unsigned)npfds, POLLIN);
    while (port->running)
    {
        if (poll(pfds, (nfds_t)npfds, 10) <= 0)
            continue;

        snd_seq_event_t* event = NULL;
        while (snd_seq_event_input(port->seq, &event) > 0 && event)
        {
            uint8_t bytes[3] = {0, 0, 0};
            uint8_t length = 0;
            double ts = alsa_now_ms();

            switch (event->type)
            {
                case SND_SEQ_EVENT_NOTEON:
                    bytes[0] = (uint8_t)(CMIDI_CODE_NOTE_ON | (event->data.note.channel & 0x0F));
                    bytes[1] = event->data.note.note;
                    bytes[2] = event->data.note.velocity;
                    length = 3;
                    break;

                case SND_SEQ_EVENT_NOTEOFF:
                    bytes[0] = (uint8_t)(CMIDI_CODE_NOTE_OFF | (event->data.note.channel & 0x0F));
                    bytes[1] = event->data.note.note;
                    bytes[2] = event->data.note.velocity;
                    length = 3;
                    break;

                case SND_SEQ_EVENT_CONTROLLER:
                    bytes[0] = (uint8_t)(CMIDI_CODE_CONTROL_CHANGE | (event->data.control.channel & 0x0F));
                    bytes[1] = (uint8_t)event->data.control.param;
                    bytes[2] = (uint8_t)event->data.control.value;
                    length = 3;
                    break;

                case SND_SEQ_EVENT_PITCHBEND:
                {
                    int v = event->data.control.value + 8192;
                    bytes[0] = (uint8_t)(CMIDI_CODE_PITCH_BEND | (event->data.control.channel & 0x0F));
                    bytes[1] = (uint8_t)(v & 0x7F);
                    bytes[2] = (uint8_t)((v >> 7) & 0x7F);
                    length = 3;
                    break;
                }

                case SND_SEQ_EVENT_PGMCHANGE:
                    bytes[0] = (uint8_t)(CMIDI_CODE_PROGRAM_CHANGE | (event->data.control.channel & 0x0F));
                    bytes[1] = (uint8_t)event->data.control.value;
                    length = 2;
                    break;

                case SND_SEQ_EVENT_CHANPRESS:
                    bytes[0] = (uint8_t)(CMIDI_CODE_CHANNEL_PRESSURE | (event->data.control.channel & 0x0F));
                    bytes[1] = (uint8_t)event->data.control.value;
                    length = 2;
                    break;

                case SND_SEQ_EVENT_KEYPRESS:
                    bytes[0] = (uint8_t)(CMIDI_CODE_POLY_PRESSURE | (event->data.note.channel & 0x0F));
                    bytes[1] = event->data.note.note;
                    bytes[2] = event->data.note.velocity;
                    length = 3;
                    break;

                case SND_SEQ_EVENT_CLOCK:
                    bytes[0] = CMIDI_CODE_CLOCK;
                    length = 1;
                    break;

                case SND_SEQ_EVENT_START:
                    bytes[0] = CMIDI_CODE_START;
                    length = 1;
                    break;

                case SND_SEQ_EVENT_CONTINUE:
                    bytes[0] = CMIDI_CODE_CONTINUE;
                    length = 1;
                    break;

                case SND_SEQ_EVENT_STOP:
                    bytes[0] = CMIDI_CODE_STOP;
                    length = 1;
                    break;

                case SND_SEQ_EVENT_SYSEX:
                {
                    const uint8_t* data = (const uint8_t*)event->data.ext.ptr;
                    unsigned int sxlen = event->data.ext.len;
                    if (data && sxlen > 0)
                        cmidi_dispatch_sysex_input(port, port->callback, port->userdata, data, (uint32_t)sxlen, ts);

                    continue;
                }

                default:
                    break;
            }

            if (length > 0)
                cmidi_dispatch_input(port, port->callback, port->userdata, bytes, length, ts);
        }
    }

    return NULL;
}

int cmidi_platform_enumerate(cmidi_device_t* out, int max_count)
{
    snd_seq_t* seq;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    {
        cmidi_set_error("snd_seq_open failed during enumeration");
        return 0;
    }

    snd_seq_client_info_t* cinfo;
    snd_seq_port_info_t* pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    int count = 0;
    while (snd_seq_query_next_client(seq, cinfo) >= 0 && count < max_count)
    {
        int client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0 && count < max_count)
        {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            int has_in = (caps & SND_SEQ_PORT_CAP_READ) && (caps & SND_SEQ_PORT_CAP_SUBS_READ);
            if (!has_in)
                continue;

            out[count].id = (uint32_t)count;
            out[count].can_input = 1;
            out[count].can_output = 0;
            snprintf(out[count].name, CMIDI_MAX_DEVICE_NAME, "%s: %s", snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo));
            ++count;
        }
    }

    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0 && count < max_count)
    {
        int client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0 && count < max_count)
        {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            int has_out = (caps & SND_SEQ_PORT_CAP_WRITE) && (caps & SND_SEQ_PORT_CAP_SUBS_WRITE);
            if (!has_out)
                continue;

            out[count].id = (uint32_t)count;
            out[count].can_input = 0;
            out[count].can_output = 1;
            snprintf(out[count].name, CMIDI_MAX_DEVICE_NAME, "%s: %s", snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo));
            ++count;
        }
    }

    snd_seq_close(seq);
    return count;
}

cmidi_port_t* cmidi_platform_open_input(uint32_t device_id, cmidi_input_callback_t callback, void* userdata)
{
    cmidi_port_t* port = (cmidi_port_t*)calloc(1, sizeof(*port));
    if (!port)
    {
        cmidi_set_error("Out of memory");
        return NULL;
    }

    port->is_input = 1;
    port->callback = callback;
    port->userdata = userdata;
    if (snd_seq_open(&port->seq, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
    {
        cmidi_set_error("snd_seq_open failed");
        free(port);
        return NULL;
    }

    snd_seq_set_client_name(port->seq, "cmidi");
    port->local_port = snd_seq_create_simple_port(port->seq, "Input", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);
    if (port->local_port < 0)
    {
        cmidi_set_error("Failed to create ALSA input port");
        snd_seq_close(port->seq);
        free(port);
        return NULL;
    }

    AlsaAddr src;
    if (!find_alsa_addr(port->seq, device_id, &src, 1))
    {
        cmidi_set_error("MIDI input device %u not found", (unsigned)device_id);
        snd_seq_close(port->seq);
        free(port);
        return NULL;
    }

    snd_seq_port_subscribe_t* sub;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_addr_t sender = { (unsigned char)src.client, (unsigned char)src.port };
    snd_seq_addr_t dest = { (unsigned char)snd_seq_client_id(port->seq), (unsigned char)port->local_port };
    snd_seq_port_subscribe_set_sender(sub, &sender);
    snd_seq_port_subscribe_set_dest(sub, &dest);

    if (snd_seq_subscribe_port(port->seq, sub) < 0)
    {
        cmidi_set_error("snd_seq_subscribe_port failed");
        snd_seq_close(port->seq);
        free(port);
        return NULL;
    }

    snprintf(port->name, CMIDI_MAX_DEVICE_NAME, "%s", src.name);
    port->running = 1;
    if (pthread_create(&port->thread, NULL, alsa_read_thread, port) != 0)
    {
        cmidi_set_error("pthread_create failed");
        port->running = 0;
        snd_seq_close(port->seq);
        free(port);
        return NULL;
    }

    return port;
}

cmidi_port_t* cmidi_platform_open_output(uint32_t device_id)
{
    cmidi_port_t* port = (cmidi_port_t*)calloc(1, sizeof(*port));
    if (!port)
    {
        cmidi_set_error("Out of memory");
        return NULL;
    }

    port->is_input = 0;
    if (snd_seq_open(&port->seq, "default", SND_SEQ_OPEN_OUTPUT, 0) < 0)
    {
        cmidi_set_error("snd_seq_open failed");
        free(port);
        return NULL;
    }

    snd_seq_set_client_name(port->seq, "cmidi");
    port->local_port = snd_seq_create_simple_port(port->seq, "Output", SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_APPLICATION);
    if (port->local_port < 0)
    {
        cmidi_set_error("Failed to create ALSA output port");
        snd_seq_close(port->seq);
        free(port);
        return NULL;
    }

    AlsaAddr dst;
    if (!find_alsa_addr(port->seq, device_id, &dst, 0))
    {
        cmidi_set_error("MIDI output device %u not found", (unsigned)device_id);
        snd_seq_close(port->seq);
        free(port);
        return NULL;
    }

    snd_seq_port_subscribe_t* sub;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_addr_t sender = { (unsigned char)snd_seq_client_id(port->seq), (unsigned char)port->local_port };
    snd_seq_addr_t dest = { (unsigned char)dst.client, (unsigned char)dst.port };
    snd_seq_port_subscribe_set_sender(sub, &sender);
    snd_seq_port_subscribe_set_dest(sub, &dest);

    if (snd_seq_subscribe_port(port->seq, sub) < 0)
    {
        cmidi_set_error("snd_seq_subscribe_port failed");
        snd_seq_close(port->seq);
        free(port);
        return NULL;
    }

    snprintf(port->name, CMIDI_MAX_DEVICE_NAME, "%s", dst.name);
    return port;
}

int cmidi_platform_send(cmidi_port_t* port, const uint8_t* bytes, uint8_t length)
{
    if (!port || port->is_input || length == 0)
        return 0;

    snd_seq_event_t event;
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, port->local_port);
    snd_seq_ev_set_subs(&event);
    snd_seq_ev_set_direct(&event);

    uint8_t status = bytes[0] & CMIDI_CODE_SYSEX;
    uint8_t ch = bytes[0] & 0x0F;
    if(length >= 3)
    {
        if (status == CMIDI_CODE_NOTE_ON)
            snd_seq_ev_set_noteon(&event, ch, bytes[1], bytes[2]);
        else if (status == CMIDI_CODE_NOTE_OFF)
            snd_seq_ev_set_noteoff(&event, ch, bytes[1], bytes[2]);
        else if (status == CMIDI_CODE_CONTROL_CHANGE)
            snd_seq_ev_set_controller(&event, ch, bytes[1], bytes[2]);
        else if (status == CMIDI_CODE_PITCH_BEND)
        {
            int v = bytes[1] | ((int)bytes[2] << 7);
            snd_seq_ev_set_pitchbend(&event, ch, v - 8192);
        }
        else if (status == CMIDI_CODE_POLY_PRESSURE)
        {
            event.type = SND_SEQ_EVENT_KEYPRESS;
            event.data.note.channel = ch;
            event.data.note.note = bytes[1];
            event.data.note.velocity = bytes[2];
        }
        else
            return 0;
    }
    else if(length >= 2)
    {
        if (status == CMIDI_CODE_PROGRAM_CHANGE)
            snd_seq_ev_set_pgmchange(&event, ch, bytes[1]);
        else if (status == CMIDI_CODE_CHANNEL_PRESSURE)
            snd_seq_ev_set_chanpress(&event, ch, bytes[1]);
        else
            return 0;
    }
    else
        return 0;

    return snd_seq_event_output_direct(port->seq, &event) >= 0 ? 1 : 0;
}

int cmidi_platform_send_sysex(cmidi_port_t* port, const uint8_t* bytes, uint32_t length)
{
    if (!port || port->is_input || !bytes || length == 0)
        return 0;

    snd_seq_event_t event;
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, port->local_port);
    snd_seq_ev_set_subs(&event);
    snd_seq_ev_set_direct(&event);
    snd_seq_ev_set_sysex(&event, length, (void*)bytes);
    return snd_seq_event_output_direct(port->seq, &event) >= 0 ? 1 : 0;
}

void cmidi_platform_close(cmidi_port_t* port)
{
    if (!port)
        return;
        
    if (port->is_input)
    {
        port->running = 0;
        pthread_join(port->thread, NULL);
    }

    if (port->seq)
        snd_seq_close(port->seq);

    free(port);
}

const char* cmidi_platform_port_name(const cmidi_port_t* port)
{
    return port ? port->name : NULL;
}

int cmidi_platform_port_is_input(const cmidi_port_t* port)
{
    return port ? port->is_input : 0;
}

static struct timespec s_origin;
static int s_initialised = 0;

static void ensure_init(void)
{
    if (s_initialised)
        return;

    clock_gettime(CLOCK_MONOTONIC, &s_origin);
    s_initialised = 1;
}

double cmidi_time_now_ms(void)
{
    ensure_init();
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double secs = (double)(now.tv_sec - s_origin.tv_sec);
    double ns = (double)(now.tv_nsec - s_origin.tv_nsec);
    return secs * 1000.0 + ns / 1e6;
}

void cmidi_time_sleep_ms(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

typedef pthread_t cmidi_thread_t;
typedef pthread_mutex_t cmidi_mutex_t;
typedef pthread_cond_t cmidi_cond_t;

static void mutex_init(cmidi_mutex_t* m)
{
    pthread_mutex_init(m, NULL);
}

static void mutex_lock(cmidi_mutex_t* m)
{
    pthread_mutex_lock(m);
}

static void mutex_unlock(cmidi_mutex_t* m)
{
    pthread_mutex_unlock(m);
}

static void mutex_destroy(cmidi_mutex_t* m)
{
    pthread_mutex_destroy(m);
}

static void cond_init(cmidi_cond_t* c)
{
    pthread_cond_init(c, NULL);
}

static void cond_signal(cmidi_cond_t* c, cmidi_mutex_t* m)
{
    (void)m;
    pthread_cond_broadcast(c);
}

static void cond_wait(cmidi_cond_t* c, cmidi_mutex_t* m)
{
    pthread_cond_wait(c, m);
}

static void cond_destroy(cmidi_cond_t* c)
{
    pthread_cond_destroy(c);
}

static void* thread_proc(void* arg);

static int thread_start(cmidi_thread_t* t, void* arg)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    int r = pthread_create(t, &attr, thread_proc, arg);
    if (r != 0)
        r = pthread_create(t, NULL, thread_proc, arg);

    pthread_attr_destroy(&attr);
    return r == 0;
}

static void thread_join(cmidi_thread_t t)
{
    pthread_join(t, NULL);
}

#elif defined(_WIN32) || defined(WIN32)

#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

struct cmidi_port
{
    cmidi_input_callback_t callback;
    uint8_t* sysex_buf;
    void* userdata;
    union { HMIDIIN in; HMIDIOUT out; } handle;
    MIDIHDR sysex_hdr;
    uint32_t sysex_len;
    uint32_t sysex_cap;
    int is_input;
    uint8_t sysex_hdr_buf[1024];
    char name[CMIDI_MAX_DEVICE_NAME];
};

static void win_sysex_append(cmidi_port_t* port, uint8_t b)
{
    if (port->sysex_len >= port->sysex_cap)
    {
        uint32_t newcap = port->sysex_cap ? port->sysex_cap * 2 : 256;
        uint8_t* buf = (uint8_t*)realloc(port->sysex_buf, newcap);
        if (!buf)
        {
            port->sysex_len = 0;
            return;
        }

        port->sysex_buf = buf;
        port->sysex_cap = newcap;
    }
    
    port->sysex_buf[port->sysex_len++] = b;
}

static void CALLBACK winmm_input_proc(HMIDIIN hMidi, UINT msg, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2)
{
    cmidi_port_t* port = (cmidi_port_t*)instance;
    if (!port)
        return;

    if (msg == MIM_LONGDATA)
    {
        MIDIHDR* hdr = (MIDIHDR*)param1;
        if (hdr->dwBytesRecorded > 0)
        {
            for (DWORD k = 0; k < hdr->dwBytesRecorded; ++k)
            {
                uint8_t b = (uint8_t)hdr->lpData[k];
                win_sysex_append(port, b);
                if (b == CMIDI_CODE_SYSEX_END)
                {
                    cmidi_dispatch_sysex_input(port, port->callback, port->userdata, port->sysex_buf, port->sysex_len, (double)param2);
                    port->sysex_len = 0;
                }
            }
        }

        hdr->dwBytesRecorded = 0;
        midiInAddBuffer(hMidi, hdr, sizeof(MIDIHDR));
        return;
    }

    if (msg != MIM_DATA)
        return;

    uint8_t bytes[3];
    bytes[0] = (uint8_t)( param1 & 0xFF);
    bytes[1] = (uint8_t)((param1 >> 8) & 0xFF);
    bytes[2] = (uint8_t)((param1 >> 16) & 0xFF);

    uint8_t status = bytes[0] & CMIDI_CODE_SYSEX;
    uint8_t length;
    if (bytes[0] >= CMIDI_CODE_CLOCK)
        length = 1;
    else if (status == CMIDI_CODE_PROGRAM_CHANGE || status == CMIDI_CODE_CHANNEL_PRESSURE)
        length = 2;
    else
        length = 3;

    cmidi_dispatch_input(port, port->callback, port->userdata, bytes, length, (double)param2);
}

int cmidi_platform_enumerate(cmidi_device_t* out, int max_count)
{
    int count = 0;
    UINT in_n = midiInGetNumDevs();
    for (UINT i = 0; i < in_n && count < max_count; ++i)
    {
        MIDIINCAPSA caps;
        if (midiInGetDevCapsA(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
            continue;

        out[count].id = (uint32_t)i;
        out[count].can_input = 1;
        out[count].can_output = 0;
        snprintf(out[count].name, CMIDI_MAX_DEVICE_NAME, "%s", caps.szPname);
        ++count;
    }

    UINT out_n = midiOutGetNumDevs();
    for (UINT i = 0; i < out_n && count < max_count; ++i)
    {
        MIDIOUTCAPSA caps;
        if (midiOutGetDevCapsA(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
            continue;

        out[count].id = (uint32_t)i;
        out[count].can_input = 0;
        out[count].can_output = 1;
        snprintf(out[count].name, CMIDI_MAX_DEVICE_NAME, "%s", caps.szPname);
        ++count;
    }

    return count;
}

cmidi_port_t* cmidi_platform_open_input(uint32_t device_id, cmidi_input_callback_t callback, void* userdata)
{
    cmidi_port_t* port = (cmidi_port_t*)calloc(1, sizeof(*port));
    if (!port)
    {
        cmidi_set_error("Out of memory");
        return NULL;
    }

    *port = (cmidi_port_t) {
        .is_input = 1,
        .callback = callback,
        .userdata = userdata    
    };

    MIDIINCAPSA caps;
    if (midiInGetDevCapsA(device_id, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        snprintf(port->name, CMIDI_MAX_DEVICE_NAME, "%s", caps.szPname);

    MMRESULT r = midiInOpen(&port->handle.in, (UINT)device_id, (DWORD_PTR)winmm_input_proc, (DWORD_PTR)port, CALLBACK_FUNCTION);
    if (r != MMSYSERR_NOERROR)
    {
        cmidi_set_error("midiInOpen failed (device %u, error %u)", (unsigned)device_id, (unsigned)r);
        free(port);
        return NULL;
    }

    port->sysex_hdr.lpData = (LPSTR)port->sysex_hdr_buf;
    port->sysex_hdr.dwBufferLength = sizeof(port->sysex_hdr_buf);
    if (midiInPrepareHeader(port->handle.in, &port->sysex_hdr, sizeof(MIDIHDR)) == MMSYSERR_NOERROR)
        midiInAddBuffer(port->handle.in, &port->sysex_hdr, sizeof(MIDIHDR));

    midiInStart(port->handle.in);
    return port;
}

cmidi_port_t* cmidi_platform_open_output(uint32_t device_id)
{
    cmidi_port_t* port = (cmidi_port_t*)calloc(1, sizeof(*port));
    if (!port)
    {
        cmidi_set_error("Out of memory");
        return NULL;
    }

    port->is_input = 0;
    MIDIOUTCAPSA caps;
    if (midiOutGetDevCapsA(device_id, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        snprintf(port->name, CMIDI_MAX_DEVICE_NAME, "%s", caps.szPname);

    MMRESULT r = midiOutOpen(&port->handle.out, (UINT)device_id, 0, 0, CALLBACK_NULL);
    if (r != MMSYSERR_NOERROR)
    {
        cmidi_set_error("midiOutOpen failed (device %u, error %u)", (unsigned)device_id, (unsigned)r);
        free(port);
        return NULL;
    }

    return port;
}

int cmidi_platform_send(cmidi_port_t* port, const uint8_t* bytes, uint8_t length)
{
    if (!port || port->is_input || length == 0)
        return 0;

    DWORD msg = 0;
    for (uint8_t i = 0; i < length && i < 3; ++i)
    {
        msg |= ((DWORD)bytes[i]) << (i * 8);
    }

    return midiOutShortMsg(port->handle.out, msg) == MMSYSERR_NOERROR ? 1 : 0;
}

int cmidi_platform_send_sysex(cmidi_port_t* port, const uint8_t* bytes, uint32_t length)
{
    if (!port || port->is_input || !bytes || length == 0)
        return 0;

    MIDIHDR hdr = {
        .lpData = (LPSTR)bytes,
        .dwBufferLength = length,
        .dwBytesRecorded = length
    };

    if (midiOutPrepareHeader(port->handle.out, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR)
        return 0;

    if (midiOutLongMsg(port->handle.out, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR)
    {
        midiOutUnprepareHeader(port->handle.out, &hdr, sizeof(hdr));
        return 0;
    }

    // midiOutLongMsg is asynchronous.
    // Must stay valid until the driver is done with it.
    // Wait for MHDR_DONE before returning.
    int waited_ms = 0;
    while (!(hdr.dwFlags & MHDR_DONE) && waited_ms < 5000)
    {
        Sleep(1);
        waited_ms += 1;
    }

    midiOutUnprepareHeader(port->handle.out, &hdr, sizeof(hdr));
    return (hdr.dwFlags & MHDR_DONE) ? 1 : 0;
}

void cmidi_platform_close(cmidi_port_t* port)
{
    if (!port)
        return;

    if (port->is_input)
    {
        midiInStop(port->handle.in);
        midiInReset(port->handle.in);
        midiInUnprepareHeader(port->handle.in, &port->sysex_hdr, sizeof(MIDIHDR));
        midiInClose(port->handle.in);
        free(port->sysex_buf);
    }
    else
    {
        midiOutReset(port->handle.out);
        midiOutClose(port->handle.out);
    }

    free(port);
}

const char* cmidi_platform_port_name(const cmidi_port_t* port)
{
    return port ? port->name : NULL;
}

int cmidi_platform_port_is_input(const cmidi_port_t* port)
{
    return port ? port->is_input : 0;
}

static LARGE_INTEGER s_freq;
static LARGE_INTEGER s_origin;
static int s_initialised = 0;

static void on_exit(void)
{
    timeEndPeriod(1);
}

static void ensure_init(void)
{
    if (s_initialised)
        return;

    timeBeginPeriod(1);
    atexit(on_exit);
    QueryPerformanceFrequency(&s_freq);
    QueryPerformanceCounter(&s_origin);
    s_initialised = 1;
}

double cmidi_time_now_ms(void)
{
    ensure_init();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - s_origin.QuadPart) * 1000.0 / (double)s_freq.QuadPart;
}

void cmidi_time_sleep_ms(uint32_t ms)
{
    ensure_init();
    Sleep(ms);
}

typedef HANDLE cmidi_thread_t;
typedef CRITICAL_SECTION cmidi_mutex_t;
typedef CONDITION_VARIABLE cmidi_cond_t;

static void mutex_init(cmidi_mutex_t* m)
{
    InitializeCriticalSection(m);
}

static void mutex_lock(cmidi_mutex_t* m)
{
    EnterCriticalSection(m);
}

static void mutex_unlock(cmidi_mutex_t* m)
{
    LeaveCriticalSection(m);
}

static void mutex_destroy(cmidi_mutex_t* m)
{
    DeleteCriticalSection(m);
}

static void cond_init(cmidi_cond_t* c)
{
    InitializeConditionVariable(c);
}

static void cond_signal(cmidi_cond_t* c, cmidi_mutex_t* m)
{
    (void)m;
    WakeAllConditionVariable(c);
}

static void cond_wait(cmidi_cond_t* c, cmidi_mutex_t* m)
{
    SleepConditionVariableCS(c, m, INFINITE);
}

static void cond_destroy(cmidi_cond_t* c)
{
    (void)c;
}

static DWORD WINAPI thread_proc(LPVOID arg);

static int thread_start(cmidi_thread_t* t, void* arg)
{
    *t = CreateThread(NULL, 0, thread_proc, arg, 0, NULL);
    if (*t)
        SetThreadPriority(*t, THREAD_PRIORITY_TIME_CRITICAL);

    return *t != NULL;
}

static void thread_join(cmidi_thread_t t)
{
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
}

#elif defined(__APPLE__)

#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>

struct cmidi_port
{
	cmidi_input_callback_t callback;
	void* userdata;
	uint8_t* sysex_buf;
	MIDIClientRef client;
	MIDIPortRef port;
	MIDIEndpointRef endpoint;
	uint32_t sysex_len;
	uint32_t sysex_cap;
	int is_input;
	char name[CMIDI_MAX_DEVICE_NAME];
};

static double mach_to_ms(MIDITimeStamp ts)
{
	static mach_timebase_info_data_t info;
	static int initialised = 0;
	if (!initialised)
	{
		mach_timebase_info(&info);
		initialised = 1;
	}

	return (double)ts * (double)info.numer / (double)info.denom / 1e6;
}

static void get_endpoint_name(MIDIEndpointRef ep, char* buf, int size)
{
	CFStringRef str = NULL;
	MIDIObjectGetStringProperty(ep, kMIDIPropertyDisplayName, &str);
	if (str)
	{
		CFStringGetCString(str, buf, size, kCFStringEncodingUTF8);
		CFRelease(str);
	}
	else
		snprintf(buf, size, "Unknown");
}

static void mac_sysex_append(cmidi_port_t* port, uint8_t b)
{
	if (port->sysex_len >= port->sysex_cap)
	{
		uint32_t newcap = port->sysex_cap ? port->sysex_cap * 2 : 256;
		uint8_t* buf = (uint8_t*)realloc(port->sysex_buf, newcap);
		if (!buf)
		{
			port->sysex_len = 0;
			return;
		}

		port->sysex_buf = buf;
		port->sysex_cap = newcap;
	}

	port->sysex_buf[port->sysex_len++] = b;
}

static void core_midi_read_proc(const MIDIPacketList* pktList, void* readProcRefCon, void* srcConnRefCon)
{
	(void)srcConnRefCon;
	cmidi_port_t* port = (cmidi_port_t*)readProcRefCon;
	if (!port)
		return;

	const MIDIPacket* pkt = &pktList->packet[0];
	for (UInt32 p = 0; p < pktList->numPackets; ++p)
	{
		double ts_ms = mach_to_ms(pkt->timeStamp);
		UInt32 j = 0;

		while (j < pkt->length)
		{
			uint8_t b0 = pkt->data[j];
			if (port->sysex_len > 0 || b0 == CMIDI_CODE_SYSEX)
			{
				while (j < pkt->length)
				{
					uint8_t b = pkt->data[j++];
					mac_sysex_append(port, b);
					if (b == CMIDI_CODE_SYSEX_END)
					{
						cmidi_dispatch_sysex_input(port, port->callback, port->userdata, port->sysex_buf, port->sysex_len, ts_ms);
						port->sysex_len = 0;
						break;
					}
				}
				
				continue;
			}

			uint8_t status = b0 & CMIDI_CODE_SYSEX;
			uint8_t bytes[3];
			uint8_t length = 0;

			if (b0 >= CMIDI_CODE_CLOCK)
			{
				bytes[0] = b0;
				length = 1;
				++j;
			}
			else if (status == CMIDI_CODE_PROGRAM_CHANGE || status == CMIDI_CODE_CHANNEL_PRESSURE)
			{
				if (j + 1 >= pkt->length)
					break;
					
				bytes[0] = pkt->data[j];
				bytes[1] = pkt->data[j + 1];
				length = 2;
				j += 2;
			}
			else if (b0 < CMIDI_CODE_SYSEX)
			{
				if (j + 2 >= pkt->length)
					break;

				bytes[0] = pkt->data[j];
				bytes[1] = pkt->data[j + 1];
				bytes[2] = pkt->data[j + 2];
				length = 3;
				j += 3;
			}
			else
			{
				++j;
				continue;
			}

			if (length > 0)
				cmidi_dispatch_input(port, port->callback, port->userdata, bytes, length, ts_ms);
		}

		pkt = MIDIPacketNext(pkt);
	}
}

int cmidi_platform_enumerate(cmidi_device_t* out, int max_count)
{
	int count = 0;
	ItemCount src_n = MIDIGetNumberOfSources();
	for (ItemCount i = 0; i < src_n && count < max_count; ++i)
	{
		MIDIEndpointRef ep = MIDIGetSource(i);
		out[count].id = (uint32_t)i;
		out[count].can_input = 1;
		out[count].can_output = 0;
		get_endpoint_name(ep, out[count].name, CMIDI_MAX_DEVICE_NAME);
		++count;
	}

	ItemCount dst_n = MIDIGetNumberOfDestinations();
	for (ItemCount i = 0; i < dst_n && count < max_count; ++i)
	{
		MIDIEndpointRef ep = MIDIGetDestination(i);
		out[count].id = (uint32_t)i;
		out[count].can_input = 0;
		out[count].can_output = 1;
		get_endpoint_name(ep, out[count].name, CMIDI_MAX_DEVICE_NAME);
		++count;
	}

	return count;
}

cmidi_port_t* cmidi_platform_open_input(uint32_t device_id, cmidi_input_callback_t callback, void* userdata)
{
	if (device_id >= (uint32_t)MIDIGetNumberOfSources())
	{
		cmidi_set_error("Input device %u not found", (unsigned)device_id);
		return NULL;
	}

	cmidi_port_t* port = (cmidi_port_t*)calloc(1, sizeof(*port));
	if (!port)
	{
		cmidi_set_error("Out of memory");
		return NULL;
	}

	port->is_input = 1;
	port->callback = callback;
	port->userdata = userdata;
	port->endpoint = MIDIGetSource(device_id);
	get_endpoint_name(port->endpoint, port->name, CMIDI_MAX_DEVICE_NAME);

	OSStatus err = MIDIClientCreate(CFSTR("cmidi"), NULL, NULL, &port->client);
	if (err != noErr)
	{
		cmidi_set_error("MIDIClientCreate failed (%d)", (int)err);
		free(port);
		return NULL;
	}

	err = MIDIInputPortCreate(port->client, CFSTR("Input"), core_midi_read_proc, port, &port->port);
	if (err != noErr)
	{
		cmidi_set_error("MIDIInputPortCreate failed (%d)", (int)err);
		MIDIClientDispose(port->client);
		free(port);
		return NULL;
	}

	err = MIDIPortConnectSource(port->port, port->endpoint, NULL);
	if (err != noErr)
	{
		cmidi_set_error("MIDIPortConnectSource failed (%d)", (int)err);
		MIDIPortDispose(port->port);
		MIDIClientDispose(port->client);
		free(port);
		return NULL;
	}

	return port;
}

cmidi_port_t* cmidi_platform_open_output(uint32_t device_id)
{
	if (device_id >= (uint32_t)MIDIGetNumberOfDestinations())
	{
		cmidi_set_error("Output device %u not found", (unsigned)device_id);
		return NULL;
	}

	cmidi_port_t* port = (cmidi_port_t*)calloc(1, sizeof(*port));
	if (!port)
	{
		cmidi_set_error("Out of memory");
		return NULL;
	}

	port->is_input = 0;
	port->endpoint = MIDIGetDestination(device_id);
	get_endpoint_name(port->endpoint, port->name, CMIDI_MAX_DEVICE_NAME);

	OSStatus err = MIDIClientCreate(CFSTR("cmidi"), NULL, NULL, &port->client);
	if (err != noErr)
	{
		cmidi_set_error("MIDIClientCreate failed (%d)", (int)err);
		free(port);
		return NULL;
	}

	err = MIDIOutputPortCreate(port->client, CFSTR("Output"), &port->port);
	if (err != noErr)
	{
		cmidi_set_error("MIDIOutputPortCreate failed (%d)", (int)err);
		MIDIClientDispose(port->client);
		free(port);
		return NULL;
	}

	return port;
}

int cmidi_platform_send(cmidi_port_t* port, const uint8_t* bytes, uint8_t length)
{
	if (!port || port->is_input || length == 0 || length > 3)
		return 0;

	Byte buf[sizeof(MIDIPacketList) + 32];
	MIDIPacketList* pktList = (MIDIPacketList*)buf;
	MIDIPacket* pkt = MIDIPacketListInit(pktList);
	pkt = MIDIPacketListAdd(pktList, sizeof(buf), pkt, 0, length, bytes);
	if (!pkt)
		return 0;

	return MIDISend(port->port, port->endpoint, pktList) == noErr ? 1 : 0;
}

int cmidi_platform_send_sysex(cmidi_port_t* port, const uint8_t* bytes, uint32_t length)
{
	if (!port || port->is_input || !bytes || length == 0)
		return 0;

	size_t bufsize = sizeof(MIDIPacketList) + (size_t)length + 32;
	Byte* buf = (Byte*)malloc(bufsize);
	if (!buf)
		return 0;

	MIDIPacketList* pktList = (MIDIPacketList*)buf;
	MIDIPacket* pkt = MIDIPacketListInit(pktList);
	pkt = MIDIPacketListAdd(pktList, bufsize, pkt, 0, length, bytes);
	int ok = 0;
	if (pkt)
		ok = (MIDISend(port->port, port->endpoint, pktList) == noErr) ? 1 : 0;

	free(buf);
	return ok;
}

void cmidi_platform_close(cmidi_port_t* port)
{
	if (!port)
		return;

	if (port->is_input)
		MIDIPortDisconnectSource(port->port, port->endpoint);

	MIDIPortDispose(port->port);
	MIDIClientDispose(port->client);
	free(port->sysex_buf);
	free(port);
}

const char* cmidi_platform_port_name(const cmidi_port_t* port)
{
	return port ? port->name : NULL;
}

int cmidi_platform_port_is_input(const cmidi_port_t* port)
{
	return port ? port->is_input : 0;
}

#endif

#define CMIDI_INITIAL_CAPACITY 256

struct cmidi_file
{
    cmidi_sched_event_t* events;
    int count;
    int capacity;
    uint16_t ticks_per_beat;
    uint32_t initial_tempo;
    uint16_t format; // SMF format: 0, 1, or 2. See cmidi_file_format()/cmidi_file_set_format().

    uint8_t* sysex_pool;
    size_t sysex_pool_size;
    size_t sysex_pool_cap;
};

static int file_push(cmidi_file_t* file, cmidi_sched_event_t event)
{
    if (file->count >= file->capacity)
    {
        int newcap = file->capacity * 2;
        cmidi_sched_event_t* buf = (cmidi_sched_event_t*)realloc(file->events, (size_t)newcap * sizeof(*buf));
        if (!buf)
            return 0;

        file->events = buf;
        file->capacity = newcap;
    }

    file->events[file->count++] = event;
    return 1;
}

static uint32_t file_sysex_push(cmidi_file_t* file, uint8_t leading, const uint8_t* payload, uint32_t payload_len)
{
    size_t needed = (size_t)payload_len + 1;
    if (file->sysex_pool_size + needed > file->sysex_pool_cap)
    {
        size_t newcap = file->sysex_pool_cap ? file->sysex_pool_cap * 2 : 256;
        while (newcap < file->sysex_pool_size + needed)
        {
            newcap *= 2;
        }

        uint8_t* buf = (uint8_t*)realloc(file->sysex_pool, newcap);
        if (!buf)
            return (uint32_t)-1;

        file->sysex_pool = buf;
        file->sysex_pool_cap = newcap;
    }

    uint32_t offset = (uint32_t)file->sysex_pool_size;
    file->sysex_pool[file->sysex_pool_size++] = leading;
    if (payload_len > 0)
        memcpy(file->sysex_pool + file->sysex_pool_size, payload, payload_len);

    file->sysex_pool_size += payload_len;
    return offset;
}

static int event_cmp(const void* a, const void* buffer)
{
    const cmidi_sched_event_t* ea = (const cmidi_sched_event_t*)a;
    const cmidi_sched_event_t* eb = (const cmidi_sched_event_t*)buffer;
    if (ea->tick < eb->tick)
        return -1;

    if (ea->tick > eb->tick)
        return 1;

    return 0;
}

static uint16_t p_be16(const uint8_t* p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

static uint32_t p_be32(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t p_vlq(const uint8_t* data, size_t* pos, size_t limit)
{
    uint32_t val = 0;
    uint8_t buffer;
    do {
        buffer = data[(*pos)++];
        val = (val << 7) | (buffer & 0x7F);
    } while (buffer & CMIDI_CODE_NOTE_OFF);
    return val;
}

static void parse_track(cmidi_file_t* file, const uint8_t* data, size_t start, size_t track_len)
{
    size_t pos = start;
    size_t end = start + track_len;
    uint32_t tick = 0;
    uint8_t running_status = 0;

    while (pos < end)
    {
        uint32_t delta = p_vlq(data, &pos, end);
        tick += delta;
        if (pos >= end)
            break;

        uint8_t byte = data[pos++];
        if (byte & CMIDI_CODE_NOTE_OFF)
            running_status = byte;
        else
        {
            --pos;
            byte = running_status;
        }

        if (byte == 0xFF)
        {
            if (pos >= end)
                break;

            uint8_t mtype = data[pos++];
            uint32_t mlen = p_vlq(data, &pos, end);
            cmidi_sched_event_t event = {
                .tick = tick
            };

            if (mtype == 0x51 && mlen == 3 && pos + 3 <= end)
            {
                uint32_t tempo = ((uint32_t)data[pos] << 16) | ((uint32_t)data[pos+1] << 8) | (uint32_t)data[pos+2];
                if (file->initial_tempo == 500000 && tick == 0)
                    file->initial_tempo = tempo;

                event.type = CMIDI_SCHED_TEMPO_CHANGE;
                event.tempo_us = tempo;
                file_push(file, event);
            }
            else if (mtype == 0x2F)
            {
                event.type = CMIDI_SCHED_END_OF_TRACK;
                file_push(file, event);
            }

            pos += mlen;
            running_status = 0;
            continue;
        }

        if (byte == CMIDI_CODE_SYSEX || byte == CMIDI_CODE_SYSEX_END)
        {
            uint32_t len = p_vlq(data, &pos, end);
            if (pos + len <= end)
            {
                uint32_t offset = file_sysex_push(file, byte, data + pos, len);
                if (offset != (uint32_t)-1)
                {
                    cmidi_sched_event_t event = {
                        .tick = tick,
                        .type = CMIDI_SCHED_SYSEX,
                        .sysex_offset = offset,
                        .sysex_length = len + 1
                    };

                    file_push(file, event);
                }
            }

            pos += len;
            running_status = 0;
            continue;
        }

        uint8_t status = byte & CMIDI_CODE_SYSEX;
        uint8_t channel = byte & 0x0F;
        cmidi_sched_event_t event = {
            .tick = tick,
            .channel = channel
        };

        if ((status == CMIDI_CODE_NOTE_ON || status == CMIDI_CODE_NOTE_OFF) && pos + 2 <= end)
        {
            uint8_t note = data[pos++];
            uint8_t vel = data[pos++];
            event.type = (status == CMIDI_CODE_NOTE_ON && vel > 0) ? CMIDI_SCHED_NOTE_ON : CMIDI_SCHED_NOTE_OFF;
            event.data0 = note;
            event.data1 = vel;
            file_push(file, event);
        }
        else if (status == CMIDI_CODE_CONTROL_CHANGE && pos + 2 <= end)
        {
            event.type = CMIDI_SCHED_CONTROL_CHANGE;
            event.data0 = data[pos++];
            event.data1 = data[pos++];
            file_push(file, event);
        }
        else if (status == CMIDI_CODE_PROGRAM_CHANGE && pos + 1 <= end)
        {
            event.type = CMIDI_SCHED_PROGRAM_CHANGE;
            event.data0 = data[pos++];
            file_push(file, event);
        }
        else if (status == CMIDI_CODE_CHANNEL_PRESSURE && pos + 1 <= end)
        {
            event.type = CMIDI_SCHED_CHANNEL_PRESSURE;
            event.data0 = data[pos++];
            file_push(file, event);
        }
        else if (status == CMIDI_CODE_PITCH_BEND && pos + 2 <= end)
        {
            event.type = CMIDI_SCHED_PITCH_BEND;
            event.data0 = data[pos++];
            event.data1 = data[pos++];
            file_push(file, event);
        }
        else if (status == CMIDI_CODE_POLY_PRESSURE && pos + 2 <= end)
        {
            event.type = CMIDI_SCHED_POLY_PRESSURE;
            event.data0 = data[pos++];
            event.data1 = data[pos++];
            file_push(file, event);
        }
    }
}

static cmidi_file_t* parse_smf(const uint8_t* buf, size_t file_size)
{
    if (file_size < 14 || memcmp(buf, "MThd", 4) != 0)
        return NULL;

    uint32_t header_len = p_be32(buf + 4);
    uint16_t format = p_be16(buf + 8);
    uint16_t track_count = p_be16(buf + 10);
    uint16_t ticks_per_beat = p_be16(buf + 12);

    cmidi_file_t* file = (cmidi_file_t*)calloc(1, sizeof(*file));
    if (!file)
        return NULL;

    file->format = format;
    file->ticks_per_beat = ticks_per_beat;
    file->initial_tempo = 500000;
    file->capacity = CMIDI_INITIAL_CAPACITY;
    file->events = (cmidi_sched_event_t*)malloc((size_t)file->capacity * sizeof(*file->events));
    if (!file->events)
    {
        free(file);
        return NULL;
    }

    size_t pos = 8 + header_len;
    for (uint16_t t = 0; t < track_count; ++t)
    {
        if (pos + 8 > file_size)
            break;

        if (memcmp(buf + pos, "MTrk", 4) != 0)
            break;

        uint32_t track_len = p_be32(buf + pos + 4);
        pos += 8;
        parse_track(file, buf, pos, track_len);
        pos += track_len;
    }

    qsort(file->events, (size_t)file->count, sizeof(*file->events), event_cmp);
    return file;
}

typedef struct
{
    uint8_t* data;
    size_t size;
    size_t cap;
} cmidi_bytes_buf;

static int buf_push(cmidi_bytes_buf* buffer, uint8_t v)
{
    if (buffer->size >= buffer->cap)
    {
        size_t newcap = buffer->cap * 2;
        uint8_t* p = (uint8_t*)realloc(buffer->data, newcap);
        if (!p)
            return 0;

        buffer->data = p; buffer->cap = newcap;
    }

    buffer->data[buffer->size++] = v;
    return 1;
}

static void buf_push_be16(cmidi_bytes_buf* buffer, uint16_t v)
{
    buf_push(buffer, (uint8_t)(v >> 8));
    buf_push(buffer, (uint8_t)(v & 0xFF));
}

static void buf_push_be32(cmidi_bytes_buf* buffer, uint32_t v)
{
    buf_push(buffer, (uint8_t)(v >> 24));
    buf_push(buffer, (uint8_t)(v >> 16));
    buf_push(buffer, (uint8_t)(v >> 8));
    buf_push(buffer, (uint8_t)(v & 0xFF));
}

static void buf_push_vlq(cmidi_bytes_buf* buffer, uint32_t v)
{
    uint8_t temp[4];
    int n = 0;
    if (v < CMIDI_CODE_NOTE_OFF)
    {
        temp[n++] = (uint8_t)v;
    }
    else if (v < 0x4000)
    {
        temp[n++] = (uint8_t)((v >> 7) | CMIDI_CODE_NOTE_OFF);
        temp[n++] = (uint8_t)(v & 0x7F);
    }
    else if (v < 0x200000)
    {
        temp[n++] = (uint8_t)((v>>14)|CMIDI_CODE_NOTE_OFF);
        temp[n++]=(uint8_t)((v>>7)|CMIDI_CODE_NOTE_OFF);
        temp[n++]=(uint8_t)(v&0x7F);
    }
    else
    {
        temp[n++]=(uint8_t)((v>>21)|CMIDI_CODE_NOTE_OFF);
        temp[n++]=(uint8_t)((v>>14)|CMIDI_CODE_NOTE_OFF);
        temp[n++]=(uint8_t)((v>>7)|CMIDI_CODE_NOTE_OFF);
        temp[n++]=(uint8_t)(v&0x7F);
    }

    for (int i = 0; i < n; ++i)
    {
        buf_push(buffer, temp[i]);
    }
}

#pragma region Public API
cmidi_file_t* cmidi_file_load(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    long sz = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t* buf = (uint8_t*)malloc((size_t)sz);
    if (!buf)
    {
        fclose(file);
        return NULL;
    }

    fread(buf, 1, (size_t)sz, file);
    fclose(file);

    cmidi_file_t* result = parse_smf(buf, (size_t)sz);
    free(buf);
    return result;
}

cmidi_file_t* cmidi_file_load_memory(const uint8_t* data, size_t size)
{
    return parse_smf(data, size);
}

const cmidi_sched_event_t* cmidi_file_events(const cmidi_file_t* file)
{
    return file ? file->events : NULL;
}

int cmidi_file_event_count(const cmidi_file_t* file)
{
    return file ? file->count : 0;
}

uint16_t cmidi_file_ticks_per_beat(const cmidi_file_t* file)
{
    return file ? file->ticks_per_beat : 480;
}

uint32_t cmidi_file_initial_tempo(const cmidi_file_t* file)
{
    return file ? file->initial_tempo : 500000;
}

const uint8_t* cmidi_file_sysex_pool(const cmidi_file_t* file)
{
    return file ? file->sysex_pool : NULL;
}

uint16_t cmidi_file_format(const cmidi_file_t* file)
{
    return file ? file->format : 0;
}

void cmidi_file_set_format(cmidi_file_t* file, uint16_t format)
{
    if (!file)
        return;

    file->format = format;
}

cmidi_file_t* cmidi_file_create(uint16_t ticks_per_beat)
{
    cmidi_file_t* file = (cmidi_file_t*)calloc(1, sizeof(*file));
    if (!file)
        return NULL;
        
    file->ticks_per_beat = ticks_per_beat;
    file->initial_tempo = 500000;
    file->capacity = CMIDI_INITIAL_CAPACITY;
    file->events = (cmidi_sched_event_t*)malloc((size_t)file->capacity * sizeof(*file->events));
    if (!file->events)
    {
        free(file);
        return NULL;
    }

    return file;
}

void cmidi_file_set_tempo(cmidi_file_t* file, uint32_t tempo_us)
{
    if (!file)
        return;

    file->initial_tempo = tempo_us;
    cmidi_sched_event_t event = {
        .tick = 0,
        .type = CMIDI_SCHED_TEMPO_CHANGE,
        .tempo_us = tempo_us
    };

    file_push(file, event);
}

void cmidi_file_add_note(cmidi_file_t* file, uint8_t channel, uint32_t start_tick, uint8_t note, uint8_t velocity, uint32_t duration_ticks)
{
    if (!file)
        return;

    cmidi_sched_event_t on = {
        .tick = start_tick,
        .type = CMIDI_SCHED_NOTE_ON,
        .channel = channel,
        .data0 = note,
        .data1 = velocity
    };

    cmidi_sched_event_t off = {
        .tick = start_tick + duration_ticks,
        .type = CMIDI_SCHED_NOTE_OFF,
        .channel = channel,
        .data0 = note
    };

    file_push(file, on);
    file_push(file, off);
}

void cmidi_file_add_event(cmidi_file_t* file, cmidi_sched_event_t event)
{
    if (!file)
        return;

    file_push(file, event);
}

void cmidi_file_add_sysex(cmidi_file_t* file, uint32_t tick, const uint8_t* data, uint32_t length)
{
    if (!file || !data || length < 2 || data[0] != CMIDI_CODE_SYSEX)
        return;

    uint32_t offset = file_sysex_push(file, data[0], data + 1, length - 1);
    if (offset == (uint32_t)-1)
        return;

    cmidi_sched_event_t event = {
        .tick = tick,
        .type = CMIDI_SCHED_SYSEX,
        .sysex_offset = offset,
        .sysex_length = length
    };

    file_push(file, event);
}

static int event_in_track(const cmidi_sched_event_t* event, int channel_filter)
{
    switch (event->type)
    {
        case CMIDI_SCHED_TEMPO_CHANGE:
        case CMIDI_SCHED_SYSEX:
            return channel_filter == -2 || channel_filter == -1;

        case CMIDI_SCHED_NOTE_ON:
        case CMIDI_SCHED_NOTE_OFF:
        case CMIDI_SCHED_CONTROL_CHANGE:
        case CMIDI_SCHED_POLY_PRESSURE:
        case CMIDI_SCHED_PROGRAM_CHANGE:
        case CMIDI_SCHED_CHANNEL_PRESSURE:
        case CMIDI_SCHED_PITCH_BEND:
            return channel_filter == -2 || channel_filter == (int)event->channel;

        default:
            return 0;
    }
}

static void write_event_bytes(cmidi_bytes_buf* track, const cmidi_sched_event_t* event, const uint8_t* sysex_pool)
{
    uint8_t status = (uint8_t)event->type;
    switch (event->type)
    {
        case CMIDI_SCHED_NOTE_ON:
        case CMIDI_SCHED_NOTE_OFF:
        case CMIDI_SCHED_CONTROL_CHANGE:
        case CMIDI_SCHED_POLY_PRESSURE:
            buf_push(track, (uint8_t)(status | (event->channel & 0x0F)));
            buf_push(track, event->data0);
            buf_push(track, event->data1);
            break;

        case CMIDI_SCHED_PROGRAM_CHANGE:
        case CMIDI_SCHED_CHANNEL_PRESSURE:
            buf_push(track, (uint8_t)(status | (event->channel & 0x0F)));
            buf_push(track, event->data0);
            break;

        case CMIDI_SCHED_PITCH_BEND:
            buf_push(track, (uint8_t)(status | (event->channel & 0x0F)));
            buf_push(track, event->data0);
            buf_push(track, event->data1);
            break;

        case CMIDI_SCHED_TEMPO_CHANGE:
            buf_push(track, 0xFF); buf_push(track, 0x51); buf_push_vlq(track, 3);
            buf_push(track, (uint8_t)(event->tempo_us >> 16));
            buf_push(track, (uint8_t)(event->tempo_us >> 8));
            buf_push(track, (uint8_t)(event->tempo_us & 0xFF));
            break;

        case CMIDI_SCHED_SYSEX:
        {
            if (sysex_pool && event->sysex_length >= 1)
            {
                const uint8_t* sx = sysex_pool + event->sysex_offset;
                uint32_t payload_len = event->sysex_length - 1;
                buf_push(track, sx[0]);
                buf_push_vlq(track, payload_len);
                for (uint32_t k = 0; k < payload_len; ++k)
                {
                    buf_push(track, sx[1 + k]);
                }
            }

            break;
        }

        default:
            break;
    }
}

static void build_track(cmidi_bytes_buf* track, const cmidi_file_t* file, int channel_filter)
{
    track->size = 0;
    uint32_t prev_tick = 0;
    int is_conductor = (channel_filter == -2 || channel_filter == -1);

    if (is_conductor)
    {
        buf_push_vlq(track, 0);
        buf_push(track, 0xFF);
        buf_push(track, 0x51);
        buf_push_vlq(track, 3);
        buf_push(track, (uint8_t)(file->initial_tempo >> 16));
        buf_push(track, (uint8_t)(file->initial_tempo >> 8));
        buf_push(track, (uint8_t)(file->initial_tempo & 0xFF));
    }

    for (int i = 0; i < file->count; ++i)
    {
        const cmidi_sched_event_t* event = &file->events[i];
        if (!event_in_track(event, channel_filter))
            continue;

        if (is_conductor && event->type == CMIDI_SCHED_TEMPO_CHANGE && event->tick == 0)
            continue;

        uint32_t delta = event->tick - prev_tick;
        prev_tick = event->tick;
        buf_push_vlq(track, delta);
        write_event_bytes(track, event, file->sysex_pool);
    }

    buf_push_vlq(track, 0);
    buf_push(track, 0xFF);
    buf_push(track, 0x2F);
    buf_push_vlq(track, 0);
}

static void write_be16(FILE* f, uint16_t v)
{
    uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)(v & 0xFF) };
    fwrite(b, 1, 2, f);
}

static void write_be32(FILE* f, uint32_t v)
{
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)(v & 0xFF) };
    fwrite(b, 1, 4, f);
}

static void write_mtrk(FILE* f, const cmidi_bytes_buf* track)
{
    fwrite("MTrk", 1, 4, f);
    write_be32(f, (uint32_t)track->size);
    fwrite(track->data, 1, track->size, f);
}

int cmidi_file_save(const cmidi_file_t* file, const char* path)
{
    if (!file)
        return 0;

    qsort(file->events, (size_t)file->count, sizeof(*file->events), event_cmp);
    uint16_t out_format = (file->format == 0) ? 0 : 1;
    int used_channels[16];
    int used_channel_count = 0;
    if (out_format == 1)
    {
        int seen[16] = { 0 };
        for (int i = 0; i < file->count; ++i)
        {
            if (event_in_track(&file->events[i], -2) && !event_in_track(&file->events[i], -1))
                seen[file->events[i].channel & 0x0F] = 1;
        }

        for (int ch = 0; ch < 16; ++ch)
        {
            if (seen[ch])
                used_channels[used_channel_count++] = ch;
        }
    }

    uint16_t track_count = (out_format == 0) ? 1 : (uint16_t)(1 + used_channel_count);
    FILE* file_dest = fopen(path, "wb");
    if (!file_dest)
        return 0;

    fwrite("MThd", 1, 4, file_dest);
    write_be32(file_dest, 6);
    write_be16(file_dest, out_format);
    write_be16(file_dest, track_count);
    write_be16(file_dest, file->ticks_per_beat);

    cmidi_bytes_buf track = { NULL, 0, 4096 };
    track.data = (uint8_t*)malloc(track.cap);
    if (!track.data)
    {
        fclose(file_dest);
        return 0;
    }

    build_track(&track, file, out_format == 0 ? -2 : -1);
    write_mtrk(file_dest, &track);
    for (int k = 0; k < used_channel_count; ++k)
    {
        build_track(&track, file, used_channels[k]);
        write_mtrk(file_dest, &track);
    }

    free(track.data);
    fclose(file_dest);
    return 1;
}

void cmidi_file_free(cmidi_file_t* file)
{
    if (!file)
        return;

    free(file->events);
    free(file->sysex_pool);
    free(file);
}

#define CMIDI_MAX_TEMPO_POINTS 256
#define CMIDI_CHUNK_MS 2

typedef struct
{
	uint32_t tick;
	uint32_t tempo_us;
	double time_ms;
} cmidi_tempo_point;

struct cmidi_scheduler
{
	uint32_t current_tick;
	uint32_t loop_end_tick;

	int event_count;
	int tempo_count;

	uint16_t ticks_per_beat;

	uint8_t loop_enabled;
	volatile uint8_t playing;
	volatile uint8_t stop_requested;
	uint8_t thread_running;

	cmidi_port_t* port;
	cmidi_sched_event_t* events;

	uint8_t* sysex_pool;
	size_t sysex_pool_size;

	cmidi_scheduler_callback_t callback;
	void* callback_userdata;
	cmidi_scheduler_end_callback_t end_callback;
	void* end_callback_userdata;

	cmidi_mutex_t mutex;
	cmidi_cond_t done_cond;
	cmidi_thread_t thread;

	cmidi_tempo_point tempo_map[CMIDI_MAX_TEMPO_POINTS];
};


uint32_t cmidi_bpm_to_tempo_us(double bpm)
{
	if (bpm <= 0.0)
		return 0;

	return (uint32_t)(60000000.0 / bpm + 0.5);
}

double cmidi_tempo_us_to_bpm(uint32_t tempo_us)
{
	if (tempo_us == 0)
		return 0.0;

	return 60000000.0 / (double)tempo_us;
}

double cmidi_ticks_to_ms(uint32_t ticks, uint16_t ticks_per_beat, uint32_t tempo_us)
{
	if (ticks_per_beat == 0)
		return 0.0;

	double ms_per_tick = (double)tempo_us / (double)ticks_per_beat / 1000.0;
	return (double)ticks * ms_per_tick;
}

uint32_t cmidi_ms_to_ticks(double ms, uint16_t ticks_per_beat, uint32_t tempo_us)
{
	if (ticks_per_beat == 0 || tempo_us == 0)
		return 0;

	double ms_per_tick = (double)tempo_us / (double)ticks_per_beat / 1000.0;
	if (ms_per_tick <= 0.0)
		return 0;

	return (uint32_t)(ms / ms_per_tick + 0.5);
}

static void build_tempo_map(cmidi_scheduler_t* scheduler)
{
	scheduler->tempo_map[0].tick = 0;
	scheduler->tempo_map[0].tempo_us = 500000;
	scheduler->tempo_map[0].time_ms = 0.0;
	scheduler->tempo_count = 1;

	for (int i = 0; i < scheduler->event_count && scheduler->tempo_count < CMIDI_MAX_TEMPO_POINTS; ++i)
	{
		const cmidi_sched_event_t* event = &scheduler->events[i];
		if (event->type != CMIDI_SCHED_TEMPO_CHANGE)
			continue;

		cmidi_tempo_point* prev = &scheduler->tempo_map[scheduler->tempo_count - 1];
		double ms_per_tick = (double)prev->tempo_us / (double)scheduler->ticks_per_beat / 1000.0;
		double time_at = prev->time_ms + (event->tick - prev->tick) * ms_per_tick;

		if (scheduler->tempo_count == 1 && event->tick == 0)
			scheduler->tempo_map[0].tempo_us = event->tempo_us;
		else
		{
			cmidi_tempo_point* point = &scheduler->tempo_map[scheduler->tempo_count++];
			point->tick = event->tick;
			point->tempo_us = event->tempo_us;
			point->time_ms = time_at;
		}
	}
}

static double tick_to_ms(const cmidi_scheduler_t* scheduler, uint32_t tick)
{
	const cmidi_tempo_point* point = &scheduler->tempo_map[0];
	for (int i = 1; i < scheduler->tempo_count; ++i)
	{
		if (scheduler->tempo_map[i].tick > tick)
			break;

		point = &scheduler->tempo_map[i];
	}
	return point->time_ms + (double)(tick - point->tick) * ((double)point->tempo_us / (double)scheduler->ticks_per_beat / 1000.0);
}

static void send_all_notes_off(cmidi_port_t* port)
{
	if (!port)
		return;

	for (uint8_t ch = 0; ch < 16; ++ch)
	{
		uint8_t b[3] = { (uint8_t)(0xB0 | ch), 0x7B, 0x00 };
		cmidi_send(port, b, 3);
	}
}

static void sysex_wire_bytes(const uint8_t* pool_entry, uint32_t pool_entry_len, const uint8_t** out_bytes, uint32_t* out_len)
{
	if (pool_entry_len >= 1 && pool_entry[0] == CMIDI_CODE_SYSEX_END)
	{
		*out_bytes = pool_entry + 1;
		*out_len = pool_entry_len - 1;
	}
	else
	{
		*out_bytes = pool_entry;
		*out_len = pool_entry_len;
	}
}

static void send_sched_event(cmidi_port_t* port, const cmidi_sched_event_t* event, const uint8_t* sysex_pool)
{
	if (!port)
		return;

	if (event->type == CMIDI_SCHED_SYSEX)
	{
		if (sysex_pool && event->sysex_length >= 1)
		{
			const uint8_t* bytes;
			uint32_t len;
			sysex_wire_bytes(sysex_pool + event->sysex_offset, event->sysex_length, &bytes, &len);
			if (len > 0)
				cmidi_platform_send_sysex(port, bytes, len);
		}

		return;
	}

	uint8_t b[3];
	uint8_t len = 3;
	switch (event->type)
	{
		case CMIDI_SCHED_NOTE_ON:
		case CMIDI_SCHED_NOTE_OFF:
		case CMIDI_SCHED_CONTROL_CHANGE:
		case CMIDI_SCHED_POLY_PRESSURE:
			b[0] = (uint8_t)(event->type | (event->channel & 0x0F));
			b[1] = event->data0;
			b[2] = event->data1;
			break;

		case CMIDI_SCHED_PROGRAM_CHANGE:
		case CMIDI_SCHED_CHANNEL_PRESSURE:
			b[0] = (uint8_t)(event->type | (event->channel & 0x0F));
			b[1] = event->data0;
			len = 2;
			break;

		case CMIDI_SCHED_PITCH_BEND:
			b[0] = (uint8_t)(event->type | (event->channel & 0x0F));
			b[1] = event->data0;
			b[2] = event->data1;
			break;

		default:
			return;
	}

	cmidi_send(port, b, len);
}

static cmidi_fired_event_t decode_event(const cmidi_sched_event_t* event, double timestamp_ms, uint32_t tempo_us, const uint8_t* sysex_pool)
{
	cmidi_fired_event_t fired_event = {
		.tick = event->tick,
		.timestamp_ms = timestamp_ms,
		.type = event->type,
		.channel = event->channel,
		.tempo_us = (event->type == CMIDI_SCHED_TEMPO_CHANGE) ? event->tempo_us : tempo_us
	};

	switch (event->type)
	{
		case CMIDI_SCHED_NOTE_ON:
		case CMIDI_SCHED_NOTE_OFF:
			fired_event.note = event->data0;
			fired_event.velocity = event->data1;
			break;

		case CMIDI_SCHED_CONTROL_CHANGE:
			fired_event.cc_number = event->data0;
			fired_event.cc_value = event->data1;
			break;

		case CMIDI_SCHED_PROGRAM_CHANGE:
			fired_event.program = event->data0;
			break;

		case CMIDI_SCHED_PITCH_BEND:
			fired_event.pitch_bend = (int)(event->data0 | ((int)event->data1 << 7)) - 8192;
			break;

		case CMIDI_SCHED_SYSEX:
			if (sysex_pool && event->sysex_length >= 1)
				sysex_wire_bytes(sysex_pool + event->sysex_offset, event->sysex_length, &fired_event.sysex_data, &fired_event.sysex_length);

			break;

		default:
			break;
	}

	return fired_event;
}

static int compute_loop_bound(const cmidi_scheduler_t* scheduler)
{
	int n = 0;
	while(n < scheduler->event_count && scheduler->events[n].tick < scheduler->loop_end_tick)
	{
		++n;
	}

	return n;
}

static void run_playback(cmidi_scheduler_t* scheduler)
{
	double start_ms = cmidi_time_now_ms();
	int loop_pass = 0;
	int i = 0;
	int loop_bound = scheduler->loop_enabled ? compute_loop_bound(scheduler) : scheduler->event_count;

	while (!scheduler->stop_requested && i < loop_bound)
	{
		const cmidi_sched_event_t* event = &scheduler->events[i];
		double target_ms = tick_to_ms(scheduler, event->tick);
		if (scheduler->loop_enabled && loop_pass > 0)
			target_ms += tick_to_ms(scheduler, scheduler->loop_end_tick) * loop_pass;

		while (!scheduler->stop_requested)
		{
			double rem = target_ms - (cmidi_time_now_ms() - start_ms);
			if (rem <= 0.0)
				break;

			if (rem > 1.0)
			{
				uint32_t chunk = (rem - 1.0 > CMIDI_CHUNK_MS) ? CMIDI_CHUNK_MS : (uint32_t)(rem - 1.0);
				if (chunk < 1)
					chunk = 1;

				cmidi_time_sleep_ms(chunk);
			}
			else
			{
				while ((target_ms - (cmidi_time_now_ms() - start_ms)) > 0.0);
				break;
			}
		}

		if (scheduler->stop_requested)
			break;

		mutex_lock(&scheduler->mutex);
		scheduler->current_tick = event->tick;
		mutex_unlock(&scheduler->mutex);

		uint32_t cur_tempo = scheduler->tempo_map[0].tempo_us;
		for (int t = 1; t < scheduler->tempo_count; ++t)
		{
			if (scheduler->tempo_map[t].tick > event->tick)
				break;

			cur_tempo = scheduler->tempo_map[t].tempo_us;
		}

		if (scheduler->callback)
		{
			cmidi_fired_event_t fired = decode_event(event, cmidi_time_now_ms() - start_ms, cur_tempo, scheduler->sysex_pool);
			scheduler->callback(&fired, scheduler->callback_userdata);
		}

		send_sched_event(scheduler->port, event, scheduler->sysex_pool);
		++i;
		if (scheduler->loop_enabled && i >= loop_bound)
		{
			send_all_notes_off(scheduler->port);
			i = 0;
			++loop_pass;
		}
	}

	const int ended_naturally = !scheduler->stop_requested;
	send_all_notes_off(scheduler->port);
	mutex_lock(&scheduler->mutex);
	scheduler->playing = 0;
	scheduler->current_tick = 0;
	cond_signal(&scheduler->done_cond, &scheduler->mutex);
	mutex_unlock(&scheduler->mutex);

	if (ended_naturally && scheduler->end_callback)
		scheduler->end_callback(scheduler->end_callback_userdata);
}

#if defined(_WIN32) || defined(WIN32)
static DWORD WINAPI thread_proc(LPVOID arg)
{
	run_playback((cmidi_scheduler_t*)arg);
	return 0;
}
#else
static void* thread_proc(void* arg)
{
	run_playback((cmidi_scheduler_t*)arg);
	return NULL;
}
#endif

cmidi_scheduler_t* cmidi_scheduler_create(cmidi_port_t* port)
{
	cmidi_scheduler_t* scheduler = (cmidi_scheduler_t*)malloc(sizeof(*scheduler));
	if (!scheduler)
		return NULL;

	*scheduler = (cmidi_scheduler_t){
		.port = port,
		.ticks_per_beat = 480,
		.tempo_map[0].tick = 0,
		.tempo_map[0].tempo_us = 500000,
		.tempo_map[0].time_ms = 0.0,
		.tempo_count = 1
	};

	mutex_init(&scheduler->mutex);
	cond_init(&scheduler->done_cond);
	return scheduler;
}

void cmidi_scheduler_destroy(cmidi_scheduler_t* scheduler)
{
	if (!scheduler)
		return;

	cmidi_scheduler_stop(scheduler);
	free(scheduler->events);
	free(scheduler->sysex_pool);
	cond_destroy(&scheduler->done_cond);
	mutex_destroy(&scheduler->mutex);
	free(scheduler);
}

int cmidi_scheduler_load_file(cmidi_scheduler_t* scheduler, const struct cmidi_file* file)
{
	if (!scheduler || !file)
		return 0;

	return cmidi_scheduler_set_events(scheduler, cmidi_file_events(file), cmidi_file_event_count(file), cmidi_file_ticks_per_beat(file), cmidi_file_sysex_pool(file));
}

int cmidi_scheduler_set_events(cmidi_scheduler_t* scheduler, const cmidi_sched_event_t* events, int count, uint16_t ticks_per_beat, const uint8_t* sysex_pool)
{
	if (!scheduler || !events || count <= 0 || ticks_per_beat == 0)
		return 0;

	size_t sysex_total = 0;
	for (int i = 0; i < count; ++i)
	{
		if (events[i].type == CMIDI_SCHED_SYSEX)
			sysex_total += events[i].sysex_length;
	}

	uint8_t* new_pool = NULL;
	if (sysex_total > 0)
	{
		if (!sysex_pool)
			return 0;

		new_pool = (uint8_t*)malloc(sysex_total);
		if (!new_pool)
			return 0;
	}

	cmidi_sched_event_t* new_events = (cmidi_sched_event_t*)malloc((size_t)count * sizeof(*new_events));
	if (!new_events)
	{
		free(new_pool);
		return 0;
	}

	size_t pool_pos = 0;
	for (int i = 0; i < count; ++i)
	{
		new_events[i] = events[i];
		if (events[i].type == CMIDI_SCHED_SYSEX)
		{
			memcpy(new_pool + pool_pos, sysex_pool + events[i].sysex_offset, events[i].sysex_length);
			new_events[i].sysex_offset = (uint32_t)pool_pos;
			pool_pos += events[i].sysex_length;
		}
	}

	free(scheduler->events);
	free(scheduler->sysex_pool);
	scheduler->events = new_events;
	scheduler->sysex_pool = new_pool;
	scheduler->sysex_pool_size = sysex_total;
	scheduler->event_count = count;
	scheduler->ticks_per_beat = ticks_per_beat;
	qsort(scheduler->events, (size_t)count, sizeof(*scheduler->events), event_cmp);
	build_tempo_map(scheduler);
	return 1;
}

const uint8_t* cmidi_scheduler_sysex_pool(const cmidi_scheduler_t* scheduler)
{
	return scheduler ? scheduler->sysex_pool : NULL;
}

void cmidi_scheduler_set_callback(cmidi_scheduler_t* scheduler, cmidi_scheduler_callback_t callback, void* ud)
{
	if (!scheduler)
		return;

	scheduler->callback = callback; scheduler->callback_userdata = ud;
}

void cmidi_scheduler_set_end_callback(cmidi_scheduler_t* scheduler, cmidi_scheduler_end_callback_t callback, void* ud)
{
	if (!scheduler)
		return;

	scheduler->end_callback = callback; scheduler->end_callback_userdata = ud;
}

void cmidi_scheduler_set_loop(cmidi_scheduler_t* scheduler, int enabled, uint32_t loop_end_tick)
{
	if (!scheduler)
		return;

	scheduler->loop_enabled = enabled; scheduler->loop_end_tick = loop_end_tick;
}

int cmidi_scheduler_start(cmidi_scheduler_t* scheduler)
{
	if (!scheduler || !scheduler->events || scheduler->event_count == 0)
		return 0;

	cmidi_scheduler_stop(scheduler);
	mutex_lock(&scheduler->mutex);
	scheduler->stop_requested = 0;
	scheduler->playing = 1;
	scheduler->current_tick = 0;
	mutex_unlock(&scheduler->mutex);
	if (!thread_start(&scheduler->thread, scheduler))
	{
		scheduler->playing = 0;
		return 0;
	}

	scheduler->thread_running = 1;
	return 1;
}

void cmidi_scheduler_stop(cmidi_scheduler_t* scheduler)
{
	if (!scheduler || !scheduler->thread_running)
		return;

	scheduler->stop_requested = 1;
	thread_join(scheduler->thread);
	scheduler->thread_running = 0;
	scheduler->stop_requested = 0;
}

void cmidi_scheduler_wait(cmidi_scheduler_t* scheduler)
{
	if (!scheduler)
		return;

	mutex_lock(&scheduler->mutex);
	while (scheduler->playing)
	{
		cond_wait(&scheduler->done_cond, &scheduler->mutex);
	}

	mutex_unlock(&scheduler->mutex);
}

int cmidi_scheduler_is_playing(const cmidi_scheduler_t* scheduler)
{
	return scheduler ? scheduler->playing : 0;
}

uint32_t cmidi_scheduler_current_tick(const cmidi_scheduler_t* scheduler)
{
	if (!scheduler)
		return 0;
		
	mutex_lock((cmidi_mutex_t*)&scheduler->mutex);
	uint32_t t = scheduler->current_tick;
	mutex_unlock((cmidi_mutex_t*)&scheduler->mutex);
	return t;
}

#endif // CMIDI_IMPLEMENTATION

/*
MIT License

Copyright (c) 2026 Dev Dynasty

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
