#pragma once

typedef enum ParamId
{
	PARAM_GAIN = 0,
	PARAM_WAVEFORM,
	PARAM_SHAPE,
	PARAM_ATTACK,
	PARAM_DECAY,
	PARAM_SUSTAIN,
	PARAM_RELEASE,
	PARAM_VIBRATO_DEPTH,
	PARAM_VIBRATO_RATE,
	PARAM_VIBRATO_DELAY,
	PARAM_SHAPE_MOD_DEPTH,
	PARAM_SHAPE_MOD_RATE,
	PARAM_ARP_MODE,
	PARAM_ARP_RATE,
	PARAM_SWEEP_AMOUNT,
	PARAM_SWEEP_TIME,
	PARAM_BITS,
	PARAM_DOWNSAMPLE,
	PARAM_NOISE_MODE,
	PARAM_VOICE_MODE,
	PARAM_GLIDE,
	PARAM_PCM_SAMPLE,
	PARAM_PCM_LOOP,
	PARAM_PCM_PITCH,
	PARAM_PCM_TUNE,
	PARAM_COUNT,
} ParamId;

typedef enum ParamKind
{
	PARAM_KIND_DB,
	PARAM_KIND_PERCENT,
	PARAM_KIND_MS,
	PARAM_KIND_HZ,
	PARAM_KIND_SEMITONES,
	PARAM_KIND_CENTS,
	PARAM_KIND_BITS,
	PARAM_KIND_FACTOR,
	PARAM_KIND_ENUM,
	PARAM_KIND_SAMPLE, // A whole number shown as the name of that sample.
} ParamKind;

// How the editor shows a parameter. The range, default and flags the host sees live in the Upl_ParamInfo table.
typedef struct ParamStyle
{
	ParamKind kind;
	double curve; // Knob feel only: above 1 spends more of the travel on small values. The host still sees a plain linear range.
	bool bipolar;
	const char* const* names; // ParamKind enum only.
} ParamStyle;

typedef struct GuiView GuiView;
typedef struct PresetList PresetList;

typedef struct Plugin
{
	Upl_Plugin* upl;
	Synth* synth;
	PcmBank* pcm_bank;
	PresetList* presets;
	GuiView* gui_view;
	NoteQueue gui_notes; // UI thread -> audio thread: the on-screen keyboard.
} Plugin;

void plugin_gui_note(Plugin* plugin, bool is_on, int key);

const Upl_ParamInfo* params_info(ParamId id);
const ParamStyle* params_style(ParamId id);
bool params_is_stepped(ParamId id);
double params_clamp(ParamId id, double value);
void params_format(const Plugin* plugin, ParamId id, double value, char* out, size_t capacity);
double params_to_position(ParamId id, double value);
double params_from_position(ParamId id, double position);

double params_get(const Plugin* plugin, ParamId id);
void params_read_synth(const Plugin* plugin, SynthParams* out);

void params_edit_begin(Plugin* plugin, ParamId id);
void params_edit_set(Plugin* plugin, ParamId id, double value);
void params_edit_end(Plugin* plugin, ParamId id);
