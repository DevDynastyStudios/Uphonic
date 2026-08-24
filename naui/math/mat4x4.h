typedef struct
{
	float m[4][4];
}
Naui_Mat4x4;

static inline Naui_Mat4x4 naui_mat4x4_identity(void)
{
	Naui_Mat4x4 r = {0};
	r.m[0][0] = 1.0f;
	r.m[1][1] = 1.0f;
	r.m[2][2] = 1.0f;
	r.m[3][3] = 1.0f;
	return r;
}

static inline Naui_Mat4x4 naui_mat4x4_translate(float x, float y, float z)
{
	Naui_Mat4x4 r = naui_mat4x4_identity();
	r.m[3][0] = x;
	r.m[3][1] = y;
	r.m[3][2] = z;
	return r;
}

static inline Naui_Mat4x4 naui_mat4x4_scale(float x, float y, float z)
{
	Naui_Mat4x4 r = naui_mat4x4_identity();
	r.m[0][0] = x;
	r.m[1][1] = y;
	r.m[2][2] = z;
	return r;
}

static inline Naui_Mat4x4 naui_mat4x4_rotate_z(float angle)
{
	Naui_Mat4x4 r = naui_mat4x4_identity();
	float c = cosf(angle);
	float s = sinf(angle);
	r.m[0][0] =  c; r.m[1][0] = -s;
	r.m[0][1] =  s; r.m[1][1] =  c;
	return r;
}

static inline Naui_Mat4x4 naui_mat4x4_mul(const Naui_Mat4x4 a, const Naui_Mat4x4 b)
{
	Naui_Mat4x4 r;
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			r.m[col][row] =
				a.m[0][row] * b.m[col][0] +
				a.m[1][row] * b.m[col][1] +
				a.m[2][row] * b.m[col][2] +
				a.m[3][row] * b.m[col][3];
		}
	}
	return r;
}

static inline Naui_Vec4 naui_mat4x4_mul_vec4(const Naui_Mat4x4 m, const Naui_Vec4 v)
{
	return (Naui_Vec4){
		m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w,
		m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w,
		m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w,
		m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w,
	};
}

static inline Naui_Vec2 naui_mat4x4_transform_point2(const Naui_Mat4x4 m, const Naui_Vec2 p)
{
	Naui_Vec4 v = naui_mat4x4_mul_vec4(m, (Naui_Vec4){ p.x, p.y, 0.0f, 1.0f });
	return (Naui_Vec2){ v.x, v.y };
}

static inline Naui_Mat4x4 naui_mat4x4_rotate_around(Naui_Vec2 pivot_offset, float angle, Naui_Vec2 position)
{
	Naui_Mat4x4 to_origin   = naui_mat4x4_translate(-pivot_offset.x, -pivot_offset.y, 0.0f);
	Naui_Mat4x4 rot         = naui_mat4x4_rotate_z(angle);
	Naui_Mat4x4 from_origin = naui_mat4x4_translate(pivot_offset.x, pivot_offset.y, 0.0f);
	Naui_Mat4x4 to_position = naui_mat4x4_translate(position.x, position.y, 0.0f);

	Naui_Mat4x4 r = naui_mat4x4_mul(rot, to_origin);
	r = naui_mat4x4_mul(from_origin, r);
	r = naui_mat4x4_mul(to_position, r);
	return r;
}