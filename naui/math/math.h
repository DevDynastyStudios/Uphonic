static inline float naui_lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

static inline float naui_clamp(float x, float min, float max)
{
	return fmaxf(min, fminf(x, max));
}

static inline float naui_clamp01(float x)
{
	return fmaxf(0.0f, fminf(x, 1.0f));
}

static inline float naui_wrap(float x, float min, float max)
{
	float range = max - min;
	return x - range * floorf((x - min) / range);
}
