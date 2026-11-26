#define PCM_BUILTIN_RATE 11025
#define PCM_BUILTIN_BITS 8
#define PCM_WAVE_CYCLE_LENGTH 32
#define PCM_WAVE_BITS 4
#define PCM_WAVE_RATE 8372 // 32 steps * 261.625 Hz (C4)
#define PCM_PI 3.14159265358979323846

typedef struct PcmBuilder
{
	float* data;
	uint32_t length;
	uint32_t rate;
	uint32_t noise_state;
	float previous_noise;
} PcmBuilder;

#pragma region Private API

static float uct__builder_noise(PcmBuilder* builder)
{
	uint32_t x = builder->noise_state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	builder->noise_state = x;

	return (float)(x & 0xFFFF) / 32768.0f - 1.0f;
}

static float uct__quantize(float value, int bits)
{
	const float levels = (float)((1 << (bits - 1)) - 1);
	if(value > 1.0f)
		value = 1.0f;

	if(value < -1.0f)
		value = -1.0f;

	return roundf(value * levels) / levels;
}

static bool uct__builder_begin(PcmBuilder* builder, double seconds, uint32_t rate)
{
	builder->rate = rate;
	builder->length = (uint32_t)(seconds * rate);
	builder->noise_state = 0x1234ABCDu;
	builder->previous_noise = 0.0f;
	builder->data = (float*)calloc(builder->length, sizeof(float));
	return builder->data != NULL;
}

static void uct__add_sample(PcmBank* bank, const char* name, PcmBuilder* builder, int bits, int root_key)
{
	if(builder->data == NULL || bank->count >= PCM_MAX_SAMPLES)
	{
		free(builder->data);
		return;
	}

	for(uint32_t i = 0; i < builder->length; i++)
	{
		builder->data[i] = uct__quantize(builder->data[i], bits);
	}

	PcmSample* sample = &bank->samples[bank->count];
	snprintf(sample->name, sizeof(sample->name), "%s", name);
	sample->data = builder->data;
	sample->length = builder->length;
	sample->rate = builder->rate;
	sample->root_key = root_key;
	bank->count++;
}

static void uct__build_kick(PcmBank* bank)
{
	PcmBuilder b;
	if(!uct__builder_begin(&b, 0.30, PCM_BUILTIN_RATE))
		return;

	double phase = 0.0;

	for(uint32_t i = 0; i < b.length; i++)
	{
		const double t = (double)i / b.rate;
		phase += 2.0 * PCM_PI * (45.0 + 115.0 * exp(-t * 28.0)) / b.rate;
		b.data[i] = (float)(sin(phase) * exp(-t * 9.0));
	}

	uct__add_sample(bank, "KICK", &b, PCM_BUILTIN_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_snare(PcmBank* bank)
{
	PcmBuilder b;
	if(!uct__builder_begin(&b, 0.22, PCM_BUILTIN_RATE))
		return;

	for(uint32_t i = 0; i < b.length; i++)
	{
		const double t = (double)i / b.rate;
		const double tone = sin(2.0 * PCM_PI * 185.0 * t) * exp(-t * 30.0) * 0.6;
		b.data[i] = (float)(uct__builder_noise(&b) * exp(-t * 22.0) * 0.7 + tone);
	}

	uct__add_sample(bank, "SNARE", &b, PCM_BUILTIN_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_hat(PcmBank* bank)
{
	PcmBuilder b;
	if(!uct__builder_begin(&b, 0.08, PCM_BUILTIN_RATE))
		return;

	for(uint32_t i = 0; i < b.length; i++)
	{
		const double t = (double)i / b.rate;
		const float noise = uct__builder_noise(&b);
		b.data[i] = (float)((noise - b.previous_noise) * 0.5 * exp(-t * 60.0));
		b.previous_noise = noise;
	}

	uct__add_sample(bank, "HAT", &b, PCM_BUILTIN_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_clap(PcmBank* bank)
{
	PcmBuilder b;
	if(!uct__builder_begin(&b, 0.25, PCM_BUILTIN_RATE))
		return;

	for(uint32_t i = 0; i < b.length; i++)
	{
		const double t = (double)i / b.rate;
		const double burst = fmod(t, 0.012) < 0.008 && t < 0.036 ? exp(-fmod(t, 0.012) * 90.0) : 0.0;
		const double tail = t >= 0.036 ? exp(-(t - 0.036) * 22.0) * 0.8 : 0.0;
		b.data[i] = (float)(uct__builder_noise(&b) * (burst + tail) * 0.8);
	}

	uct__add_sample(bank, "CLAP", &b, PCM_BUILTIN_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_tom(PcmBank* bank)
{
	PcmBuilder b;
	if(!uct__builder_begin(&b, 0.30, PCM_BUILTIN_RATE))
		return;

	double phase = 0.0;

	for(uint32_t i = 0; i < b.length; i++)
	{
		const double t = (double)i / b.rate;
		phase += 2.0 * PCM_PI * (95.0 + 95.0 * exp(-t * 14.0)) / b.rate;
		b.data[i] = (float)(sin(phase) * exp(-t * 7.0));
	}

	uct__add_sample(bank, "TOM", &b, PCM_BUILTIN_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_zap(PcmBank* bank)
{
	PcmBuilder b;
	if(!uct__builder_begin(&b, 0.25, PCM_BUILTIN_RATE))
		return;

	double phase = 0.0;

	for(uint32_t i = 0; i < b.length; i++)
	{
		const double t = (double)i / b.rate;
		phase += 2.0 * PCM_PI * (120.0 + 1700.0 * exp(-t * 16.0)) / b.rate;
		b.data[i] = (float)(sin(phase) * (1.0 - t / 0.25));
	}

	uct__add_sample(bank, "ZAP", &b, PCM_BUILTIN_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_coin(PcmBank* bank)
{
	PcmBuilder b;
	if(!uct__builder_begin(&b, 0.30, PCM_BUILTIN_RATE))
		return;

	for(uint32_t i = 0; i < b.length; i++)
	{
		const double t = (double)i / b.rate;
		const double frequency = t < 0.07 ? 988.0 : 1319.0;
		const double envelope = t < 0.07 ? 1.0 : exp(-(t - 0.07) * 9.0);
		b.data[i] = (float)((fmod(t * frequency, 1.0) < 0.5 ? 0.8 : -0.8) * envelope);
	}

	uct__add_sample(bank, "COIN", &b, PCM_BUILTIN_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_wave(PcmBank* bank, const char* name, const double* harmonics, int harmonic_count)
{
	PcmBuilder b;
	b.rate = PCM_WAVE_RATE;
	b.length = PCM_WAVE_CYCLE_LENGTH;
	b.data = (float*)calloc(b.length, sizeof(float));

	if(b.data == NULL)
		return;

	for(uint32_t i = 0; i < b.length; i++)
	{
		double value = 0.0;
		double total = 0.0;
		for(int h = 0; h < harmonic_count; h++)
		{
			value += harmonics[h] * sin(2.0 * PCM_PI * (h + 1) * i / b.length);
			total += fabs(harmonics[h]);
		}

		b.data[i] = (float)(value / total);
	}

	uct__add_sample(bank, name, &b, PCM_WAVE_BITS, PCM_DEFAULT_ROOT_KEY);
}

static void uct__build_builtins(PcmBank* bank)
{
	static const double k_sine[] = { 1.0 };
	static const double k_organ[] = { 1.0, 0.5, 0.35, 0.2 };
	uct__build_kick(bank);
	uct__build_snare(bank);
	uct__build_hat(bank);
	uct__build_clap(bank);
	uct__build_tom(bank);
	uct__build_zap(bank);
	uct__build_coin(bank);
	uct__build_wave(bank, "SINE4", k_sine, 1);
	uct__build_wave(bank, "ORGAN4", k_organ, 4);
}

#pragma endregion

#pragma region Public API

PcmBank* pcm_bank_create(void)
{
	PcmBank* bank = (PcmBank*)calloc(1, sizeof(PcmBank));
	if(bank == NULL)
		return NULL;

	uct__build_builtins(bank);
	return bank;
}

void pcm_bank_destroy(PcmBank* bank)
{
	if(bank == NULL)
		return;

	for(int i = 0; i < bank->count; i++)
	{
		free(bank->samples[i].data);
	}

	free(bank);
}

const PcmSample* pcm_bank_get(const PcmBank* bank, int index)
{
	if(bank == NULL || index < 0 || index >= bank->count)
		return NULL;

	return &bank->samples[index];
}

int pcm_bank_find(const PcmBank* bank, const char* name)
{
	if(bank == NULL || name == NULL)
		return -1;

	for(int i = 0; i < bank->count; i++)
	{
		if(strcmp(bank->samples[i].name, name) == 0)
			return i;
	}

	return -1;
}

#pragma endregion
