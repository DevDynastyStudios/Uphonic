static inline float naui_ease_in_sine(float x)
{
    return 1.0f - cosf((x * NAUI_PI) / 2.0f);
}

static inline float naui_ease_out_sine(float x)
{
    return sinf((x * NAUI_PI) / 2.0f);
}

static inline float naui_ease_in_out_sine(float x)
{
    return -(cosf(NAUI_PI * x) - 1.0f) / 2.0f;
}

static inline float naui_ease_in_quad(float x)
{
    return x * x;
}

static inline float naui_ease_out_quad(float x)
{
    float t = 1.0f - x;
    return 1.0f - t * t;
}

static inline float naui_ease_in_out_quad(float x)
{
    if (x < 0.5f) return 2.0f * x * x;
    float t = -2.0f * x + 2.0f;
    return 1.0f - (t * t) / 2.0f;
}

static inline float naui_ease_in_cubic(float x)
{
    return x * x * x;
}

static inline float naui_ease_out_cubic(float x)
{
    float t = 1.0f - x;
    return 1.0f - t * t * t;
}

static inline float naui_ease_in_out_cubic(float x)
{
    if (x < 0.5f) return 4.0f * x * x * x;
    float t = -2.0f * x + 2.0f;
    return 1.0f - (t * t * t) / 2.0f;
}

static inline float naui_ease_in_quart(float x)
{
    float x2 = x * x;
    return x2 * x2;
}

static inline float naui_ease_out_quart(float x)
{
    float t = 1.0f - x;
    float t2 = t * t;
    return 1.0f - t2 * t2;
}

static inline float naui_ease_in_out_quart(float x)
{
    if (x < 0.5f) {
        float x2 = x * x;
        return 8.0f * x2 * x2;
    }
    float t = -2.0f * x + 2.0f;
    float t2 = t * t;
    return 1.0f - (t2 * t2) / 2.0f;
}

static inline float naui_ease_in_quint(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

static inline float naui_ease_out_quint(float x)
{
    float t = 1.0f - x;
    float t2 = t * t;
    return 1.0f - t2 * t2 * t;
}

static inline float naui_ease_in_out_quint(float x)
{
    if (x < 0.5f) {
        float x2 = x * x;
        return 16.0f * x2 * x2 * x;
    }
    float t = -2.0f * x + 2.0f;
    float t2 = t * t;
    return 1.0f - (t2 * t2 * t) / 2.0f;
}

static inline float naui_ease_in_expo(float x)
{
    return x == 0.0f ? 0.0f : exp2f(10.0f * x - 10.0f);
}

static inline float naui_ease_out_expo(float x)
{
    return x == 1.0f ? 1.0f : 1.0f - exp2f(-10.0f * x);
}

static inline float naui_ease_in_out_expo(float x)
{
    if (x == 0.0f) return 0.0f;
    if (x == 1.0f) return 1.0f;
    return x < 0.5f
        ? exp2f(20.0f * x - 10.0f) / 2.0f
        : (2.0f - exp2f(-20.0f * x + 10.0f)) / 2.0f;
}

static inline float naui_ease_in_circ(float x)
{
    return 1.0f - sqrtf(1.0f - x * x);
}

static inline float naui_ease_out_circ(float x)
{
    float t = x - 1.0f;
    return sqrtf(1.0f - t * t);
}

static inline float naui_ease_in_out_circ(float x)
{
    if (x < 0.5f) {
        float t = 2.0f * x;
        return (1.0f - sqrtf(1.0f - t * t)) / 2.0f;
    }
    float t = -2.0f * x + 2.0f;
    return (sqrtf(1.0f - t * t) + 1.0f) / 2.0f;
}

static inline float naui_ease_in_back(float x)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return c3 * x * x * x - c1 * x * x;
}

static inline float naui_ease_out_back(float x)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float t = x - 1.0f;
    float t2 = t * t;
    return 1.0f + c3 * (t2 * t) + c1 * t2;
}

static inline float naui_ease_in_out_back(float x)
{
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;

    if (x < 0.5f) {
        float t = 2.0f * x;
        return (t * t * ((c2 + 1.0f) * t - c2)) / 2.0f;
    }
    float t = 2.0f * x - 2.0f;
    return (t * t * ((c2 + 1.0f) * t + c2) + 2.0f) / 2.0f;
}

static inline float naui_ease_in_elastic(float x)
{
    const float c4 = (2.0f * NAUI_PI) / 3.0f;

    if (x == 0.0f) return 0.0f;
    if (x == 1.0f) return 1.0f;
    return -exp2f(10.0f * x - 10.0f) * sinf((x * 10.0f - 10.75f) * c4);
}

static inline float naui_ease_out_elastic(float x)
{
    const float c4 = (2.0f * NAUI_PI) / 3.0f;

    if (x == 0.0f) return 0.0f;
    if (x == 1.0f) return 1.0f;
    return exp2f(-10.0f * x) * sinf((x * 10.0f - 0.75f) * c4) + 1.0f;
}

static inline float naui_ease_in_out_elastic(float x)
{
    const float c5 = (2.0f * NAUI_PI) / 4.5f;

    if (x == 0.0f) return 0.0f;
    if (x == 1.0f) return 1.0f;

    float s = sinf((20.0f * x - 11.125f) * c5);
    return x < 0.5f
        ? -(exp2f(20.0f * x - 10.0f) * s) / 2.0f
        :  (exp2f(-20.0f * x + 10.0f) * s) / 2.0f + 1.0f;
}

static inline float naui_ease_out_bounce(float x)
{
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (x < 1.0f / d1) {
        return n1 * x * x;
    } else if (x < 2.0f / d1) {
        x -= 1.5f / d1;
        return n1 * x * x + 0.75f;
    } else if (x < 2.5f / d1) {
        x -= 2.25f / d1;
        return n1 * x * x + 0.9375f;
    } else {
        x -= 2.625f / d1;
        return n1 * x * x + 0.984375f;
    }
}

static inline float naui_ease_in_bounce(float x)
{
    return 1.0f - naui_ease_out_bounce(1.0f - x);
}

static inline float ease_in_out_bounce(float x)
{
    return x < 0.5f
        ? (1.0f - naui_ease_out_bounce(1.0f - 2.0f * x)) / 2.0f
        : (1.0f + naui_ease_out_bounce(2.0f * x - 1.0f)) / 2.0f;
}