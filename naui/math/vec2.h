typedef struct
{
	float x, y;
} Naui_Vec2;

static inline Naui_Vec2 naui_vec2(float x, float y)
{
	return (Naui_Vec2){ x, y };
}

static inline Naui_Vec2 naui_vec2_add(const Naui_Vec2 a, const Naui_Vec2 b)
{
	return (Naui_Vec2){ a.x + b.x, a.y + b.y };
}

static inline Naui_Vec2 naui_vec2_sub(const Naui_Vec2 a, const Naui_Vec2 b)
{
	return (Naui_Vec2){ a.x - b.x, a.y - b.y };
}

static inline Naui_Vec2 naui_vec2_scale(const Naui_Vec2 v, float s)
{
	return (Naui_Vec2){ v.x * s, v.y * s };
}

static inline Naui_Vec2 naui_vec2_abs(const Naui_Vec2 v)
{
	return (Naui_Vec2){ fabsf(v.x), fabsf(v.y) };
}

static inline Naui_Vec2 naui_vec2_negate(const Naui_Vec2 v)
{
	return (Naui_Vec2){ -v.x, -v.y };
}

static inline Naui_Vec2 naui_vec2_min(const Naui_Vec2 a, const Naui_Vec2 b)
{
	return (Naui_Vec2){ fminf(a.x, b.x), fminf(a.y, b.y) };
}

static inline Naui_Vec2 naui_vec2_max(const Naui_Vec2 a, const Naui_Vec2 b)
{
	return (Naui_Vec2){ fmaxf(a.x, b.x), fmaxf(a.y, b.y) };
}

static inline Naui_Vec2 naui_vec2_lerp(const Naui_Vec2 a, const Naui_Vec2 b, float t)
{
	return (Naui_Vec2)
	{
		a.x + t * (b.x - a.x),
		a.y + t * (b.y - a.y)
	};
}

static inline float naui_vec2_length(const Naui_Vec2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

static inline float naui_vec2_distance(const Naui_Vec2 a, const Naui_Vec2 b)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	return sqrtf(dx * dx + dy * dy);
}

static inline float naui_vec2_dot(const Naui_Vec2 a, const Naui_Vec2 b)
{
	return a.x * b.x + a.y * b.y;
}
