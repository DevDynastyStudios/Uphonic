typedef enum
{
	UPH_SNAP_BEAT,
	UPH_SNAP_HALF,
	UPH_SNAP_QUARTER,
	UPH_SNAP_EIGHTH,
	UPH_SNAP_SIXTEENTH,
	UPH_SNAP_LINE
}
Uph_SnapResolution;

#define UPH_GRID_MIN_SUB_SPACING 8.0f
#define UPH_GRID_MIN_BEAT_SPACING 5.0f
#define UPH_SNAP_LINE_MIN_DIVISION (1.0 / 64.0)

static inline double uph_time_signature_beat_length(void)
{
	const uint32_t denominator = uph_state.project.time_signature.denominator;
	return denominator > 0 ? 4.0 / (double)denominator : 1.0;
}

static inline double uph_time_signature_bar_length(void)
{
	const uint32_t numerator = uph_state.project.time_signature.numerator;
	return (double)(numerator > 0 ? numerator : 1) * uph_time_signature_beat_length();
}

static inline Leaf_Color uph_color_mix(Leaf_Color a, Leaf_Color b, float t)
{
	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	return (Leaf_Color) {
		(uint8_t)((float)a.r + ((float)b.r - (float)a.r) * t + 0.5f),
		(uint8_t)((float)a.g + ((float)b.g - (float)a.g) * t + 0.5f),
		(uint8_t)((float)a.b + ((float)b.b - (float)a.b) * t + 0.5f),
		(uint8_t)((float)a.a + ((float)b.a - (float)a.a) * t + 0.5f)
	};
}

static inline double uph_snap_line_division(float zoom_x)
{
	const double beat_length = uph_time_signature_beat_length();
	if (zoom_x <= 0.0f)
		return beat_length;

	if (beat_length * zoom_x < NAUI_DPI(UPH_GRID_MIN_BEAT_SPACING))
		return uph_time_signature_bar_length();

	double division = beat_length;
	while (division * 0.5 >= UPH_SNAP_LINE_MIN_DIVISION && division * 0.5 * zoom_x >= NAUI_DPI(UPH_GRID_MIN_SUB_SPACING))
		division *= 0.5;

	return division;
}

static inline double uph_snap_division(Uph_SnapResolution res, float zoom_x)
{
	switch (res)
	{
		case UPH_SNAP_BEAT:      return uph_time_signature_beat_length();
		case UPH_SNAP_HALF:      return 0.5;
		case UPH_SNAP_QUARTER:   return 0.25;
		case UPH_SNAP_EIGHTH:    return 0.125;
		case UPH_SNAP_SIXTEENTH: return 0.0625;
		case UPH_SNAP_LINE:      return uph_snap_line_division(zoom_x);
	}

	return 1.0;
}

static inline double uph_snap_beat_round(double beat, Uph_SnapResolution resolution, float zoom_x)
{
	const double division = uph_snap_division(resolution, zoom_x);
	return round(beat / division) * division;
}

static inline double uph_snap_beat_floor(double beat, Uph_SnapResolution resolution, float zoom_x)
{
	const double division = uph_snap_division(resolution, zoom_x);
	return floor(beat / division) * division;
}

typedef enum
{
	UPH_GRID_LINE_SUBDIVISION,
	UPH_GRID_LINE_BEAT,
	UPH_GRID_LINE_BAR
}
Uph_GridLineKind;

typedef void (*Uph_GridLineFn)(float x, Uph_GridLineKind kind, int64_t bar_index, void *user_data);

void uph_grid_lines(float origin_x, float width, float zoom_x, float scroll_x, double division, Uph_GridLineFn fn, void *user_data);
void uph_grid_render_beats(Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division);
void uph_grid_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division);
double uph_calculate_pattern_length(const Uph_MidiPattern *pattern);
double uph_calculate_automation_length(const Uph_Automation *automation);
void uph_automation_segment_controls(const Uph_AutomationPoint *from, float out[4]);
float uph_automation_segment_value(const Uph_AutomationPoint *from, const Uph_AutomationPoint *to, double beat);
bool uph_automation_value_at_beat(const Uph_Automation *automation, double beat, float *out_value);

#define UPH_MIXER_DB_MIN -60.0f
#define UPH_MIXER_DB_MAX 6.0f

float uph_linear_to_db(float linear);
float uph_db_to_linear(float db);
float uph_automation_to_volume(float value);
float uph_automation_to_pan(float value);

typedef uint8_t Uph_LoopRegionDragMode;
enum
{
	UPH_LOOP_DRAG_NONE,
	UPH_LOOP_DRAG_CREATE,
	UPH_LOOP_DRAG_START,
	UPH_LOOP_DRAG_END,
	UPH_LOOP_DRAG_MOVE
};

typedef struct
{
	double anchor;
	Uph_LoopRegionDragMode mode;
	bool moved;
	bool created;
}
Uph_LoopRegionDrag;

static inline bool uph_loop_region_valid(const Uph_LoopRegion *region)
{
	return region->end > region->start;
}

bool uph_loop_region_update(Uph_LoopRegion *region, Uph_LoopRegionDrag *drag, Leaf_BoundingBox bbox, float zoom_x, float scroll_x, double division, bool hovered);
void uph_loop_region_render_ruler(const Uph_LoopRegion *region, Leaf_BoundingBox bbox, float zoom_x, float scroll_x, bool enabled);
void uph_loop_region_render_bounds(const Uph_LoopRegion *region, float origin_x, float top, float bottom, float zoom_x, float scroll_x, bool enabled);
