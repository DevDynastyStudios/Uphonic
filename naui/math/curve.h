#define NAUI_CURVE_MAX_POINTS 16

typedef float (*Naui_EaseFn)(float x);

typedef enum
{
	NAUI_CURVE_LINEAR,
	NAUI_CURVE_STEP,
	NAUI_CURVE_EASE,
	NAUI_CURVE_BEZIER
}
Naui_CurveInterp;

typedef struct
{
	float x, y;
	Naui_CurveInterp interp;
	Naui_EaseFn ease;
	float bezier[4];
}
Naui_CurvePoint;

typedef struct
{
	Naui_CurvePoint points[NAUI_CURVE_MAX_POINTS];
	int32_t count;
}
Naui_Curve;

#define NAUI_BEZIER_EASE		0.25f, 0.1f, 0.25f, 1.0f
#define NAUI_BEZIER_EASE_IN		0.42f, 0.0f, 1.0f, 1.0f
#define NAUI_BEZIER_EASE_OUT	0.0f, 0.0f, 0.58f, 1.0f
#define NAUI_BEZIER_EASE_IN_OUT	0.42f, 0.0f, 0.58f, 1.0f

#pragma region Bezier

static inline double _naui_bezier_axis(const double p1, const double p2, const double t)
{
	const double a = 1.0 + 3.0 * p1 - 3.0 * p2;
	const double b = 3.0 * p2 - 6.0 * p1;
	const double c = 3.0 * p1;
	return ((a * t + b) * t + c) * t;
}

static inline double _naui_bezier_axis_slope(const double p1, const double p2, const double t)
{
	const double a = 1.0 + 3.0 * p1 - 3.0 * p2;
	const double b = 3.0 * p2 - 6.0 * p1;
	const double c = 3.0 * p1;
	return (3.0 * a * t + 2.0 * b) * t + c;
}

static inline float naui_cubic_bezier(const float x1_in, const float y1_in, const float x2_in, const float y2_in, const float x_in)
{
	if (!(x_in > 0.0f))
		return 0.0f;

	if (x_in >= 1.0f)
		return 1.0f;

	const double x1 = NAUI_CLAMP01(x1_in);
	const double x2 = NAUI_CLAMP01(x2_in);
	const double y1 = y1_in;
	const double y2 = y2_in;
	const double x = x_in;

	double t = x;
	for (int32_t i = 0; i < 8; i++)
	{
		const double error = _naui_bezier_axis(x1, x2, t) - x;
		const double slope = _naui_bezier_axis_slope(x1, x2, t);

		if (slope < 1e-2)
			break;

		if (fabs(error) < 1e-9)
			return (float)_naui_bezier_axis(y1, y2, t);

		t -= error / slope;
	}

	double low = 0.0;
	double high = 1.0;
	t = x;
	for (int32_t i = 0; i < 48; i++)
	{
		if (_naui_bezier_axis(x1, x2, t) < x)
			low = t;
		else
			high = t;

		t = (low + high) * 0.5;
	}

	return (float)_naui_bezier_axis(y1, y2, t);
}

#pragma endregion

#pragma region Curve

static inline Naui_Curve naui_curve_make(void)
{
	return (Naui_Curve){0};
}

static inline int32_t naui_curve_add_point(Naui_Curve *curve, const float x, const float y, const Naui_CurveInterp interp)
{
	if (!curve || curve->count >= NAUI_CURVE_MAX_POINTS || x != x || y != y)
		return -1;

	int32_t index = curve->count;
	while (index > 0 && curve->points[index - 1].x > x)
	{
		curve->points[index] = curve->points[index - 1];
		index--;
	}

	curve->points[index] = (Naui_CurvePoint){ .x = x, .y = y, .interp = interp };
	curve->count++;
	return index;
}

static inline int32_t naui_curve_add_ease(Naui_Curve *curve, const float x, const float y, const Naui_EaseFn ease)
{
	const int32_t index = naui_curve_add_point(curve, x, y, NAUI_CURVE_EASE);
	if (index >= 0)
		curve->points[index].ease = ease;

	return index;
}

static inline int32_t naui_curve_add_bezier(Naui_Curve *curve, const float x, const float y, const float x1, const float y1, const float x2, const float y2)
{
	const int32_t index = naui_curve_add_point(curve, x, y, NAUI_CURVE_BEZIER);
	if (index >= 0)
	{
		curve->points[index].bezier[0] = NAUI_CLAMP01(x1);
		curve->points[index].bezier[1] = y1;
		curve->points[index].bezier[2] = NAUI_CLAMP01(x2);
		curve->points[index].bezier[3] = y2;
	}

	return index;
}

static inline bool naui_curve_remove_point(Naui_Curve *curve, const int32_t index)
{
	if (!curve || index < 0 || index >= curve->count)
		return false;

	for (int32_t i = index; i < curve->count - 1; i++)
	{
		curve->points[i] = curve->points[i + 1];
	}

	curve->count--;
	return true;
}

static inline Naui_Curve naui_curve_linear(const float y0, const float y1)
{
	Naui_Curve curve = naui_curve_make();
	naui_curve_add_point(&curve, 0.0f, y0, NAUI_CURVE_LINEAR);
	naui_curve_add_point(&curve, 1.0f, y1, NAUI_CURVE_LINEAR);
	return curve;
}

static inline Naui_Curve naui_curve_from_easing(const Naui_EaseFn ease)
{
	Naui_Curve curve = naui_curve_make();
	naui_curve_add_ease(&curve, 0.0f, 0.0f, ease);
	naui_curve_add_point(&curve, 1.0f, 1.0f, NAUI_CURVE_LINEAR);
	return curve;
}

static inline Naui_Curve naui_curve_cubic_bezier(const float x1, const float y1, const float x2, const float y2)
{
	Naui_Curve curve = naui_curve_make();
	naui_curve_add_bezier(&curve, 0.0f, 0.0f, x1, y1, x2, y2);
	naui_curve_add_point(&curve, 1.0f, 1.0f, NAUI_CURVE_LINEAR);
	return curve;
}

static inline float naui_curve_eval(const Naui_Curve *curve, const float x)
{
	if (!curve || curve->count <= 0)
		return 0.0f;

	const Naui_CurvePoint *points = curve->points;
	const int32_t last = curve->count - 1;

	if (x != x || x < points[0].x)
		return points[0].y;

	if (x >= points[last].x)
		return points[last].y;

	int32_t i = 0;
	while (i + 1 < last && points[i + 1].x <= x)
	{
		i++;
	}

	const Naui_CurvePoint *from = &points[i];
	const Naui_CurvePoint *to = &points[i + 1];
	const float t = (x - from->x) / (to->x - from->x);

	switch (from->interp)
	{
		case NAUI_CURVE_STEP:
			return from->y;

		case NAUI_CURVE_EASE:
			return NAUI_LERP(from->y, to->y, from->ease ? from->ease(t) : t);

		case NAUI_CURVE_BEZIER:
			return NAUI_LERP(from->y, to->y, naui_cubic_bezier(from->bezier[0], from->bezier[1], from->bezier[2], from->bezier[3], t));

		case NAUI_CURVE_LINEAR:
		default:
			return NAUI_LERP(from->y, to->y, t);
	}
}

#pragma endregion

static inline float naui_curve_advance(float progress, const float delta_seconds, const float duration_seconds, const bool forward)
{
	if (duration_seconds <= 0.0f)
		return forward ? 1.0f : 0.0f;

	if (progress != progress)
		progress = 0.0f;

	const float step = fmaxf(delta_seconds, 0.0f) / duration_seconds;
	return NAUI_CLAMP01(forward ? progress + step : progress - step);
}
