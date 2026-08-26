#define NAUI_PI 3.1415926535897931f
#define NAUI_SQRT2 1.41421353816986083984375f
#define NAUI_EPSILON 1e-9
#define NAUI_EPSILON_F 1e-6f

#define NAUI_MIN(a, b) ((a) < (b) ? (a) : (b))
#define NAUI_MAX(a, b) ((a) > (b) ? (a) : (b))

#define NAUI_LERP(a, b, t) ((a) + (t) * ((b) - (a)))

#define NAUI_CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define NAUI_CLAMP01(x) ((x) < 0.0f ? 0.0f : ((x) > 1.0f ? 1.0f : (x)))

#define NAUI_WRAP(x, lo, hi) ((x) - ((hi) - (lo)) * floorf(((x) - (lo)) / ((hi) - (lo))))