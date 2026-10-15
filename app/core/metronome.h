#define UPH_METRONOME_TIME_RESET 2.0f
#define UPH_METRONOME_MAX_TAPS 8
#define UPH_METRONOME_MIN_INTERVAL (60.0f / 300.0f)
#define UPH_METRONOME_OUTLIER_LOW 0.5f
#define UPH_METRONOME_OUTLIER_HIGH 2.0f

static bool _metronome_active = false;
static float _metronome_last_tap_time;
static float _metronome_avg_interval;
static uint32_t _metronome_count = 0;

static void _uph_metronome_reset()
{
	_metronome_active = false;
	_metronome_avg_interval = 0.0f;
	_metronome_count = 0;
}

static bool _uph_metronome_tap(float *out_bpm)
{
	const float current_time = naui_time();

	if (!_metronome_active)
	{
		_metronome_active = true;
		_metronome_last_tap_time = current_time;
		_metronome_count = 0;
		_metronome_avg_interval = 0.0f;
		return false;
	}

	const float since_last = current_time - _metronome_last_tap_time;

	if (since_last > UPH_METRONOME_TIME_RESET)
	{
		_metronome_last_tap_time = current_time;
		_metronome_count = 0;
		_metronome_avg_interval = 0.0f;
		return false;
	}

	if (since_last < UPH_METRONOME_MIN_INTERVAL)
		return false;

	if (_metronome_avg_interval > 0.0f)
	{
		const float ratio = since_last / _metronome_avg_interval;
		if (ratio < UPH_METRONOME_OUTLIER_LOW || ratio > UPH_METRONOME_OUTLIER_HIGH)
		{
			_metronome_last_tap_time = current_time;
			return false;
		}
	}

	const uint32_t n = (_metronome_count < UPH_METRONOME_MAX_TAPS) ? (_metronome_count + 1) : UPH_METRONOME_MAX_TAPS;
	_metronome_avg_interval += (since_last - _metronome_avg_interval) / (float)n;
	_metronome_count = n;
	_metronome_last_tap_time = current_time;
	*out_bpm = 60.0f / _metronome_avg_interval;
	return true;
}
