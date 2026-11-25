#pragma once

// Knows nothing about CLAP, so it can be unit tested or reused. Every sound is
// generated here by hand (pulse, stepped triangle, saw, LFSR noise); there is
// no ma_device and no miniaudio generator involved. The HOST owns the audio
// device and calls us when it wants samples.
//
// Threading: everything runs on the audio thread, except synth_create,
// synth_destroy, synth_activate and synth_deactivate (main thread, not while
// processing) and the three UI readers at the bottom (any thread). Nothing
// after activation allocates, locks, or touches files.

#define SYNTH_GAIN_MIN_DB (-60.0)

// CLAP's -1: no id on note-on, wildcard on note-off and choke.
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
	double shape; // Pulse width, triangle skew, saw bend or noise density: 0.5 is the plain wave in every case.
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
	int pcm_keys; // 1: the sample follows the keys, 0: every key plays it at the same pitch.
	double pcm_tune;
} SynthParams;

Synth* synth_create(void);
void synth_destroy(Synth* synth);

// max_frames is the host's per-call maximum; it sizes the scratch buffer.
bool synth_activate(Synth* synth, double sample_rate, uint32_t max_frames);
void synth_deactivate(Synth* synth);
void synth_reset(Synth* synth);

// Cheap enough to call every block. Playing notes pick up envelope, vibrato,
// arpeggio and lo-fi changes live; the waveform applies to NEW notes.
void synth_set_params(Synth* synth, const SynthParams* params);

// The sample channel's data. Safe to call while playing as long as the old bank stays alive until the audio stops.
void synth_set_pcm_bank(Synth* synth, const PcmBank* bank);

void synth_set_pitch_bend(Synth* synth, double semitones);
void synth_set_mod_wheel(Synth* synth, double amount);

void synth_note_on(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key, double velocity);
void synth_note_off(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key);
void synth_note_choke(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key);
void synth_all_notes_off(Synth* synth);

// A short fixed tone mixed in after the gain, so it is audible whatever the parameters are.
void synth_start_test_tone(Synth* synth);

// Overwrites left and right, it does not mix into them.
void synth_render(Synth* synth, float* left, float* right, uint32_t frame_count);

bool synth_pop_ended(Synth* synth, SynthEndedNote* out);

// Copies the newest samples of the output, oldest first. Returns how many were written.
uint32_t synth_scope_copy(const Synth* synth, float* out, uint32_t count);
uint32_t synth_active_voice_count(const Synth* synth);
void synth_active_keys(const Synth* synth, uint64_t* low, uint64_t* high);
