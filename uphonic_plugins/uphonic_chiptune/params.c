#define PARAM_GAIN_MAX_DB 6.0
#define PARAM_GAIN_DEFAULT_DB (-6.0)
#define NEG_INF_TEXT "-inf"

#define PARAM_PERCENT_SCALE 100.0
#define PARAM_MS_PER_SECOND 1000.0
#define PARAM_MS_SECONDS_THRESHOLD 1000.0
#define PARAM_MS_DECIMALS_THRESHOLD 10.0
#define PARAM_HZ_DECIMALS_THRESHOLD 10.0

static const char* const k_wave_names[SYNTH_WAVE_COUNT] = { "Pulse", "Triangle", "Saw", "Noise", "PCM" };
static const char* const k_arp_names[SYNTH_ARP_COUNT] = { "Off", "Major", "Minor", "Dom7", "Maj7", "Min7", "Octaves", "Power" };
static const char* const k_noise_names[SYNTH_NOISE_COUNT] = { "Long", "Short" };
static const char* const k_voice_names[SYNTH_VOICE_COUNT] = { "Poly", "Mono" };
static const char* const k_pcm_loop_names[2] = { "One-shot", "Loop" };
static const char* const k_pcm_pitch_names[2] = { "Fixed", "Keys" };

static const Upl_ParamInfo k_param_infos[PARAM_COUNT] =
{
	[PARAM_GAIN] = { .name = "Gain", .unit = "dB", .min = SYNTH_GAIN_MIN_DB, .max = PARAM_GAIN_MAX_DB, .default_value = PARAM_GAIN_DEFAULT_DB },
	[PARAM_WAVEFORM] = { .name = "Waveform", .min = 0.0, .max = SYNTH_WAVE_COUNT - 1, .default_value = SYNTH_WAVE_PULSE, .flags = UPL_PARAM_STEPPED },
	[PARAM_SHAPE] = { .name = "Shape", .min = 0.05, .max = 0.95, .default_value = 0.5 },
	[PARAM_ATTACK] = { .name = "Attack", .unit = "ms", .min = 0.0, .max = 1000.0, .default_value = 1.0 },
	[PARAM_DECAY] = { .name = "Decay", .unit = "ms", .min = 0.0, .max = 2000.0, .default_value = 120.0 },
	[PARAM_SUSTAIN] = { .name = "Sustain", .min = 0.0, .max = 1.0, .default_value = 0.75 },
	[PARAM_RELEASE] = { .name = "Release", .unit = "ms", .min = 0.0, .max = 2000.0, .default_value = 100.0 },
	[PARAM_VIBRATO_DEPTH] = { .name = "Vibrato Depth", .unit = "ct", .min = 0.0, .max = 200.0, .default_value = 20.0 },
	[PARAM_VIBRATO_RATE] = { .name = "Vibrato Rate", .unit = "Hz", .min = 0.1, .max = 20.0, .default_value = 6.0 },
	[PARAM_VIBRATO_DELAY] = { .name = "Vibrato Delay", .unit = "ms", .min = 0.0, .max = 2000.0, .default_value = 250.0 },
	[PARAM_SHAPE_MOD_DEPTH] = { .name = "Shape Mod Depth", .min = 0.0, .max = 0.45, .default_value = 0.0 },
	[PARAM_SHAPE_MOD_RATE] = { .name = "Shape Mod Rate", .unit = "Hz", .min = 0.1, .max = 20.0, .default_value = 3.0 },
	[PARAM_ARP_MODE] = { .name = "Arp Mode", .min = 0.0, .max = SYNTH_ARP_COUNT - 1, .default_value = SYNTH_ARP_OFF, .flags = UPL_PARAM_STEPPED },
	[PARAM_ARP_RATE] = { .name = "Arp Rate", .unit = "Hz", .min = 1.0, .max = 60.0, .default_value = 20.0 },
	[PARAM_SWEEP_AMOUNT] = { .name = "Sweep Amount", .unit = "st", .min = -24.0, .max = 24.0, .default_value = 0.0, .flags = UPL_PARAM_STEPPED },
	[PARAM_SWEEP_TIME] = { .name = "Sweep Time", .unit = "ms", .min = 0.0, .max = 2000.0, .default_value = 100.0 },
	[PARAM_BITS] = { .name = "Bit Depth", .unit = "bit", .min = SYNTH_BITS_MIN, .max = SYNTH_BITS_MAX, .default_value = SYNTH_BITS_MAX, .flags = UPL_PARAM_STEPPED },
	[PARAM_DOWNSAMPLE] = { .name = "Downsample", .min = 1.0, .max = SYNTH_DOWNSAMPLE_MAX, .default_value = 1.0, .flags = UPL_PARAM_STEPPED },
	[PARAM_NOISE_MODE] = { .name = "Noise Mode", .min = 0.0, .max = SYNTH_NOISE_COUNT - 1, .default_value = SYNTH_NOISE_LONG, .flags = UPL_PARAM_STEPPED },
	[PARAM_VOICE_MODE] = { .name = "Voice Mode", .min = 0.0, .max = SYNTH_VOICE_COUNT - 1, .default_value = SYNTH_VOICE_POLY, .flags = UPL_PARAM_STEPPED },
	[PARAM_GLIDE] = { .name = "Glide", .unit = "ms", .min = 0.0, .max = 1000.0, .default_value = 60.0 },
	[PARAM_PCM_SAMPLE] = { .name = "PCM Sample", .min = 0.0, .max = PCM_MAX_SAMPLES - 1, .default_value = 0.0, .flags = UPL_PARAM_STEPPED },
	[PARAM_PCM_LOOP] = { .name = "PCM Loop", .min = 0.0, .max = 1.0, .default_value = 0.0, .flags = UPL_PARAM_STEPPED },
	[PARAM_PCM_PITCH] = { .name = "PCM Pitch", .min = 0.0, .max = 1.0, .default_value = 1.0, .flags = UPL_PARAM_STEPPED },
	[PARAM_PCM_TUNE] = { .name = "PCM Tune", .unit = "st", .min = -24.0, .max = 24.0, .default_value = 0.0, .flags = UPL_PARAM_STEPPED },
};

static const ParamStyle k_param_styles[PARAM_COUNT] =
{
	[PARAM_GAIN] = { .kind = PARAM_KIND_DB, .curve = 1.0 },
	[PARAM_WAVEFORM] = { .kind = PARAM_KIND_ENUM, .curve = 1.0, .names = k_wave_names },
	[PARAM_SHAPE] = { .kind = PARAM_KIND_PERCENT, .curve = 1.0 },
	[PARAM_ATTACK] = { .kind = PARAM_KIND_MS, .curve = 3.0 },
	[PARAM_DECAY] = { .kind = PARAM_KIND_MS, .curve = 3.0 },
	[PARAM_SUSTAIN] = { .kind = PARAM_KIND_PERCENT, .curve = 1.0 },
	[PARAM_RELEASE] = { .kind = PARAM_KIND_MS, .curve = 3.0 },
	[PARAM_VIBRATO_DEPTH] = { .kind = PARAM_KIND_CENTS, .curve = 1.5 },
	[PARAM_VIBRATO_RATE] = { .kind = PARAM_KIND_HZ, .curve = 1.5 },
	[PARAM_VIBRATO_DELAY] = { .kind = PARAM_KIND_MS, .curve = 2.0 },
	[PARAM_SHAPE_MOD_DEPTH] = { .kind = PARAM_KIND_PERCENT, .curve = 1.0 },
	[PARAM_SHAPE_MOD_RATE] = { .kind = PARAM_KIND_HZ, .curve = 1.5 },
	[PARAM_ARP_MODE] = { .kind = PARAM_KIND_ENUM, .curve = 1.0, .names = k_arp_names },
	[PARAM_ARP_RATE] = { .kind = PARAM_KIND_HZ, .curve = 1.5 },
	[PARAM_SWEEP_AMOUNT] = { .kind = PARAM_KIND_SEMITONES, .curve = 1.0, .bipolar = true },
	[PARAM_SWEEP_TIME] = { .kind = PARAM_KIND_MS, .curve = 3.0 },
	[PARAM_BITS] = { .kind = PARAM_KIND_BITS, .curve = 1.0 },
	[PARAM_DOWNSAMPLE] = { .kind = PARAM_KIND_FACTOR, .curve = 2.0 },
	[PARAM_NOISE_MODE] = { .kind = PARAM_KIND_ENUM, .curve = 1.0, .names = k_noise_names },
	[PARAM_VOICE_MODE] = { .kind = PARAM_KIND_ENUM, .curve = 1.0, .names = k_voice_names },
	[PARAM_GLIDE] = { .kind = PARAM_KIND_MS, .curve = 2.0 },
	[PARAM_PCM_SAMPLE] = { .kind = PARAM_KIND_SAMPLE, .curve = 1.0 },
	[PARAM_PCM_LOOP] = { .kind = PARAM_KIND_ENUM, .curve = 1.0, .names = k_pcm_loop_names },
	[PARAM_PCM_PITCH] = { .kind = PARAM_KIND_ENUM, .curve = 1.0, .names = k_pcm_pitch_names },
	[PARAM_PCM_TUNE] = { .kind = PARAM_KIND_SEMITONES, .curve = 1.0, .bipolar = true },
};

#pragma region Public API

const Upl_ParamInfo* params_info(ParamId id)
{
	return &k_param_infos[id];
}

const ParamStyle* params_style(ParamId id)
{
	return &k_param_styles[id];
}

bool params_is_stepped(ParamId id)
{
	return (k_param_infos[id].flags & UPL_PARAM_STEPPED) != 0;
}

double params_clamp(ParamId id, double value)
{
	const Upl_ParamInfo* info = &k_param_infos[id];

	if(isnan(value))
		return info->default_value;

	if(value < info->min)
		value = info->min;

	if(value > info->max)
		value = info->max;

	if(params_is_stepped(id))
		value = floor(value + 0.5);

	return value;
}

void params_format(const Plugin* plugin, ParamId id, double value, char* out, size_t capacity)
{
	const ParamStyle* style = &k_param_styles[id];
	value = params_clamp(id, value);

	switch(style->kind)
	{
		case PARAM_KIND_DB:
			if(value <= k_param_infos[id].min)
				snprintf(out, capacity, NEG_INF_TEXT "dB");
			else
				snprintf(out, capacity, "%.1fdB", value);
			return;

		case PARAM_KIND_PERCENT:
			snprintf(out, capacity, "%.0f%%", value * PARAM_PERCENT_SCALE);
			return;

		case PARAM_KIND_MS:
			if(value >= PARAM_MS_SECONDS_THRESHOLD)
				snprintf(out, capacity, "%.2fs", value / PARAM_MS_PER_SECOND);
			else if(value < PARAM_MS_DECIMALS_THRESHOLD)
				snprintf(out, capacity, "%.1fms", value);
			else
				snprintf(out, capacity, "%.0fms", value);
			return;

		case PARAM_KIND_HZ:
			if(value < PARAM_HZ_DECIMALS_THRESHOLD)
				snprintf(out, capacity, "%.1fHz", value);
			else
				snprintf(out, capacity, "%.0fHz", value);
			return;

		case PARAM_KIND_SEMITONES:
			snprintf(out, capacity, "%+.0fst", value);
			return;

		case PARAM_KIND_CENTS:
			snprintf(out, capacity, "%.0fct", value);
			return;

		case PARAM_KIND_BITS:
			snprintf(out, capacity, "%.0fbit", value);
			return;

		case PARAM_KIND_FACTOR:
			snprintf(out, capacity, "x%.0f", value);
			return;

		case PARAM_KIND_ENUM:
			snprintf(out, capacity, "%s", style->names[(int)value]);
			return;

		case PARAM_KIND_SAMPLE:
		{
			const PcmSample* sample = pcm_bank_get(plugin->pcm_bank, (int)value);
			snprintf(out, capacity, "%s", sample != NULL ? sample->name : "---");
			return;
		}
	}
}

double params_to_position(ParamId id, double value)
{
	const Upl_ParamInfo* info = &k_param_infos[id];
	const double range = info->max - info->min;

	if(range <= 0.0)
		return 0.0;

	double normalized = (params_clamp(id, value) - info->min) / range;
	return pow(normalized, 1.0 / k_param_styles[id].curve);
}

double params_from_position(ParamId id, double position)
{
	const Upl_ParamInfo* info = &k_param_infos[id];

	if(position < 0.0)
		position = 0.0;

	if(position > 1.0)
		position = 1.0;

	return params_clamp(id, info->min + (info->max - info->min) * pow(position, k_param_styles[id].curve));
}

double params_get(const Plugin* plugin, ParamId id)
{
	return upl_param(plugin->upl, id);
}

void params_read_synth(const Plugin* plugin, SynthParams* out)
{
	*out = (SynthParams)
	{
		.gain_db = params_get(plugin, PARAM_GAIN),
		.waveform = (int)params_get(plugin, PARAM_WAVEFORM),
		.shape = params_get(plugin, PARAM_SHAPE),
		.shape_mod_depth = params_get(plugin, PARAM_SHAPE_MOD_DEPTH),
		.shape_mod_rate = params_get(plugin, PARAM_SHAPE_MOD_RATE),
		.attack_ms = params_get(plugin, PARAM_ATTACK),
		.decay_ms = params_get(plugin, PARAM_DECAY),
		.sustain = params_get(plugin, PARAM_SUSTAIN),
		.release_ms = params_get(plugin, PARAM_RELEASE),
		.vibrato_cents = params_get(plugin, PARAM_VIBRATO_DEPTH),
		.vibrato_rate = params_get(plugin, PARAM_VIBRATO_RATE),
		.vibrato_delay_ms = params_get(plugin, PARAM_VIBRATO_DELAY),
		.arp_mode = (int)params_get(plugin, PARAM_ARP_MODE),
		.arp_rate = params_get(plugin, PARAM_ARP_RATE),
		.sweep_semitones = params_get(plugin, PARAM_SWEEP_AMOUNT),
		.sweep_ms = params_get(plugin, PARAM_SWEEP_TIME),
		.bits = (int)params_get(plugin, PARAM_BITS),
		.downsample = (int)params_get(plugin, PARAM_DOWNSAMPLE),
		.noise_mode = (int)params_get(plugin, PARAM_NOISE_MODE),
		.voice_mode = (int)params_get(plugin, PARAM_VOICE_MODE),
		.glide_ms = params_get(plugin, PARAM_GLIDE),
		.pcm_sample = (int)params_get(plugin, PARAM_PCM_SAMPLE),
		.pcm_loop = (int)params_get(plugin, PARAM_PCM_LOOP),
		.pcm_keys = (int)params_get(plugin, PARAM_PCM_PITCH),
		.pcm_tune = params_get(plugin, PARAM_PCM_TUNE),
	};
}

void params_edit_begin(Plugin* plugin, ParamId id)
{
	upl_param_begin_gesture(plugin->upl, id);
}

void params_edit_set(Plugin* plugin, ParamId id, double value)
{
	upl_param_set(plugin->upl, id, value);
}

void params_edit_end(Plugin* plugin, ParamId id)
{
	upl_param_end_gesture(plugin->upl, id);
}

#pragma endregion
