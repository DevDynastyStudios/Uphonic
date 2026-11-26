#define ENV_MIN_LEVEL 0.001
#define ENV_MIN_SECONDS 0.001
#define GAIN_SMOOTH_SECS 0.005
#define VOICE_LEVEL 0.4f
#define NOISE_LEVEL 0.7f
#define INITIAL_GAIN 0.25f
#define ENDED_CAPACITY (SYNTH_MAX_VOICES * 4)

#define DEFAULT_SAMPLE_RATE 48000.0
#define DEFAULT_MAX_FRAMES 4096

#define NOTE_A4_FREQUENCY 440.0
#define NOTE_A4_KEY 69.0
#define NOTE_SEMITONES_PER_OCTAVE 12.0
#define CENTS_PER_SEMITONE 100.0

#define CONTROL_BLOCK_FRAMES 32

#define VIBRATO_FADE_SECONDS 0.15
#define MOD_WHEEL_VIBRATO_CENTS 100.0
#define GLIDE_TIME_CONSTANTS 3.0
#define GLIDE_SNAP_SEMITONES 0.001

#define SHAPE_MIN 0.02
#define SHAPE_MAX 0.98
#define SHAPE_NEUTRAL 0.5
#define MAX_FREQUENCY_RATIO 0.45
#define MIN_FREQUENCY 1.0

#define TRIANGLE_STEPS 32
#define TRIANGLE_LEVELS 16

#define NOISE_CLOCK_MULTIPLIER 16.0
#define NOISE_MAX_STEPS_PER_FRAME 16
#define LFSR_SEED 1
#define LFSR_BITS 15
#define LFSR_TAP_LONG 1
#define LFSR_TAP_SHORT 6

#define SOFT_CLIP_LIMIT 1.5f
#define SOFT_CLIP_COEFFICIENT (4.0f / 27.0f)

#define BITS_FULL SYNTH_BITS_MAX
#define NOISE_DENSITY_BITS 8
#define PCM_TUNE_LIMIT 48.0
#define MONO_VOICE_INDEX 0
#define ARP_MAX_STEPS 4

#define TEST_TONE_SECONDS 0.5
#define TEST_TONE_FADE_SECONDS 0.01
#define TEST_TONE_LEVEL 0.2f
#define SYNTH_TWO_PI 6.283185307179586

#define KEYS_PER_MASK 64
#define KEY_COUNT 128

typedef enum
{
	ENV_OFF = 0,
	ENV_ATTACK,
	ENV_DECAY,
	ENV_SUSTAIN,
	ENV_RELEASE,
} EnvStage;

typedef struct ArpPattern
{
	int count;
	int offsets[ARP_MAX_STEPS];
} ArpPattern;

typedef struct Voice
{
	EnvStage stage;
	float envelope;
	float velocity;
	float release_decrement;
	int32_t note_id;
	int16_t port;
	int16_t channel;
	int16_t key;
	uint64_t age;

	const PcmSample* pcm;
	double pcm_position;
	double pcm_step;
	bool pcm_done;
	double phase;
	double phase_increment;
	double duty;
	double pitch_semitones;
	double target_semitones;
	double elapsed_seconds;
	double vibrato_phase;
	double shape_mod_phase;

	uint16_t lfsr;
	double noise_phase;
	double noise_increment;
	float noise_value;
} Voice;

typedef struct HeldNote
{
	int32_t note_id;
	int16_t port;
	int16_t channel;
	int16_t key;
	float velocity;
} HeldNote;

struct Synth
{
	Voice voices[SYNTH_MAX_VOICES];
	uint64_t age_counter;

	HeldNote held[SYNTH_MAX_VOICES];
	int held_count;

	SynthParams params;
	double sample_rate;
	uint32_t max_frames;
	float* scratch;

	float gain_target;
	float gain_current;
	float gain_coefficient;
	float attack_increment;
	float decay_decrement;
	double release_seconds;

	double bend_semitones;
	double mod_wheel;

	double last_semitones;
	bool has_last_note;
	_Atomic(const PcmBank*) pcm_bank;

	float held_sample;
	int hold_counter;

	SynthEndedNote ended[ENDED_CAPACITY];
	uint32_t ended_count;

	double test_tone_phase;
	uint32_t test_tone_frames_total;
	uint32_t test_tone_frames_left;

	_Atomic float scope[SYNTH_SCOPE_SIZE];
	_Atomic uint32_t scope_position;
	_Atomic uint32_t active_voices;
	_Atomic uint64_t active_keys[2];
};

static const ArpPattern k_arp_patterns[SYNTH_ARP_COUNT] =
{
	[SYNTH_ARP_OFF] = { 1, { 0 } },
	[SYNTH_ARP_MAJOR] = { 3, { 0, 4, 7 } },
	[SYNTH_ARP_MINOR] = { 3, { 0, 3, 7 } },
	[SYNTH_ARP_DOM7] = { 4, { 0, 4, 7, 10 } },
	[SYNTH_ARP_MAJ7] = { 4, { 0, 4, 7, 11 } },
	[SYNTH_ARP_MIN7] = { 4, { 0, 3, 7, 10 } },
	[SYNTH_ARP_OCTAVES] = { 2, { 0, 12 } },
	[SYNTH_ARP_POWER] = { 3, { 0, 7, 12 } },
};

#pragma region Private API

static float uct__clamp_velocity(double velocity)
{
	if(velocity < 0.0)
		return 0.0f;

	if(velocity > 1.0)
		return 1.0f;

	return (float)velocity;
}

static double uct__clamp(double value, double low, double high)
{
	if(value < low)
		return low;

	if(value > high)
		return high;

	return value;
}

static double uct__wrap_phase(double phase)
{
	return phase - floor(phase);
}

static bool uct__is_held_stage(EnvStage stage)
{
	return stage == ENV_ATTACK || stage == ENV_DECAY || stage == ENV_SUSTAIN;
}

static void uct__push_ended_note(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key)
{
	if(note_id < 0)
		return;

	if(synth->ended_count >= ENDED_CAPACITY)
		return;

	SynthEndedNote* ended = &synth->ended[synth->ended_count];
	ended->note_id = note_id;
	ended->port = port;
	ended->channel = channel;
	ended->key = key;

	synth->ended_count++;
}

static void uct__push_ended(Synth* synth, const Voice* voice)
{
	uct__push_ended_note(synth, voice->note_id, voice->port, voice->channel, voice->key);
}

static void uct__push_ended_held(Synth* synth, const HeldNote* note)
{
	uct__push_ended_note(synth, note->note_id, note->port, note->channel, note->key);
}

static void uct__update_rates(Synth* synth)
{
	double sample_rate = synth->sample_rate;
	if(sample_rate <= 0.0)
		sample_rate = DEFAULT_SAMPLE_RATE;

	const SynthParams* params = &synth->params;
	double attack_seconds = fmax(params->attack_ms * 0.001, ENV_MIN_SECONDS);
	double decay_seconds = fmax(params->decay_ms * 0.001, ENV_MIN_SECONDS);
	double release_seconds = fmax(params->release_ms * 0.001, ENV_MIN_SECONDS);

	synth->attack_increment = (float)(1.0 / (attack_seconds * sample_rate));
	synth->decay_decrement = (float)((1.0 - params->sustain) / (decay_seconds * sample_rate));
	synth->release_seconds = release_seconds;
	synth->gain_coefficient = (float)(1.0 - exp(-1.0 / (GAIN_SMOOTH_SECS * sample_rate)));
}

static void uct__sanitize_params(SynthParams* params)
{
	params->waveform = (int)uct__clamp(params->waveform, 0, SYNTH_WAVE_COUNT - 1);
	params->pcm_sample = (int)uct__clamp(params->pcm_sample, 0, PCM_MAX_SAMPLES - 1);
	params->pcm_loop = (int)uct__clamp(params->pcm_loop, 0, 1);
	params->pcm_keys = (int)uct__clamp(params->pcm_keys, 0, 1);
	params->pcm_tune = uct__clamp(params->pcm_tune, -PCM_TUNE_LIMIT, PCM_TUNE_LIMIT);
	params->arp_mode = (int)uct__clamp(params->arp_mode, 0, SYNTH_ARP_COUNT - 1);
	params->noise_mode = (int)uct__clamp(params->noise_mode, 0, SYNTH_NOISE_COUNT - 1);
	params->voice_mode = (int)uct__clamp(params->voice_mode, 0, SYNTH_VOICE_COUNT - 1);
	params->bits = (int)uct__clamp(params->bits, SYNTH_BITS_MIN, SYNTH_BITS_MAX);
	params->downsample = (int)uct__clamp(params->downsample, 1, SYNTH_DOWNSAMPLE_MAX);
	params->sustain = uct__clamp(params->sustain, 0.0, 1.0);
	params->shape = uct__clamp(params->shape, SHAPE_MIN, SHAPE_MAX);
	params->shape_mod_depth = fmax(params->shape_mod_depth, 0.0);
	params->attack_ms = fmax(params->attack_ms, 0.0);
	params->decay_ms = fmax(params->decay_ms, 0.0);
	params->release_ms = fmax(params->release_ms, 0.0);
	params->vibrato_cents = fmax(params->vibrato_cents, 0.0);
	params->vibrato_delay_ms = fmax(params->vibrato_delay_ms, 0.0);
	params->glide_ms = fmax(params->glide_ms, 0.0);
	params->sweep_ms = fmax(params->sweep_ms, 0.0);
}

static Voice* uct__find_voice_for_note_on(Synth* synth)
{
	Voice* oldest = NULL;
	Voice* oldest_releasing = NULL;

	for(int i = 0; i < SYNTH_MAX_VOICES; i++)
	{
		Voice* voice = &synth->voices[i];

		if(voice->stage == ENV_OFF)
			return voice;

		if(oldest == NULL || voice->age < oldest->age)
			oldest = voice;

		if(voice->stage != ENV_RELEASE)
			continue;

		if(oldest_releasing == NULL || voice->age < oldest_releasing->age)
			oldest_releasing = voice;
	}

	Voice* stolen = oldest_releasing != NULL ? oldest_releasing : oldest;
	uct__push_ended(synth, stolen);
	return stolen;
}

static bool uct__note_matches(int32_t note_id, int16_t port, int16_t channel, int16_t key, int32_t want_id, int16_t want_port, int16_t want_channel, int16_t want_key)
{
	if(want_id != SYNTH_UNSPECIFIED && note_id != want_id)
		return false;

	if(want_port != SYNTH_UNSPECIFIED && port != want_port)
		return false;

	if(want_channel != SYNTH_UNSPECIFIED && channel != want_channel)
		return false;

	if(want_key != SYNTH_UNSPECIFIED && key != want_key)
		return false;

	return true;
}

static bool uct__voice_matches(const Voice* voice, int32_t note_id, int16_t port, int16_t channel, int16_t key)
{
	return uct__note_matches(voice->note_id, voice->port, voice->channel, voice->key, note_id, port, channel, key);
}

static void uct__start_voice(Synth* synth, Voice* voice, int32_t note_id, int16_t port, int16_t channel, int16_t key, float velocity, bool keep_pitch)
{
	if(voice->stage == ENV_OFF)
		voice->envelope = 0.0f;

	if(!keep_pitch)
		voice->pitch_semitones = key;

	voice->target_semitones = key;
	voice->phase = 0.0;
	voice->elapsed_seconds = 0.0;
	voice->vibrato_phase = 0.0;
	voice->shape_mod_phase = 0.0;

	voice->pcm = pcm_bank_get(atomic_load_explicit(&synth->pcm_bank, memory_order_acquire), synth->params.pcm_sample);
	voice->pcm_position = 0.0;
	voice->pcm_done = false;

	voice->lfsr = LFSR_SEED;
	voice->noise_phase = 0.0;
	voice->noise_value = 1.0f;

	voice->stage = ENV_ATTACK;
	voice->velocity = velocity;
	voice->note_id = note_id;
	voice->port = port;
	voice->channel = channel;
	voice->key = key;
	voice->age = ++synth->age_counter;
}

static void uct__release_voice(Synth* synth, Voice* voice)
{
	if(!uct__is_held_stage(voice->stage))
		return;

	double frames = synth->release_seconds * synth->sample_rate;
	voice->release_decrement = (float)(fmax(voice->envelope, ENV_MIN_LEVEL) / frames);
	voice->stage = ENV_RELEASE;
}

static void uct__mono_note_on(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key, float velocity)
{
	if(synth->held_count >= SYNTH_MAX_VOICES)
	{
		uct__push_ended_held(synth, &synth->held[0]);
		memmove(&synth->held[0], &synth->held[1], sizeof(HeldNote) * (SYNTH_MAX_VOICES - 1));
		synth->held_count--;
	}

	HeldNote* note = &synth->held[synth->held_count];
	note->note_id = note_id;
	note->port = port;
	note->channel = channel;
	note->key = key;
	note->velocity = velocity;
	synth->held_count++;

	Voice* voice = &synth->voices[MONO_VOICE_INDEX];

	if(uct__is_held_stage(voice->stage))
	{
		voice->target_semitones = key;
		voice->velocity = velocity;
		voice->note_id = note_id;
		voice->port = port;
		voice->channel = channel;
		voice->key = key;
		return;
	}

	if(voice->stage == ENV_RELEASE)
		uct__push_ended(synth, voice);

	bool keep_pitch = voice->stage == ENV_RELEASE && synth->params.glide_ms > 0.0;
	uct__start_voice(synth, voice, note_id, port, channel, key, velocity, keep_pitch);
}

static void uct__mono_remove(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key, bool immediate)
{
	Voice* voice = &synth->voices[MONO_VOICE_INDEX];
	bool top_removed = false;
	int write = 0;

	for(int read = 0; read < synth->held_count; read++)
	{
		HeldNote* note = &synth->held[read];

		if(!uct__note_matches(note->note_id, note->port, note->channel, note->key, note_id, port, channel, key))
		{
			synth->held[write] = *note;
			write++;
			continue;
		}

		if(read == synth->held_count - 1)
			top_removed = true;

		bool reports_later = top_removed && write == 0;
		if(!reports_later)
			uct__push_ended_held(synth, note);
	}

	synth->held_count = write;

	if(!top_removed)
		return;

	if(synth->held_count > 0)
	{
		const HeldNote* next = &synth->held[synth->held_count - 1];
		voice->target_semitones = next->key;
		voice->velocity = next->velocity;
		voice->note_id = next->note_id;
		voice->port = next->port;
		voice->channel = next->channel;
		voice->key = next->key;
		return;
	}

	if(!immediate)
	{
		uct__release_voice(synth, voice);
		return;
	}

	if(voice->stage != ENV_OFF)
		uct__push_ended(synth, voice);

	voice->stage = ENV_OFF;
	voice->envelope = 0.0f;
}

static void uct__release_all(Synth* synth)
{
	for(int i = 0; i < SYNTH_MAX_VOICES; i++)
	{
		uct__release_voice(synth, &synth->voices[i]);
	}

	for(int i = 0; i < synth->held_count - 1; i++)
	{
		uct__push_ended_held(synth, &synth->held[i]);
	}

	synth->held_count = 0;
}

static bool uct__advance_envelope(Synth* synth, Voice* voice)
{
	switch(voice->stage)
	{
		case ENV_ATTACK:
			voice->envelope += synth->attack_increment;

			if(voice->envelope >= 1.0f)
			{
				voice->envelope = 1.0f;
				voice->stage = ENV_DECAY;
			}

			return true;

		case ENV_DECAY:
			voice->envelope -= synth->decay_decrement;
			if(voice->envelope <= (float)synth->params.sustain)
			{
				voice->envelope = (float)synth->params.sustain;
				voice->stage = ENV_SUSTAIN;
			}

			return true;

		case ENV_SUSTAIN:
			voice->envelope += ((float)synth->params.sustain - voice->envelope) * synth->gain_coefficient;
			return true;

		case ENV_RELEASE:
			break;

		case ENV_OFF:
			return false;
	}

	voice->envelope -= voice->release_decrement;

	if(voice->envelope > 0.0f)
		return true;

	voice->envelope = 0.0f;
	voice->stage = ENV_OFF;
	uct__push_ended(synth, voice);
	return false;
}

static double uct__poly_blep(double phase, double increment)
{
	if(phase < increment)
	{
		double t = phase / increment;
		return t + t - t * t - 1.0;
	}

	if(phase > 1.0 - increment)
	{
		double t = (phase - 1.0) / increment;
		return t * t + t + t + 1.0;
	}

	return 0.0;
}

static double uct__pulse(double phase, double increment, double duty)
{
	double value = phase < duty ? 1.0 : -1.0;

	double falling_phase = phase - duty;
	if(falling_phase < 0.0)
		falling_phase += 1.0;

	value += uct__poly_blep(phase, increment);
	value -= uct__poly_blep(falling_phase, increment);
	double offset = 2.0 * duty - 1.0;
	double scale = 1.0 / (1.0 + fabs(offset));
	return (value - offset) * scale;
}

static double uct__warp_phase(double phase, double shape)
{
	if(phase < shape)
		return 0.5 * phase / shape;

	return 0.5 + 0.5 * (phase - shape) / (1.0 - shape);
}

static double uct__saw(double phase, double increment, double shape)
{
	return 2.0 * uct__warp_phase(phase, shape) - 1.0 - uct__poly_blep(phase, increment);
}

static double uct__triangle(double phase, double shape)
{
	phase = uct__warp_phase(phase, shape);
	int step = (int)(phase * TRIANGLE_STEPS);
	if(step >= TRIANGLE_STEPS)
		step = TRIANGLE_STEPS - 1;

	int level = step < TRIANGLE_LEVELS ? step : TRIANGLE_STEPS - 1 - step;
	return level / ((TRIANGLE_LEVELS - 1) * 0.5) - 1.0;
}

static float uct__noise(const SynthParams* params, Voice* voice)
{
	const int tap = params->noise_mode == SYNTH_NOISE_SHORT ? LFSR_TAP_SHORT : LFSR_TAP_LONG;
	int steps = 0;
	voice->noise_phase += voice->noise_increment;

	while(voice->noise_phase >= 1.0 && steps < NOISE_MAX_STEPS_PER_FRAME)
	{
		uint16_t feedback = (uint16_t)((voice->lfsr ^ (voice->lfsr >> tap)) & 1u);
		voice->lfsr = (uint16_t)((voice->lfsr >> 1) | (feedback << (LFSR_BITS - 1)));
		voice->noise_phase -= 1.0;
		steps++;
	}

	if(steps == NOISE_MAX_STEPS_PER_FRAME)
		voice->noise_phase = 0.0;

	const double threshold = voice->duty * (1 << NOISE_DENSITY_BITS);
	const double high = (double)(voice->lfsr & ((1u << NOISE_DENSITY_BITS) - 1u)) < threshold ? 1.0 : -1.0;
	const double offset = 2.0 * voice->duty - 1.0;

	voice->noise_value = (float)((high - offset) / (1.0 + fabs(offset)) * NOISE_LEVEL);
	return voice->noise_value;
}

static float uct__pcm(const SynthParams* params, Voice* voice)
{
	const PcmSample* sample = voice->pcm;
	if(sample == NULL || voice->pcm_done)
		return 0.0f;

	double position = voice->pcm_position;
	if(position >= sample->length)
	{
		if(!params->pcm_loop)
		{
			voice->pcm_done = true;
			return 0.0f;
		}

		position = fmod(position, (double)sample->length);
	}

	const float value = sample->data[(uint32_t)position];
	voice->pcm_position = position + voice->pcm_step;
	return value;
}

static float uct__oscillator(const SynthParams* params, Voice* voice)
{
	const double phase = voice->phase;
	const double increment = voice->phase_increment;
	double value = 0.0;

	switch(params->waveform)
	{
		case SYNTH_WAVE_PULSE:
			value = uct__pulse(phase, increment, voice->duty);
			break;

		case SYNTH_WAVE_TRIANGLE:
			value = uct__triangle(phase, voice->duty);
			break;

		case SYNTH_WAVE_SAW:
			value = uct__saw(phase, increment, voice->duty);
			break;

		case SYNTH_WAVE_NOISE:
			return uct__noise(params, voice);

		case SYNTH_WAVE_PCM:
			return uct__pcm(params, voice);
	}

	voice->phase += increment;
	if(voice->phase >= 1.0)
		voice->phase -= 1.0;

	return (float)value;
}

static void uct__update_control(Synth* synth, Voice* voice, uint32_t step_frames)
{
	const SynthParams* params = &synth->params;
	const double step_seconds = step_frames / synth->sample_rate;
	if(voice->pitch_semitones != voice->target_semitones)
	{
		double difference = voice->target_semitones - voice->pitch_semitones;
		double glide_seconds = params->glide_ms * 0.001;
		if(glide_seconds <= 0.0 || fabs(difference) < GLIDE_SNAP_SEMITONES)
			voice->pitch_semitones = voice->target_semitones;
		else
		{
			double time_constant = glide_seconds / GLIDE_TIME_CONSTANTS;
			voice->pitch_semitones += difference * (1.0 - exp(-step_seconds / time_constant));
		}
	}

	double vibrato_semitones = 0.0;
	double vibrato_cents = params->vibrato_cents + synth->mod_wheel * MOD_WHEEL_VIBRATO_CENTS;
	if(vibrato_cents > 0.0)
	{
		double since_delay = voice->elapsed_seconds - params->vibrato_delay_ms * 0.001;
		double fade = uct__clamp(since_delay / VIBRATO_FADE_SECONDS, 0.0, 1.0);
		vibrato_semitones = sin(SYNTH_TWO_PI * voice->vibrato_phase) * vibrato_cents / CENTS_PER_SEMITONE * fade;
	}

	double sweep_semitones = 0.0;
	double sweep_seconds = params->sweep_ms * 0.001;

	if(sweep_seconds > 0.0 && voice->elapsed_seconds < sweep_seconds)
		sweep_semitones = params->sweep_semitones * (1.0 - voice->elapsed_seconds / sweep_seconds);

	double arp_semitones = 0.0;
	const ArpPattern* pattern = &k_arp_patterns[params->arp_mode];
	if(pattern->count > 1)
	{
		uint64_t arp_step = (uint64_t)(voice->elapsed_seconds * params->arp_rate);
		arp_semitones = pattern->offsets[arp_step % (uint64_t)pattern->count];
	}

	double semitones = voice->pitch_semitones + vibrato_semitones + sweep_semitones + arp_semitones + synth->bend_semitones;
	double frequency = NOTE_A4_FREQUENCY * pow(2.0, (semitones - NOTE_A4_KEY) / NOTE_SEMITONES_PER_OCTAVE);
	frequency = uct__clamp(frequency, MIN_FREQUENCY, synth->sample_rate * MAX_FREQUENCY_RATIO);

	voice->phase_increment = frequency / synth->sample_rate;
	voice->noise_increment = frequency * NOISE_CLOCK_MULTIPLIER / synth->sample_rate;

	if(voice->pcm != NULL)
	{
		const double modulation = vibrato_semitones + sweep_semitones + arp_semitones + synth->bend_semitones;
		const double semitones_from_root = params->pcm_keys ? semitones - voice->pcm->root_key : modulation;
		const double speed = pow(2.0, (semitones_from_root + params->pcm_tune) / NOTE_SEMITONES_PER_OCTAVE);
		voice->pcm_step = voice->pcm->rate / synth->sample_rate * speed;
	}

	double pwm = sin(SYNTH_TWO_PI * voice->shape_mod_phase) * params->shape_mod_depth;
	voice->duty = uct__clamp(params->shape + pwm, SHAPE_MIN, SHAPE_MAX);

	voice->vibrato_phase = uct__wrap_phase(voice->vibrato_phase + params->vibrato_rate * step_seconds);
	voice->shape_mod_phase = uct__wrap_phase(voice->shape_mod_phase + params->shape_mod_rate * step_seconds);
	voice->elapsed_seconds += step_seconds;
}

// Returns false if the voice goes silent.
static bool uct__render_voice_step(Synth* synth, Voice* voice, float* mix, uint32_t frame_count)
{
	const float level = voice->velocity * VOICE_LEVEL;

	for(uint32_t i = 0; i < frame_count; i++)
	{
		if(!uct__advance_envelope(synth, voice))
			return false;

		mix[i] += uct__oscillator(&synth->params, voice) * voice->envelope * level;
		if(voice->pcm_done && uct__is_held_stage(voice->stage))
			uct__release_voice(synth, voice);
	}

	return true;
}

static void uct__render_voice(Synth* synth, Voice* voice, float* mix, uint32_t frame_count)
{
	uint32_t done = 0;

	while(done < frame_count)
	{
		uint32_t step = frame_count - done;
		if(step > CONTROL_BLOCK_FRAMES)
			step = CONTROL_BLOCK_FRAMES;

		uct__update_control(synth, voice, step);
		if(!uct__render_voice_step(synth, voice, mix + done, step))
			return;

		done += step;
	}
}

static float uct__lofi(Synth* synth, float sample)
{
	const SynthParams* params = &synth->params;

	if(params->downsample > 1)
	{
		if(synth->hold_counter == 0)
			synth->held_sample = sample;

		synth->hold_counter = (synth->hold_counter + 1) % params->downsample;
		sample = synth->held_sample;
	}

	if(params->bits >= BITS_FULL)
		return sample;

	const float levels = (float)(1u << (params->bits - 1));
	return roundf(sample / VOICE_LEVEL * levels) / levels * VOICE_LEVEL;
}

static float uct__soft_clip(float sample)
{
	if(sample > SOFT_CLIP_LIMIT)
		sample = SOFT_CLIP_LIMIT;

	if(sample < -SOFT_CLIP_LIMIT)
		sample = -SOFT_CLIP_LIMIT;

	return sample - sample * sample * sample * SOFT_CLIP_COEFFICIENT;
}

static void uct__render_test_tone(Synth* synth, float* left, float* right, uint32_t frame_count)
{
	const double phase_step = SYNTH_TWO_PI * NOTE_A4_FREQUENCY / synth->sample_rate;
	uint32_t fade_frames = (uint32_t)(TEST_TONE_FADE_SECONDS * synth->sample_rate);
	if(fade_frames == 0)
		fade_frames = 1;

	for(uint32_t i = 0; i < frame_count; i++)
	{
		if(synth->test_tone_frames_left == 0)
			break;

		const uint32_t elapsed = synth->test_tone_frames_total - synth->test_tone_frames_left;
		float fade = 1.0f;

		if(elapsed < fade_frames)
			fade = (float)elapsed / (float)fade_frames;

		if(synth->test_tone_frames_left < fade_frames)
			fade = (float)synth->test_tone_frames_left / (float)fade_frames;

		float sample = (float)sin(synth->test_tone_phase) * fade * TEST_TONE_LEVEL;
		left[i] += sample;
		right[i] += sample;

		synth->test_tone_phase += phase_step;
		if(synth->test_tone_phase >= SYNTH_TWO_PI)
			synth->test_tone_phase -= SYNTH_TWO_PI;

		synth->test_tone_frames_left--;
	}
}

static void uct__publish_activity(Synth* synth)
{
	uint32_t voice_count = 0;
	uint64_t keys[2] = { 0, 0 };

	for(int i = 0; i < SYNTH_MAX_VOICES; i++)
	{
		const Voice* voice = &synth->voices[i];

		if(voice->stage == ENV_OFF)
			continue;

		voice_count++;

		if(voice->key < 0 || voice->key >= KEY_COUNT)
			continue;

		keys[voice->key / KEYS_PER_MASK] |= (uint64_t)1 << (voice->key % KEYS_PER_MASK);
	}

	atomic_store_explicit(&synth->active_voices, voice_count, memory_order_relaxed);
	atomic_store_explicit(&synth->active_keys[0], keys[0], memory_order_relaxed);
	atomic_store_explicit(&synth->active_keys[1], keys[1], memory_order_relaxed);
}

static void uct__render_chunk(Synth* synth, float* left, float* right, uint32_t frame_count)
{
	float* mix = synth->scratch;
	memset(mix, 0, sizeof(float) * frame_count);

	for(int i = 0; i < SYNTH_MAX_VOICES; i++)
	{
		Voice* voice = &synth->voices[i];

		if(voice->stage == ENV_OFF)
			continue;

		uct__render_voice(synth, voice, mix, frame_count);
	}

	uint32_t scope_position = atomic_load_explicit(&synth->scope_position, memory_order_relaxed);

	for(uint32_t i = 0; i < frame_count; i++)
	{
		synth->gain_current += (synth->gain_target - synth->gain_current) * synth->gain_coefficient;
		float sample = uct__soft_clip(uct__lofi(synth, mix[i]) * synth->gain_current);
		left[i] = sample;
		right[i] = sample;
	}

	uct__render_test_tone(synth, left, right, frame_count);

	for(uint32_t i = 0; i < frame_count; i++)
	{
		atomic_store_explicit(&synth->scope[scope_position % SYNTH_SCOPE_SIZE], left[i], memory_order_relaxed);
		scope_position++;
	}

	atomic_store_explicit(&synth->scope_position, scope_position, memory_order_release);
	uct__publish_activity(synth);
}

#pragma endregion

#pragma region Public API

Synth* synth_create(void)
{
	Synth* synth = (Synth*)calloc(1, sizeof(Synth));
	if(synth == NULL)
		return NULL;

	synth->sample_rate = DEFAULT_SAMPLE_RATE;
	synth->gain_target = INITIAL_GAIN;
	synth->gain_current = INITIAL_GAIN;

	synth->params.gain_db = -12.0;
	synth->params.waveform = SYNTH_WAVE_PULSE;
	synth->params.shape = 0.5;
	synth->params.sustain = 1.0;
	synth->params.release_ms = 80.0;
	synth->params.bits = BITS_FULL;
	synth->params.downsample = 1;
	synth->params.vibrato_rate = 6.0;
	synth->params.arp_rate = 20.0;
	synth->params.shape_mod_rate = 3.0;
	uct__update_rates(synth);
	return synth;
}

void synth_destroy(Synth* synth)
{
	if(synth == NULL)
		return;

	free(synth->scratch);
	free(synth);
}

bool synth_activate(Synth* synth, double sample_rate, uint32_t max_frames)
{
	if(max_frames == 0)
		max_frames = DEFAULT_MAX_FRAMES;

	float* scratch = (float*)malloc(sizeof(float) * max_frames);
	if(scratch == NULL)
		return false;

	free(synth->scratch);
	synth->scratch = scratch;
	synth->max_frames = max_frames;
	synth->sample_rate = sample_rate;

	uct__update_rates(synth);
	synth->gain_current = synth->gain_target;
	synth_reset(synth);
	return true;
}

void synth_deactivate(Synth* synth)
{
	synth_reset(synth);

	free(synth->scratch);
	synth->scratch = NULL;
	synth->max_frames = 0;
}

void synth_reset(Synth* synth)
{
	for(int i = 0; i < SYNTH_MAX_VOICES; i++)
	{
		synth->voices[i].stage = ENV_OFF;
		synth->voices[i].envelope = 0.0f;
	}

	synth->held_count = 0;
	synth->has_last_note = false;
	synth->bend_semitones = 0.0;
	synth->mod_wheel = 0.0;
	synth->held_sample = 0.0f;
	synth->hold_counter = 0;
	synth->ended_count = 0;
	synth->test_tone_frames_left = 0;
	uct__publish_activity(synth);
}

void synth_set_params(Synth* synth, const SynthParams* params)
{
	const int previous_voice_mode = synth->params.voice_mode;

	synth->params = *params;
	uct__sanitize_params(&synth->params);

	synth->gain_target = synth->params.gain_db <= SYNTH_GAIN_MIN_DB ? 0.0f : (float)pow(10.0, synth->params.gain_db / 20.0);
	uct__update_rates(synth);

	if(previous_voice_mode != synth->params.voice_mode)
		uct__release_all(synth);
}

void synth_set_pcm_bank(Synth* synth, const PcmBank* bank)
{
	atomic_store_explicit(&synth->pcm_bank, bank, memory_order_release);
}

void synth_set_pitch_bend(Synth* synth, double semitones)
{
	synth->bend_semitones = semitones;
}

void synth_set_mod_wheel(Synth* synth, double amount)
{
	synth->mod_wheel = uct__clamp(amount, 0.0, 1.0);
}

void synth_note_on(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key, double velocity)
{
	if(synth->scratch == NULL)
		return;

	const float clamped_velocity = uct__clamp_velocity(velocity);

	if(synth->params.voice_mode == SYNTH_VOICE_MONO)
	{
		uct__mono_note_on(synth, note_id, port, channel, key, clamped_velocity);
		return;
	}

	Voice* voice = uct__find_voice_for_note_on(synth);
	uct__start_voice(synth, voice, note_id, port, channel, key, clamped_velocity, false);
	if(synth->params.glide_ms > 0.0 && synth->has_last_note)
		voice->pitch_semitones = synth->last_semitones;

	synth->last_semitones = key;
	synth->has_last_note = true;
}

void synth_note_off(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key)
{
	if(synth->params.voice_mode == SYNTH_VOICE_MONO)
	{
		uct__mono_remove(synth, note_id, port, channel, key, false);
		return;
	}

	for(int i = 0; i < SYNTH_MAX_VOICES; i++)
	{
		Voice* voice = &synth->voices[i];

		if(!uct__is_held_stage(voice->stage))
			continue;

		if(!uct__voice_matches(voice, note_id, port, channel, key))
			continue;

		uct__release_voice(synth, voice);
	}
}

void synth_note_choke(Synth* synth, int32_t note_id, int16_t port, int16_t channel, int16_t key)
{
	if(synth->params.voice_mode == SYNTH_VOICE_MONO)
	{
		uct__mono_remove(synth, note_id, port, channel, key, true);
		return;
	}

	for(int i = 0; i < SYNTH_MAX_VOICES; i++)
	{
		Voice* voice = &synth->voices[i];

		if(voice->stage == ENV_OFF)
			continue;

		if(!uct__voice_matches(voice, note_id, port, channel, key))
			continue;

		voice->stage = ENV_OFF;
		voice->envelope = 0.0f;
		uct__push_ended(synth, voice);
	}
}

void synth_all_notes_off(Synth* synth)
{
	synth_note_off(synth, SYNTH_UNSPECIFIED, SYNTH_UNSPECIFIED, SYNTH_UNSPECIFIED, SYNTH_UNSPECIFIED);
}

void synth_start_test_tone(Synth* synth)
{
	synth->test_tone_phase = 0.0;
	synth->test_tone_frames_total = (uint32_t)(TEST_TONE_SECONDS * synth->sample_rate);
	synth->test_tone_frames_left = synth->test_tone_frames_total;
}

void synth_render(Synth* synth, float* left, float* right, uint32_t frame_count)
{
	memset(left, 0, sizeof(float) * frame_count);
	memset(right, 0, sizeof(float) * frame_count);

	if(synth->scratch == NULL)
		return;

	uint32_t done = 0;
	while(done < frame_count)
	{
		uint32_t chunk = frame_count - done;
		if(chunk > synth->max_frames)
			chunk = synth->max_frames;

		uct__render_chunk(synth, left + done, right + done, chunk);
		done += chunk;
	}
}

bool synth_pop_ended(Synth* synth, SynthEndedNote* out)
{
	if(synth->ended_count == 0)
		return false;

	synth->ended_count--;
	*out = synth->ended[synth->ended_count];
	return true;
}

uint32_t synth_scope_copy(const Synth* synth, float* out, uint32_t count)
{
	if(count > SYNTH_SCOPE_SIZE)
		count = SYNTH_SCOPE_SIZE;

	const uint32_t end = atomic_load_explicit((_Atomic uint32_t*)&synth->scope_position, memory_order_acquire);

	for(uint32_t i = 0; i < count; i++)
	{
		uint32_t index = (end - count + i) % SYNTH_SCOPE_SIZE;
		out[i] = atomic_load_explicit((_Atomic float*)&synth->scope[index], memory_order_relaxed);
	}

	return count;
}

uint32_t synth_active_voice_count(const Synth* synth)
{
	return atomic_load_explicit((_Atomic uint32_t*)&synth->active_voices, memory_order_relaxed);
}

void synth_active_keys(const Synth* synth, uint64_t* low, uint64_t* high)
{
	*low = atomic_load_explicit((_Atomic uint64_t*)&synth->active_keys[0], memory_order_relaxed);
	*high = atomic_load_explicit((_Atomic uint64_t*)&synth->active_keys[1], memory_order_relaxed);
}

#pragma endregion
