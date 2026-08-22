#define NAUI_PI 3.1415926535897931f
#define NAUI_SQRT2 1.41421353816986083984375f
#define NAUI_EPSILON 1e-9
#define NAUI_EPSILON_F 1e-6f 

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
