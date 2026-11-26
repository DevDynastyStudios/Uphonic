#pragma once

#define SYNTH_GAIN_MIN_DB (-60.0)

#define SYNTH_UNSPECIFIED (-1)

#define SYNTH_SCOPE_SIZE 2048

#define SYNTH_BITS_MIN 2
#define SYNTH_BITS_MAX 16
#define SYNTH_DOWNSAMPLE_MAX 32

typedef struct Synth Synth;

typedef struct SynthEndedNote
{
	int32_t note_id;
	int16_t port;
	int16_t channel;
	int16_t key;
} SynthEndedNote;

enum
{
	SYNTH_WAVE_PULSE,
	SYNTH_WAVE_TRIANGLE,
	SYNTH_WAVE_SAW,
	SYNTH_WAVE_NOISE,
	SYNTH_WAVE_PCM,
	SYNTH_WAVE_COUNT,
};

enum
{
	SYNTH_ARP_OFF,
	SYNTH_ARP_MAJOR,
	SYNTH_ARP_MINOR,
	SYNTH_ARP_DOM7,
	SYNTH_ARP_MAJ7,
	SYNTH_ARP_MIN7,
	SYNTH_ARP_OCTAVES,
	SYNTH_ARP_POWER,
	SYNTH_ARP_COUNT,
};

enum
{
	SYNTH_NOISE_LONG,
	SYNTH_NOISE_SHORT,
	SYNTH_NOISE_COUNT,
};

enum
{
	SYNTH_VOICE_POLY,
	SYNTH_VOICE_MONO,
	SYNTH_VOICE_COUNT,
};

typedef struct SynthParams
{
	double gain_db;
	int waveform;
	double shape;
	double shape_mod_depth;
	double shape_mod_rate;
	double attack_ms;
	double decay_ms;
	double sustain;
	double release_ms;
	double vibrato_cents;
	double vibrato_rate;
	double vibrato_delay_ms;
	int arp_mode;
	double arp_rate;
	double sweep_semitones;
	double sweep_ms;
	int bits;
	int downsample;
	int noise_mode;
	int voice_mode;
	double glide_ms;
	int pcm_sample;
	int pcm_loop;
	int pcm_keys;
	double pcm_tune;
} SynthParams;

Synth* synth_create(void);
void synth_destroy(Synth* synth);

bool synth_activate(Synth* synth, double sample_rate, uint32_t max_frames);
void synth_deactivate(Synth* synth);
void synth_reset(Synth* synth);

void synth_set_params(Synth* synth, const SynthParams* params);

// The sample channel's data. Safe to call while playing as long as the old bank stays alive until the audio stops.
void synth_set_pcm_bank(Synth* synth, const PcmBank* bank);

void synth_set_pitch_bend(Synth* synth, double semitones);
void synth_set_mod_wheel(Synth* synth, double amount);

void synth_note_on(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key, double velocity);
void synth_note_off(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key);
void synth_note_choke(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key);
void synth_all_notes_off(Synth* synth);

void synth_start_test_tone(Synth* synth);

// Overwrites left and right, does not mix them.
void synth_render(Synth* synth, float* left, float* right, uint32_t frame_count);

bool synth_pop_ended(Synth* synth, SynthEndedNote* out);

// Copies the newest samples of the output, oldest -> newest. Returns how many were written.
uint32_t synth_scope_copy(const Synth* synth, float* out, uint32_t count);
uint32_t synth_active_voice_count(const Synth* synth);
void synth_active_keys(const Synth* synth, uint64_t* low, uint64_t* high);
