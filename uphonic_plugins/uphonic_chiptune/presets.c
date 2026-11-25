typedef struct PresetValue
{
	ParamId id;
	double value;
} PresetValue;

typedef struct FactoryPreset
{
	const char* name;
	const char* sample; // NULL unless the patch plays a PCM sample.
	const PresetValue* values; // Only what differs from the defaults; ends with PARAM_COUNT.
} FactoryPreset;

#define PRESET_END { PARAM_COUNT, 0.0 }

static const PresetValue k_preset_init[] = { PRESET_END };

static const PresetValue k_preset_nes_lead[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PULSE }, { PARAM_SHAPE, 0.25 },
	{ PARAM_ATTACK, 1.0 }, { PARAM_DECAY, 120.0 }, { PARAM_SUSTAIN, 0.75 }, { PARAM_RELEASE, 90.0 },
	{ PARAM_VIBRATO_DEPTH, 25.0 }, { PARAM_VIBRATO_RATE, 6.0 }, { PARAM_VIBRATO_DELAY, 200.0 },
	PRESET_END
};

static const PresetValue k_preset_pwm_pad[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PULSE }, { PARAM_SHAPE, 0.5 },
	{ PARAM_SHAPE_MOD_DEPTH, 0.32 }, { PARAM_SHAPE_MOD_RATE, 1.6 },
	{ PARAM_ATTACK, 40.0 }, { PARAM_DECAY, 200.0 }, { PARAM_SUSTAIN, 0.85 }, { PARAM_RELEASE, 250.0 },
	{ PARAM_VIBRATO_DEPTH, 12.0 }, { PARAM_VIBRATO_DELAY, 300.0 },
	PRESET_END
};

static const PresetValue k_preset_tri_bass[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_TRIANGLE },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 120.0 }, { PARAM_SUSTAIN, 0.85 }, { PARAM_RELEASE, 50.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_VOICE_MODE, SYNTH_VOICE_MONO }, { PARAM_GLIDE, 35.0 },
	PRESET_END
};

static const PresetValue k_preset_saw_bass[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_SAW }, { PARAM_SHAPE, 0.35 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 240.0 }, { PARAM_SUSTAIN, 0.4 }, { PARAM_RELEASE, 60.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_VOICE_MODE, SYNTH_VOICE_MONO }, { PARAM_GLIDE, 30.0 },
	{ PARAM_BITS, 8.0 },
	PRESET_END
};

static const PresetValue k_preset_arp_major[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PULSE }, { PARAM_SHAPE, 0.25 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 60.0 }, { PARAM_SUSTAIN, 0.8 }, { PARAM_RELEASE, 40.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_ARP_MODE, SYNTH_ARP_MAJOR }, { PARAM_ARP_RATE, 28.0 },
	PRESET_END
};

static const PresetValue k_preset_arp_min7[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PULSE }, { PARAM_SHAPE, 0.125 },
	{ PARAM_SHAPE_MOD_DEPTH, 0.2 }, { PARAM_SHAPE_MOD_RATE, 3.0 },
	{ PARAM_DECAY, 80.0 }, { PARAM_SUSTAIN, 0.7 }, { PARAM_RELEASE, 60.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_ARP_MODE, SYNTH_ARP_MIN7 }, { PARAM_ARP_RATE, 22.0 },
	PRESET_END
};

static const PresetValue k_preset_power_arp[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_SAW }, { PARAM_SHAPE, 0.5 },
	{ PARAM_SUSTAIN, 0.8 }, { PARAM_RELEASE, 50.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_ARP_MODE, SYNTH_ARP_POWER }, { PARAM_ARP_RATE, 36.0 },
	PRESET_END
};

static const PresetValue k_preset_laser[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PULSE }, { PARAM_SHAPE, 0.5 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 200.0 }, { PARAM_SUSTAIN, 0.0 }, { PARAM_RELEASE, 30.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_SWEEP_AMOUNT, 24.0 }, { PARAM_SWEEP_TIME, 190.0 },
	PRESET_END
};

static const PresetValue k_preset_kick[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_TRIANGLE },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 130.0 }, { PARAM_SUSTAIN, 0.0 }, { PARAM_RELEASE, 20.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_SWEEP_AMOUNT, 24.0 }, { PARAM_SWEEP_TIME, 80.0 },
	PRESET_END
};

static const PresetValue k_preset_snare[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_NOISE }, { PARAM_NOISE_MODE, SYNTH_NOISE_LONG }, { PARAM_SHAPE, 0.5 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 150.0 }, { PARAM_SUSTAIN, 0.0 }, { PARAM_RELEASE, 30.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_BITS, 5.0 },
	PRESET_END
};

static const PresetValue k_preset_hat[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_NOISE }, { PARAM_NOISE_MODE, SYNTH_NOISE_SHORT }, { PARAM_SHAPE, 0.15 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 45.0 }, { PARAM_SUSTAIN, 0.0 }, { PARAM_RELEASE, 10.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_BITS, 6.0 },
	PRESET_END
};

static const PresetValue k_preset_crunch_lead[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_SAW }, { PARAM_SHAPE, 0.5 },
	{ PARAM_SUSTAIN, 0.8 }, { PARAM_VIBRATO_DEPTH, 20.0 }, { PARAM_VIBRATO_DELAY, 150.0 },
	{ PARAM_BITS, 5.0 }, { PARAM_DOWNSAMPLE, 3.0 },
	PRESET_END
};

static const PresetValue k_preset_siren[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_TRIANGLE },
	{ PARAM_ATTACK, 80.0 }, { PARAM_SUSTAIN, 1.0 }, { PARAM_RELEASE, 200.0 },
	{ PARAM_VIBRATO_DEPTH, 160.0 }, { PARAM_VIBRATO_RATE, 5.0 }, { PARAM_VIBRATO_DELAY, 0.0 },
	PRESET_END
};

static const PresetValue k_preset_blip[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PULSE }, { PARAM_SHAPE, 0.125 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_DECAY, 110.0 }, { PARAM_SUSTAIN, 0.0 }, { PARAM_RELEASE, 20.0 },
	{ PARAM_VIBRATO_DEPTH, 0.0 }, { PARAM_SWEEP_AMOUNT, 12.0 }, { PARAM_SWEEP_TIME, 55.0 },
	PRESET_END
};

static const PresetValue k_preset_pcm_kick[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PCM }, { PARAM_PCM_LOOP, 0.0 }, { PARAM_PCM_PITCH, 0.0 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_SUSTAIN, 1.0 }, { PARAM_RELEASE, 20.0 }, { PARAM_VIBRATO_DEPTH, 0.0 },
	PRESET_END
};

static const PresetValue k_preset_pcm_coin[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PCM }, { PARAM_PCM_LOOP, 0.0 }, { PARAM_PCM_PITCH, 0.0 },
	{ PARAM_ATTACK, 0.0 }, { PARAM_SUSTAIN, 1.0 }, { PARAM_RELEASE, 20.0 }, { PARAM_VIBRATO_DEPTH, 0.0 },
	PRESET_END
};

static const PresetValue k_preset_wave_organ[] =
{
	{ PARAM_WAVEFORM, SYNTH_WAVE_PCM }, { PARAM_PCM_LOOP, 1.0 }, { PARAM_PCM_PITCH, 1.0 },
	{ PARAM_ATTACK, 4.0 }, { PARAM_SUSTAIN, 1.0 }, { PARAM_RELEASE, 80.0 },
	{ PARAM_VIBRATO_DEPTH, 12.0 }, { PARAM_VIBRATO_DELAY, 250.0 },
	PRESET_END
};

static const FactoryPreset k_factory_presets[] =
{
	{ "INIT", NULL, k_preset_init },
	{ "NES LEAD", NULL, k_preset_nes_lead },
	{ "PWM PAD", NULL, k_preset_pwm_pad },
	{ "TRI BASS", NULL, k_preset_tri_bass },
	{ "SAW BASS", NULL, k_preset_saw_bass },
	{ "ARP MAJOR", NULL, k_preset_arp_major },
	{ "ARP MIN7", NULL, k_preset_arp_min7 },
	{ "POWER ARP", NULL, k_preset_power_arp },
	{ "LASER", NULL, k_preset_laser },
	{ "KICK", NULL, k_preset_kick },
	{ "SNARE", NULL, k_preset_snare },
	{ "HI-HAT", NULL, k_preset_hat },
	{ "CRUNCH LEAD", NULL, k_preset_crunch_lead },
	{ "SIREN", NULL, k_preset_siren },
	{ "BLIP", NULL, k_preset_blip },
	{ "PCM KICK", "KICK", k_preset_pcm_kick },
	{ "PCM COIN", "COIN", k_preset_pcm_coin },
	{ "WAVE ORGAN", "ORGAN4", k_preset_wave_organ },
};

#pragma region Private API

static void _up_preset_defaults(PresetEntry* entry)
{
	for(int i = 0; i < PARAM_COUNT; i++)
	{
		entry->values[i] = params_info((ParamId)i)->default_value;
	}

	entry->sample_name[0] = '\0';
}

static void _up_add_factory(PresetList* list, const FactoryPreset* factory)
{
	PresetEntry* entry = &list->entries[list->count];
	memset(entry, 0, sizeof(*entry));

	snprintf(entry->name, sizeof(entry->name), "%s", factory->name);
	_up_preset_defaults(entry);

	for(const PresetValue* value = factory->values; value->id != PARAM_COUNT; value++)
	{
		entry->values[value->id] = params_clamp(value->id, value->value);
	}

	if(factory->sample != NULL)
		snprintf(entry->sample_name, sizeof(entry->sample_name), "%s", factory->sample);

	list->count++;
}

#pragma endregion

#pragma region Public API

PresetList* presets_create(void)
{
	PresetList* list = (PresetList*)calloc(1, sizeof(PresetList));
	if(list == NULL)
		return NULL;

	for(size_t i = 0; i < ARRAY_COUNT(k_factory_presets); i++)
	{
		_up_add_factory(list, &k_factory_presets[i]);
	}

	return list;
}

void presets_destroy(PresetList* list)
{
	free(list);
}

int presets_resolve_sample(const PresetEntry* entry, const PcmBank* bank)
{
	if(entry->sample_name[0] == '\0')
		return -1;

	return pcm_bank_find(bank, entry->sample_name);
}

#pragma endregion
