#define UPH_METRONOME_TIME_RESET 2.0f
#define UPH_METRONOME_MAX_TAPS 8
#define UPH_METRONOME_MIN_INTERVAL (60.0f / 300.0f)
#define UPH_METRONOME_OUTLIER_LOW 0.5f
#define UPH_METRONOME_OUTLIER_HIGH 2.0f

typedef struct
{
	bool active;
	float last_tap_time;
	float avg_interval;
	uint32_t count;
}
Uph_MetronomeState;

static Uph_MetronomeState _uph_metronome_state;

static bool _uph_metronome_tap(float *out_bpm)
{
	Uph_MetronomeState *state = &_uph_metronome_state;
	const float current_time = naui_time();

	if (!state->active)
	{
		state->active = true;
		state->last_tap_time = current_time;
		state->count = 0;
		state->avg_interval = 0.0f;
		return false;
	}

	const float since_last = current_time - state->last_tap_time;

	if (since_last > UPH_METRONOME_TIME_RESET)
	{
		state->last_tap_time = current_time;
		state->count = 0;
		state->avg_interval = 0.0f;
		return false;
	}

	if (since_last < UPH_METRONOME_MIN_INTERVAL)
		return false;

	if (state->avg_interval > 0.0f)
	{
		const float ratio = since_last / state->avg_interval;
		if (ratio < UPH_METRONOME_OUTLIER_LOW || ratio > UPH_METRONOME_OUTLIER_HIGH)
		{
			state->last_tap_time = current_time;
			return false;
		}
	}

	const uint32_t n = (state->count < UPH_METRONOME_MAX_TAPS) ? (state->count + 1) : UPH_METRONOME_MAX_TAPS;
	state->avg_interval += (since_last - state->avg_interval) / (float)n;
	state->count = n;
	state->last_tap_time = current_time;
	*out_bpm = 60.0f / state->avg_interval;
	return true;
}
