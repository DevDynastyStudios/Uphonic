typedef struct
{
	float x, y, z, w;
} Naui_Vec4;

static inline Naui_Vec4 naui_vec4_add(const Naui_Vec4 a, const Naui_Vec4 b)
{
	return (Naui_Vec4){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

static inline Naui_Vec4 naui_vec4_sub(const Naui_Vec4 a, const Naui_Vec4 b)
{
	return (Naui_Vec4){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

static inline Naui_Vec4 naui_vec4_scale(const Naui_Vec4 v, float s)
{
	return (Naui_Vec4){ v.x * s, v.y * s, v.z * s, v.w * s };
}

static inline Naui_Vec4 naui_vec4_abs(const Naui_Vec4 v)
{
	return (Naui_Vec4){ fabsf(v.x), fabsf(v.y), fabsf(v.z), fabsf(v.w) };
}

static inline Naui_Vec4 naui_vec4_negate(const Naui_Vec4 v)
{
	return (Naui_Vec4){ -v.x, -v.y, -v.z, -v.w };
}

static inline Naui_Vec4 naui_vec4_min(const Naui_Vec4 a, const Naui_Vec4 b)
{
	return (Naui_Vec4){ fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z), fminf(a.w, b.w) };
}

static inline Naui_Vec4 naui_vec4_max(const Naui_Vec4 a, const Naui_Vec4 b)
{
	return (Naui_Vec4){ fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z), fmaxf(a.w, b.w) };
}

static inline Naui_Vec4 naui_vec4_lerp(const Naui_Vec4 a, const Naui_Vec4 b, float t)
{
	return (Naui_Vec4)
	{
		a.x + t * (b.x - a.x),
		a.y + t * (b.y - a.y),
		a.z + t * (b.z - a.z),
		a.w + t * (b.w - a.w)
	};
}

static inline float naui_vec4_length(const Naui_Vec4 v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

static inline float naui_vec4_distance(const Naui_Vec4 a, const Naui_Vec4 b)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float dz = b.z - a.z;
	float dw = b.w - a.w;
	return sqrtf(dx * dx + dy * dy + dz * dz + dw * dw);
}

static inline float naui_vec4_dot(const Naui_Vec4 a, const Naui_Vec4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
