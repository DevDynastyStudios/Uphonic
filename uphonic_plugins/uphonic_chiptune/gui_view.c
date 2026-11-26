#define PALETTE_BG 0x1A1C2C
#define PALETTE_PANEL 0x333C57
#define PALETTE_PANEL_EDGE 0x566C86
#define PALETTE_LCD 0x12131F
#define PALETTE_TEXT 0xF4F4F4
#define PALETTE_TEXT_DIM 0x94B0C2
#define PALETTE_TEXT_FAINT 0x566C86
#define PALETTE_ACCENT_OSC 0xEF7D57
#define PALETTE_ACCENT_ENV 0xA7F070
#define PALETTE_ACCENT_MOD 0x73EFF7
#define PALETTE_ACCENT_ARP 0xFFCD75
#define PALETTE_ACCENT_LOFI 0xD7609E
#define PALETTE_ACCENT_VOICE 0x41A6F6
#define PALETTE_ACCENT_OUT 0xF4F4F4
#define PALETTE_ACCENT_PCM 0xA29BFE
#define PALETTE_KEY_WHITE 0xF4F4F4
#define PALETTE_KEY_WHITE_SHADE 0x94B0C2
#define PALETTE_KEY_BLACK 0x1A1C2C

#define STATUS_TEXT_SIZE 96
#define VALUE_TEXT_SIZE 32



#define KNOB_CELL_WIDTH 38
#define KNOB_CELL_HEIGHT 42
#define KNOB_LABEL_Y 25
#define KNOB_VALUE_Y 34
#define KNOB_SWEEP_DEGREES 270.0
#define KNOB_HALF_SWEEP_DEGREES 135.0
#define KNOB_RING_INNER 7.5
#define KNOB_RING_OUTER 10.5
#define KNOB_POINTER_LENGTH 6.0
#define KNOB_POINTER_WIDTH 1.6
#define KNOB_OUTLINE_WIDTH 1.0
#define KNOB_ARC_ORIGIN_DEGREES 90.0
#define KNOB_ARC_MIN_SWEEP_DEGREES 0.01
#define RADIANS_TO_DEGREES 57.29577951308232
#define KNOB_DRAG_PIXELS 100.0
#define KNOB_FINE_FACTOR 0.1
#define KNOB_WHEEL_STEP 0.02

#define KEYBOARD_HEIGHT 40
#define KEYBOARD_FIRST_KEY 48
#define KEYBOARD_WHITE_KEYS 21
#define WHITE_KEY_WIDTH 18
#define BLACK_KEY_WIDTH 11
#define BLACK_KEY_HEIGHT 24
#define KEYS_PER_OCTAVE 12
#define WHITE_KEYS_PER_OCTAVE 7

#define SCOPE_SAMPLES SYNTH_SCOPE_SIZE
#define SCOPE_MIN_WINDOW 96
#define SCOPE_MAX_WINDOW 800
#define SCOPE_DEFAULT_WINDOW 512
#define SCOPE_PERIODS_SHOWN 3
#define SCOPE_MAX_CROSSINGS 64
#define SCOPE_MIN_PEAK 0.02f
#define SCOPE_SILENCE 0.00001f
#define SCOPE_TRIGGER_HYSTERESIS 0.01f

#define ENV_GRAPH_HOLD_FRACTION 0.25
#define ENV_GRAPH_MIN_HOLD_MS 100.0

#define PRESET_HEIGHT 11
#define PRESET_WIDTH 11
#define PRESET_PICTURE_WIDTH 8
#define PRESET_MATCH_TOLERANCE 0.01

#define MAX_WIDGETS 96
#define TITLE_BAR_HEIGHT 20
#define PANEL_PADDING_SIDE 4
#define PANEL_PADDING_TOP 3
#define ROW_GAP 4
#define SECTION_GAP 3
#define SIDE_MARGIN 6
#define KEYBOARD_STRIP_HEIGHT 46
#define ROW_A_HEIGHT 78
#define ROW_B_HEIGHT 56
#define ROW_C_HEIGHT 56
#define ROW_D_HEIGHT 56
#define PRESET_BAR_HEIGHT 16
#define PRESET_VALUE_TOLERANCE 0.000001
#define STEPPER_ARROW_WIDTH 12
#define PCM_TRACE_MAX_STRIDE_SAMPLES 32
#define GAIN_CELL_WIDTH 44


#define TITLE_TEXT_SCALE 2
#define TITLE_GAP 6
#define TEXT_GLYPH_HEIGHT 7
#define WIDGET_ID_LABEL "uphonic_widget"
#define ROOT_ID_LABEL "uphonic_root"

#define WAVE_BUTTON_WIDTH 38
#define WAVE_BUTTON_HEIGHT 14
#define SMALL_BUTTON_HEIGHT 13
#define PRESET_GAP 2
#define LOGICAL_KEYBOARD_WIDTH (KEYBOARD_WHITE_KEYS * WHITE_KEY_WIDTH)

typedef enum WidgetKind
{
	WIDGET_KNOB,
	WIDGET_CHOICE,
	WIDGET_PRESET,
	WIDGET_ENV_GRAPH,
	WIDGET_OSC_PREVIEW,
	WIDGET_SCOPE,
	WIDGET_KEYBOARD,
	WIDGET_TEXT,
	WIDGET_PCM_DISPLAY,
	WIDGET_STEPPER,
	WIDGET_RESET,
} WidgetKind;

typedef struct GuiWidget
{
	WidgetKind kind;
	int width;
	int height;
	ParamId param;
	double value;
	const char* label;
	uint32_t color;
	ParamId enabled_param;
	int enabled_value;
} GuiWidget;

typedef struct GuiWidgetRef
{
	GuiView* view;
	int index;
} GuiWidgetRef;

typedef struct WidgetColors
{
	uint32_t fill;
	uint32_t edge;
	uint32_t ink;
} WidgetColors;

struct GuiView
{
	Plugin* plugin;
	int pixel_scale;

	GuiWidget widgets[MAX_WIDGETS];
	int widget_count;
	int hit_count;

	int hover_widget;
	int active_widget;
	int pressed_key;
	double drag_position;
	int drag_last_y;
	bool reset_armed;
	int preset_index;
	char preset_label[PRESET_NAME_SIZE + 2];

	float scope[SCOPE_SAMPLES];
	bool scope_active;
	uint32_t voice_count;
	uint64_t keys_low;
	uint64_t keys_high;
	char status[STATUS_TEXT_SIZE];
};

static const char* const k_wave_labels[SYNTH_WAVE_COUNT] = { "PULSE", "TRI", "SAW", "NOISE", "PCM" };
static const char* const k_arp_labels[SYNTH_ARP_COUNT] = { "OFF", "MAJ", "MIN", "DOM7", "MAJ7", "MIN7", "OCT", "PWR" };
static const double k_duty_presets[] = { 0.125, 0.25, 0.5, 0.75 };

// Which black key sits after each white key of an octave (C D E F G A B).
static const bool k_black_after_white[WHITE_KEYS_PER_OCTAVE] = { true, true, false, true, true, true, false };
static const int k_white_key_semitone[WHITE_KEYS_PER_OCTAVE] = { 0, 2, 4, 5, 7, 9, 11 };

#pragma region Private API

static uint32_t uct__mix_color(uint32_t from, uint32_t to, double amount)
{
	uint32_t result = 0;

	for(int shift = 0; shift <= 16; shift += 8)
	{
		double a = (from >> shift) & 0xFF;
		double b = (to >> shift) & 0xFF;
		result |= (uint32_t)(a + (b - a) * amount) << shift;
	}

	return result;
}

static uint32_t uct__dim_color(uint32_t color)
{
	return uct__mix_color(color, PALETTE_PANEL, 0.7);
}

static bool uct__widget_enabled(const GuiView* view, const GuiWidget* widget)
{
	if(widget->enabled_param == PARAM_COUNT)
		return true;

	const int current = (int)params_get(view->plugin, widget->enabled_param);

	if(widget->enabled_value < 0)
		return current != SYNTH_ARP_OFF;

	return current == widget->enabled_value;
}

static void uct__to_upper(const char* text, char* out, size_t capacity)
{
	size_t i = 0;

	for(; text[i] != '\0' && i + 1 < capacity; i++)
	{
		out[i] = (char)toupper((unsigned char)text[i]);
	}

	out[i] = '\0';
}

static WidgetColors uct__widget_colors(const GuiView* view, int index)
{
	const GuiWidget* widget = &view->widgets[index];
	const bool hovered = index == view->hover_widget;
	const bool enabled = uct__widget_enabled(view, widget);
	const uint32_t accent = enabled ? widget->color : uct__dim_color(widget->color);
	const uint32_t edge = hovered ? PALETTE_TEXT_DIM : PALETTE_PANEL_EDGE;

	WidgetColors colors = { GUI_NO_COLOR, GUI_NO_COLOR, PALETTE_TEXT_DIM };

	switch(widget->kind)
	{
		case WIDGET_CHOICE:
		{
			const bool selected = (int)params_get(view->plugin, widget->param) == (int)widget->value;
			colors.fill = selected ? accent : PALETTE_BG;
			colors.edge = edge;
			colors.ink = selected ? PALETTE_BG : (enabled ? PALETTE_TEXT_DIM : PALETTE_TEXT_FAINT);
			break;
		}

		case WIDGET_PRESET:
		{
			const bool selected = fabs(params_get(view->plugin, widget->param) - widget->value) < PRESET_MATCH_TOLERANCE;
			colors.fill = selected ? accent : PALETTE_BG;
			colors.edge = edge;
			colors.ink = selected ? PALETTE_BG : (enabled ? PALETTE_TEXT_DIM : PALETTE_TEXT_FAINT);
			break;
		}

		case WIDGET_STEPPER:
			colors.fill = PALETTE_BG;
			colors.edge = edge;
			colors.ink = enabled ? PALETTE_TEXT : PALETTE_TEXT_FAINT;
			break;

		case WIDGET_RESET:
			colors.fill = view->reset_armed ? widget->color : PALETTE_BG;
			colors.edge = edge;
			colors.ink = view->reset_armed ? PALETTE_BG : PALETTE_TEXT_DIM;
			break;

		case WIDGET_ENV_GRAPH:
		case WIDGET_OSC_PREVIEW:
		case WIDGET_SCOPE:
		case WIDGET_PCM_DISPLAY:
			colors.fill = PALETTE_LCD;
			colors.edge = PALETTE_PANEL_EDGE;
			break;

		default:
			break;
	}

	return colors;
}

static void uct__stroke_knob_arc(float center_x, float center_y, float radius, float width, double low, double high, uint32_t color)
{
	const double inset = (double)(width * 0.5f / radius) * RADIANS_TO_DEGREES;
	double from = low + inset;
	double to = high - inset;

	if(to <= from)
	{
		const double middle = (low + high) * 0.5;
		from = fmin(fmax(middle, -KNOB_HALF_SWEEP_DEGREES + inset), KNOB_HALF_SWEEP_DEGREES - inset);
		to = from + KNOB_ARC_MIN_SWEEP_DEGREES;
	}

	const float start = (float)((from - KNOB_ARC_ORIGIN_DEGREES) / RADIANS_TO_DEGREES);
	const float end = (float)((to - KNOB_ARC_ORIGIN_DEGREES) / RADIANS_TO_DEGREES);
	upl_stroke_arc(center_x, center_y, radius, start, end, gui_color(color), width);
}

static void uct__draw_knob(const GuiView* view, const GuiWidget* widget, const GuiPen* pen, bool hovered, bool active)
{
	const bool enabled = uct__widget_enabled(view, widget);
	const ParamStyle* style = params_style(widget->param);
	const double value = params_get(view->plugin, widget->param);
	const double position = params_to_position(widget->param, value);

	uint32_t accent = enabled ? widget->color : uct__dim_color(widget->color);
	uint32_t track = uct__mix_color(PALETTE_PANEL, PALETTE_BG, 0.55);
	uint32_t body = uct__mix_color(PALETTE_PANEL, PALETTE_PANEL_EDGE, active ? 0.75 : 0.45);
	uint32_t outline = hovered || active ? PALETTE_TEXT_DIM : PALETTE_BG;

	const double angle_now = -KNOB_HALF_SWEEP_DEGREES + KNOB_SWEEP_DEGREES * position;
	const double angle_from = style->bipolar ? 0.0 : -KNOB_HALF_SWEEP_DEGREES;
	const double arc_low = fmin(angle_from, angle_now);
	const double arc_high = fmax(angle_from, angle_now);

	const float scale = pen->scale;
	const float outline_radius = (float)(KNOB_RING_OUTER + KNOB_OUTLINE_WIDTH) * scale;
	const float center_x = pen->x + (widget->width / 2) * scale;
	const float center_y = pen->y + outline_radius;
	const float ring_radius = (float)(KNOB_RING_INNER + KNOB_RING_OUTER) * 0.5f * scale;
	const float ring_width = (float)(KNOB_RING_OUTER - KNOB_RING_INNER) * scale;

	upl_fill_circle(center_x, center_y, outline_radius, gui_color(outline));
	upl_fill_circle(center_x, center_y, (float)KNOB_RING_OUTER * scale, gui_color(body));

	uct__stroke_knob_arc(center_x, center_y, ring_radius, ring_width, -KNOB_HALF_SWEEP_DEGREES, KNOB_HALF_SWEEP_DEGREES, track);
	uct__stroke_knob_arc(center_x, center_y, ring_radius, ring_width, arc_low, arc_high, accent);

	const double pointer_x = sin(angle_now / RADIANS_TO_DEGREES);
	const double pointer_y = -cos(angle_now / RADIANS_TO_DEGREES);
	const float cap = KNOB_POINTER_WIDTH * 0.5f;
	const float near = cap * scale;
	const float far = (KNOB_POINTER_LENGTH - cap) * scale;
	const uint32_t pointer = enabled ? PALETTE_TEXT : PALETTE_TEXT_DIM;

	upl_draw_line(center_x + (float)pointer_x * near, center_y + (float)pointer_y * near, center_x + (float)pointer_x * far, center_y + (float)pointer_y * far, gui_color(pointer), KNOB_POINTER_WIDTH * scale);

	char text[VALUE_TEXT_SIZE];
	params_format(view->plugin, widget->param, value, text, sizeof(text));

	const int center = widget->width / 2;
	gui_draw_text_centered(pen, center, KNOB_LABEL_Y, widget->label, hovered || active ? PALETTE_TEXT : PALETTE_TEXT_DIM, 1);
	gui_draw_text_centered(pen, center, KNOB_VALUE_Y, text, accent, 1);
}

static void uct__draw_choice(const GuiWidget* widget, const GuiPen* pen, const WidgetColors* colors)
{
	const int text_y = (widget->height - TEXT_GLYPH_HEIGHT) / 2;
	gui_draw_text_centered(pen, widget->width / 2, text_y, widget->label, colors->ink, 1);
}

static void uct__draw_preset(const GuiWidget* widget, const GuiPen* pen, const WidgetColors* colors)
{
	const uint32_t ink = colors->ink;
	const int left = 1 + (widget->width - 2 - PRESET_PICTURE_WIDTH) / 2;
	const int high_y = 3;
	const int low_y = widget->height - 4;
	const int high_columns = (int)(widget->value * PRESET_PICTURE_WIDTH + 0.5);

	gui_draw_hline(pen, left, high_y, high_columns, ink);
	gui_draw_hline(pen, left + high_columns, low_y, PRESET_PICTURE_WIDTH - high_columns, ink);

	gui_draw_vline(pen, left, high_y, low_y - high_y + 1, ink);
	gui_draw_vline(pen, left + high_columns, high_y, low_y - high_y + 1, ink);
}

static double uct__noise_preview_value(int index, double shape)
{
	uint32_t hash = (uint32_t)index * 2654435761u;
	hash ^= hash >> 13;
	hash *= 0x5BD1E995u;
	hash ^= hash >> 15;

	const double high = (double)(hash & 0xFF) / 256.0 < shape ? 1.0 : -1.0;
	const double offset = 2.0 * shape - 1.0;
	return (high - offset) / (1.0 + fabs(offset));
}

static double uct__oscillator_preview_value(int waveform, double phase, double shape, int index)
{
	switch(waveform)
	{
		case SYNTH_WAVE_PULSE:
			return phase < shape ? 1.0 : -1.0;

		case SYNTH_WAVE_TRIANGLE:
		{
			const double warped = uct__warp_phase(phase, shape);
			int step = (int)(warped * TRIANGLE_STEPS);
			int level = step < TRIANGLE_LEVELS ? step : TRIANGLE_STEPS - 1 - step;
			return level / ((TRIANGLE_LEVELS - 1) * 0.5) - 1.0;
		}

		case SYNTH_WAVE_SAW:
			return 2.0 * uct__warp_phase(phase, shape) - 1.0;
	}

	return uct__noise_preview_value(index, shape);
}

static void uct__draw_sample_trace(const GuiPen* pen, int x, int width, int middle_y, int amplitude, const PcmSample* sample, uint32_t color)
{

	if(sample == NULL || sample->length == 0)
		return;

	for(int column = 0; column < width; column++)
	{
		const uint32_t start = (uint32_t)((uint64_t)column * sample->length / width);
		uint32_t end = (uint32_t)((uint64_t)(column + 1) * sample->length / width);

		if(end <= start)
			end = start + 1;

		uint32_t stride = (end - start) / PCM_TRACE_MAX_STRIDE_SAMPLES;
		if(stride == 0)
			stride = 1;

		float low = sample->data[start];
		float high = low;

		for(uint32_t i = start; i < end && i < sample->length; i += stride)
		{
			low = fminf(low, sample->data[i]);
			high = fmaxf(high, sample->data[i]);
		}

		const int top = middle_y - (int)(high * amplitude);
		const int bottom = middle_y - (int)(low * amplitude);
		gui_draw_vline(pen, x + column, top, bottom - top + 1, color);
	}
}

static void uct__draw_osc_preview(const GuiView* view, const GuiWidget* widget, const GuiPen* pen)
{
	const int waveform = (int)params_get(view->plugin, PARAM_WAVEFORM);
	const double shape = params_get(view->plugin, PARAM_SHAPE);

	const int inner_x = 2;
	const int inner_width = widget->width - 4;
	const int middle_y = widget->height / 2;
	const int amplitude = widget->height / 2 - 3;
	const int periods = waveform == SYNTH_WAVE_NOISE ? 1 : 2;

	gui_draw_hline(pen, inner_x, middle_y, inner_width, uct__mix_color(PALETTE_LCD, PALETTE_PANEL_EDGE, 0.5));

	if(waveform == SYNTH_WAVE_PCM)
	{
		const PcmSample* sample = pcm_bank_get(view->plugin->pcm_bank, (int)params_get(view->plugin, PARAM_PCM_SAMPLE));
		uct__draw_sample_trace(pen, inner_x, inner_width, middle_y, amplitude, sample, widget->color);
		return;
	}

	int previous_y = middle_y;

	for(int column = 0; column < inner_width; column++)
	{
		const double phase = fmod((double)column * periods / inner_width, 1.0);
		const double value = uct__oscillator_preview_value(waveform, phase, shape, column / 2);
		const int y = middle_y - (int)(value * amplitude);

		if(column > 0)
			gui_draw_vline(pen, inner_x + column, y < previous_y ? y : previous_y, abs(y - previous_y) + 1, widget->color);
		else
			gui_draw_pixel(pen, inner_x, y, widget->color);

		previous_y = y;
	}
}

static void uct__draw_pcm_display(const GuiView* view, const GuiWidget* widget, const GuiPen* pen)
{
	const bool enabled = uct__widget_enabled(view, widget);
	const PcmSample* sample = pcm_bank_get(view->plugin->pcm_bank, (int)params_get(view->plugin, PARAM_PCM_SAMPLE));

	const int middle_y = widget->height / 2;
	gui_draw_hline(pen, 2, middle_y, widget->width - 4, uct__mix_color(PALETTE_LCD, PALETTE_PANEL_EDGE, 0.5));

	uct__draw_sample_trace(pen, 2, widget->width - 4, middle_y, middle_y - 3, sample, enabled ? widget->color : uct__dim_color(widget->color));

	if(sample == NULL)
		return;

	char info[VALUE_TEXT_SIZE];
	const double seconds = (double)sample->length / sample->rate;
	if(seconds < 1.0)
		snprintf(info, sizeof(info), "%.0fms %uHz", seconds * 1000.0, sample->rate);
	else
		snprintf(info, sizeof(info), "%.2fs %uHz", seconds, sample->rate);
	gui_draw_text(pen, widget->width - 3 - gui_draw_text_width(info, 1), 3, info, PALETTE_TEXT_FAINT, 1);
}

static void uct__draw_stepper(const GuiView* view, const GuiWidget* widget, const GuiPen* pen, const WidgetColors* colors)
{
	const bool enabled = uct__widget_enabled(view, widget);
	const uint32_t accent = enabled ? widget->color : uct__dim_color(widget->color);

	char name[VALUE_TEXT_SIZE];

	if(widget->param == PARAM_COUNT)
		snprintf(name, sizeof(name), "%s", widget->label);
	else
		params_format(view->plugin, widget->param, params_get(view->plugin, widget->param), name, sizeof(name));

	const int room = (widget->width - 2 * STEPPER_ARROW_WIDTH) / 6;
	if((int)strlen(name) > room)
		name[room] = '\0';

	const int text_y = (widget->height - TEXT_GLYPH_HEIGHT) / 2;
	gui_draw_text(pen, 3, text_y, "<", accent, 1);
	gui_draw_text(pen, widget->width - 8, text_y, ">", accent, 1);
	gui_draw_text_centered(pen, widget->width / 2, text_y, name, colors->ink, 1);
}

static void uct__draw_reset(const GuiWidget* widget, const GuiPen* pen, const WidgetColors* colors)
{
	gui_draw_text_centered(pen, widget->width / 2, (widget->height - TEXT_GLYPH_HEIGHT) / 2, widget->label, colors->ink, 1);
}

static void uct__draw_envelope_graph(const GuiView* view, const GuiWidget* widget, const GuiPen* pen)
{
	const double attack = params_get(view->plugin, PARAM_ATTACK);
	const double decay = params_get(view->plugin, PARAM_DECAY);
	const double sustain = params_get(view->plugin, PARAM_SUSTAIN);
	const double release = params_get(view->plugin, PARAM_RELEASE);
	const double hold = fmax(ENV_GRAPH_MIN_HOLD_MS, (attack + decay + release) * ENV_GRAPH_HOLD_FRACTION);
	const double total = attack + decay + hold + release;

	const int left = 2;
	const int bottom = widget->height - 3;
	const int graph_width = widget->width - 4;
	const int graph_height = widget->height - 6;

	const double times[5] = { 0.0, attack, attack + decay, attack + decay + hold, total };
	const double levels[5] = { 0.0, 1.0, sustain, sustain, 0.0 };

	for(int column = 0; column < graph_width; column++)
	{
		const double time = total * column / (graph_width - 1);
		double level = 0.0;

		for(int segment = 0; segment < 4; segment++)
		{
			if(time < times[segment] || time > times[segment + 1])
				continue;

			const double length = times[segment + 1] - times[segment];
			const double fraction = length > 0.0 ? (time - times[segment]) / length : 1.0;
			level = levels[segment] + (levels[segment + 1] - levels[segment]) * fraction;
			break;
		}

		const int top = bottom - (int)(level * graph_height + 0.5);
		for(int y = top + 1; y <= bottom; y++)
		{
			if(((y + column) & 1) == 0)
				gui_draw_pixel(pen, left + column, y, uct__mix_color(PALETTE_LCD, widget->color, 0.25));
		}

		gui_draw_pixel(pen, left + column, top, widget->color);
	}
}

static uint32_t uct__find_scope_window(const float* samples, uint32_t* out_start)
{
	uint32_t crossings[SCOPE_MAX_CROSSINGS];
	uint32_t crossing_count = 0;

	for(uint32_t i = 1; i < SCOPE_SAMPLES && crossing_count < SCOPE_MAX_CROSSINGS; i++)
	{
		if(samples[i - 1] < -SCOPE_TRIGGER_HYSTERESIS && samples[i] >= 0.0f)
		{
			crossings[crossing_count] = i;
			crossing_count++;
		}
	}

	uint32_t window = SCOPE_DEFAULT_WINDOW;

	if(crossing_count >= 2)
	{
		window = (crossings[1] - crossings[0]) * SCOPE_PERIODS_SHOWN;

		if(window < SCOPE_MIN_WINDOW)
			window = SCOPE_MIN_WINDOW;

		if(window > SCOPE_MAX_WINDOW)
			window = SCOPE_MAX_WINDOW;
	}

	uint32_t start = SCOPE_SAMPLES - window;
	for(uint32_t i = crossing_count; i > 0; i--)
	{
		if(crossings[i - 1] + window <= SCOPE_SAMPLES)
		{
			start = crossings[i - 1];
			break;
		}
	}

	*out_start = start;
	return window;
}

static void uct__draw_scope(const GuiView* view, const GuiWidget* widget, const GuiPen* pen)
{

	const int inner_x = 2;
	const int inner_width = widget->width - 4;
	const int middle_y = widget->height / 2;
	const int amplitude = widget->height / 2 - 3;

	gui_draw_hline(pen, inner_x, middle_y, inner_width, uct__mix_color(PALETTE_LCD, PALETTE_PANEL_EDGE, 0.5));

	if(!view->scope_active)
		return;

	uint32_t start = 0;
	const uint32_t window = uct__find_scope_window(view->scope, &start);

	float peak = SCOPE_MIN_PEAK;
	for(uint32_t i = 0; i < window; i++)
	{
		peak = fmaxf(peak, fabsf(view->scope[start + i]));
	}

	int previous_y = middle_y;

	for(int column = 0; column < inner_width; column++)
	{
		const uint32_t index = start + (uint32_t)((uint64_t)column * window / inner_width);
		const int y = middle_y - (int)(view->scope[index] / peak * amplitude);

		if(column > 0)
			gui_draw_vline(pen, inner_x + column, y < previous_y ? y : previous_y, abs(y - previous_y) + 1, widget->color);
		else
			gui_draw_pixel(pen, inner_x, y, widget->color);

		previous_y = y;
	}
}

static bool uct__key_is_lit(const GuiView* view, int key)
{
	if(key == view->pressed_key)
		return true;

	if(key < 0 || key >= KEY_COUNT)
		return false;

	const uint64_t mask = key < KEYS_PER_MASK ? view->keys_low : view->keys_high;
	return ((mask >> (key % KEYS_PER_MASK)) & 1u) != 0;
}

static int uct__white_key_number(int white_index)
{
	const int octave = white_index / WHITE_KEYS_PER_OCTAVE;
	return KEYBOARD_FIRST_KEY + octave * KEYS_PER_OCTAVE + k_white_key_semitone[white_index % WHITE_KEYS_PER_OCTAVE];
}

static int uct__black_key_after(int white_index)
{
	if(white_index >= KEYBOARD_WHITE_KEYS - 1)
		return -1;

	if(!k_black_after_white[white_index % WHITE_KEYS_PER_OCTAVE])
		return -1;

	return uct__white_key_number(white_index) + 1;
}

static int uct__key_at(int local_x, int local_y)
{
	if(local_x < 0 || local_y < 0 || local_x >= LOGICAL_KEYBOARD_WIDTH || local_y >= KEYBOARD_HEIGHT)
		return -1;

	const int white_index = local_x / WHITE_KEY_WIDTH;
	if(local_y < BLACK_KEY_HEIGHT)
	{
		for(int index = (white_index > 0 ? white_index - 1 : 0); index <= white_index && index < KEYBOARD_WHITE_KEYS; index++)
		{
			const int black_key = uct__black_key_after(index);
			if(black_key < 0)
				continue;

			const int black_left = (index + 1) * WHITE_KEY_WIDTH - BLACK_KEY_WIDTH / 2;

			if(local_x >= black_left && local_x < black_left + BLACK_KEY_WIDTH)
				return black_key;
		}
	}

	return uct__white_key_number(white_index);
}

static void uct__draw_keyboard(const GuiView* view, const GuiWidget* widget, const GuiPen* pen)
{
	char label[VALUE_TEXT_SIZE];

	for(int white_index = 0; white_index < KEYBOARD_WHITE_KEYS; white_index++)
	{
		const int key = uct__white_key_number(white_index);
		const int x = white_index * WHITE_KEY_WIDTH;
		const bool lit = uct__key_is_lit(view, key);

		gui_draw_fill_rect(pen, x, 0, WHITE_KEY_WIDTH, widget->height, lit ? widget->color : PALETTE_KEY_WHITE);
		gui_draw_vline(pen, x + WHITE_KEY_WIDTH - 1, 0, widget->height, PALETTE_KEY_WHITE_SHADE);
		gui_draw_hline(pen, x, widget->height - 1, WHITE_KEY_WIDTH, PALETTE_KEY_WHITE_SHADE);

		if(key % KEYS_PER_OCTAVE != 0)
			continue;

		snprintf(label, sizeof(label), "C%d", key / KEYS_PER_OCTAVE - 1);
		gui_draw_text_centered(pen, x + WHITE_KEY_WIDTH / 2, widget->height - 11, label, PALETTE_TEXT_FAINT, 1);
	}

	for(int white_index = 0; white_index < KEYBOARD_WHITE_KEYS; white_index++)
	{
		const int key = uct__black_key_after(white_index);
		if(key < 0)
			continue;

		const int x = (white_index + 1) * WHITE_KEY_WIDTH - BLACK_KEY_WIDTH / 2;
		const bool lit = uct__key_is_lit(view, key);

		gui_draw_fill_rect(pen, x, 0, BLACK_KEY_WIDTH, BLACK_KEY_HEIGHT, lit ? widget->color : PALETTE_KEY_BLACK);
		gui_draw_rect(pen, x, 0, BLACK_KEY_WIDTH, BLACK_KEY_HEIGHT, PALETTE_BG);
		gui_draw_vline(pen, x + 1, 0, BLACK_KEY_HEIGHT - 1, lit ? widget->color : PALETTE_PANEL);
	}
}

static int uct__pixel_scale(void)
{
	int pixel_scale = (int)(GUI_BASE_PIXEL_SCALE * upl_scale() + GUI_SCALE_ROUNDING);

	if(pixel_scale < 1)
		pixel_scale = 1;

	if(pixel_scale > GUI_MAX_PIXEL_SCALE)
		pixel_scale = GUI_MAX_PIXEL_SCALE;

	return pixel_scale;
}

static int uct__wheel_notches(void)
{
	const float wheel = upl_mouse_wheel();
	const int notches = (int)roundf(wheel);

	if(notches != 0)
		return notches;

	if(wheel > 0.0f)
		return 1;

	if(wheel < 0.0f)
		return -1;

	return 0;
}

static int32_t uct__px(const GuiView* view, int value)
{
	return (int32_t)(value * view->pixel_scale);
}

static Leaf_ID uct__widget_id(int index)
{
	return leaf_id_indexed(WIDGET_ID_LABEL, (uint64_t)index);
}

static Leaf_Color uct__fill(uint32_t color)
{
	return gui_color(color);
}

static void uct__draw_leaf_widget(Leaf_BoundingBox box, void* user_data)
{
	const GuiWidgetRef* ref = (const GuiWidgetRef*)user_data;
	const GuiView* view = ref->view;
	const GuiWidget* widget = &view->widgets[ref->index];
	const GuiPen pen = { (float)box.x, (float)box.y, (float)view->pixel_scale };
	const WidgetColors colors = uct__widget_colors(view, ref->index);

	const bool hovered = ref->index == view->hover_widget;
	const bool active = ref->index == view->active_widget;

	switch(widget->kind)
	{
		case WIDGET_KNOB:
			uct__draw_knob(view, widget, &pen, hovered, active);
			break;

		case WIDGET_CHOICE:
			uct__draw_choice(widget, &pen, &colors);
			break;

		case WIDGET_PRESET:
			uct__draw_preset(widget, &pen, &colors);
			break;

		case WIDGET_ENV_GRAPH:
			uct__draw_envelope_graph(view, widget, &pen);
			break;

		case WIDGET_OSC_PREVIEW:
			uct__draw_osc_preview(view, widget, &pen);
			break;

		case WIDGET_SCOPE:
			uct__draw_scope(view, widget, &pen);
			break;

		case WIDGET_KEYBOARD:
			uct__draw_keyboard(view, widget, &pen);
			break;

		case WIDGET_TEXT:
			gui_draw_text(&pen, 0, 0, widget->label, widget->color, (int)widget->value);
			break;

		case WIDGET_PCM_DISPLAY:
			uct__draw_pcm_display(view, widget, &pen);
			break;

		case WIDGET_STEPPER:
			uct__draw_stepper(view, widget, &pen, &colors);
			break;

		case WIDGET_RESET:
			uct__draw_reset(widget, &pen, &colors);
			break;
	}
}

static void uct__widget(GuiView* view, GuiWidget widget)
{
	if(view->widget_count >= MAX_WIDGETS)
		return;

	const int index = view->widget_count;
	view->widgets[index] = widget;
	view->widget_count++;

	GuiWidgetRef ref = { view, index };
	const WidgetColors colors = uct__widget_colors(view, index);

	Leaf_ElementConfig frame =
	{
		.id = uct__widget_id(index),
		.size = { LEAF_SIZE_FIXED(uct__px(view, widget.width)), LEAF_SIZE_FIXED(uct__px(view, widget.height)) },
		.color = uct__fill(colors.fill),
	};

	if(colors.edge != GUI_NO_COLOR)
		frame.border = (Leaf_Border){ .width = view->pixel_scale, .sides = LEAF_SIDE_ALL, .color = uct__fill(colors.edge) };

	leaf_begin_element(frame);
	leaf({ .size = { LEAF_SIZE_GROW, LEAF_SIZE_GROW }, .custom_draw = uct__draw_leaf_widget, .custom_draw_data = LEAF_DATA_SLICE(ref) });
	leaf_end_element();
}

static void uct__knob(GuiView* view, int width, ParamId param, const char* label, uint32_t color, ParamId enabled_param, int enabled_value)
{
	GuiWidget widget =
	{
		.kind = WIDGET_KNOB,
		.width = width,
		.height = KNOB_CELL_HEIGHT,
		.param = param,
		.label = label,
		.color = color,
		.enabled_param = enabled_param,
		.enabled_value = enabled_value,
	};

	uct__widget(view, widget);
}

static void uct__choice(GuiView* view, int width, int height, ParamId param, int value, const char* label, uint32_t color, ParamId enabled_param, int enabled_value)
{
	GuiWidget widget =
	{
		.kind = WIDGET_CHOICE,
		.width = width,
		.height = height,
		.param = param,
		.value = value,
		.label = label,
		.color = color,
		.enabled_param = enabled_param,
		.enabled_value = enabled_value,
	};

	uct__widget(view, widget);
}

static void uct__preset(GuiView* view, int index)
{
	GuiWidget widget =
	{
		.kind = WIDGET_PRESET,
		.width = PRESET_WIDTH,
		.height = PRESET_HEIGHT,
		.param = PARAM_SHAPE,
		.value = k_duty_presets[index],
		.color = PALETTE_ACCENT_OSC,
		.enabled_param = PARAM_COUNT,
	};

	uct__widget(view, widget);
}

static void uct__display(GuiView* view, WidgetKind kind, int width, int height, uint32_t color, ParamId enabled_param, int enabled_value)
{
	GuiWidget widget =
	{
		.kind = kind,
		.width = width,
		.height = height,
		.param = PARAM_COUNT,
		.color = color,
		.enabled_param = enabled_param,
		.enabled_value = enabled_value,
	};

	uct__widget(view, widget);
}

static void uct__stepper(GuiView* view, int width, int height, ParamId param, const char* label, uint32_t color, ParamId enabled_param, int enabled_value)
{
	GuiWidget widget =
	{
		.kind = WIDGET_STEPPER,
		.width = width,
		.height = height,
		.param = param,
		.label = label,
		.color = color,
		.enabled_param = enabled_param,
		.enabled_value = enabled_value,
	};

	uct__widget(view, widget);
}

static void uct__reset_button(GuiView* view, int width, int height, const char* label, uint32_t color)
{
	GuiWidget widget =
	{
		.kind = WIDGET_RESET,
		.width = width,
		.height = height,
		.param = PARAM_COUNT,
		.label = label,
		.color = color,
		.enabled_param = PARAM_COUNT,
	};

	uct__widget(view, widget);
}

static void uct__text(GuiView* view, const char* label, uint32_t color, int scale)
{
	GuiWidget widget =
	{
		.kind = WIDGET_TEXT,
		.width = gui_draw_text_width(label, scale),
		.height = TEXT_GLYPH_HEIGHT * scale + scale,
		.param = PARAM_COUNT,
		.value = scale,
		.label = label,
		.color = color,
		.enabled_param = PARAM_COUNT,
	};

	uct__widget(view, widget);
}

static void uct__begin_stack(const GuiView* view, Leaf_LayoutDirection direction, int gap, Leaf_LayoutAlignmentX align_x, Leaf_LayoutAlignmentY align_y)
{
	Leaf_ElementConfig config =
	{
		.direction = direction,
		.child_gap = uct__px(view, gap),
		.child_alignment = { align_x, align_y },
	};

	leaf_begin_element(config);
}

static void uct__end(void)
{
	leaf_end_element();
}

static void uct__begin_panel(GuiView* view, int width, int height, const char* title, uint32_t color)
{
	Leaf_ElementConfig config =
	{
		.size = { LEAF_SIZE_FIXED(uct__px(view, width - PANEL_PADDING_SIDE * 2)), LEAF_SIZE_FIXED(uct__px(view, height - PANEL_PADDING_TOP)) },
		.padding = { uct__px(view, PANEL_PADDING_SIDE), uct__px(view, PANEL_PADDING_SIDE), uct__px(view, PANEL_PADDING_TOP), 0 },
		.color = uct__fill(PALETTE_PANEL),
		.border = { .width = view->pixel_scale, .sides = LEAF_SIDE_ALL, .color = uct__fill(PALETTE_BG) },
		.child_gap = uct__px(view, 1),
	};

	leaf_begin_element(config);

	if(title != NULL)
		uct__text(view, title, color, 1);
}

static void uct__declare_title_bar(GuiView* view)
{
	Leaf_ElementConfig config =
	{
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(uct__px(view, TITLE_BAR_HEIGHT)) },
		.border = { .width = view->pixel_scale, .sides = LEAF_SIDE_BOTTOM, .color = uct__fill(PALETTE_PANEL) },
		.child_gap = uct__px(view, TITLE_GAP),
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.direction = LEAF_DIRECTION_HORIZONTAL,
	};

	leaf_begin_element(config);
	uct__text(view, "UPHONIC", PALETTE_ACCENT_ARP, TITLE_TEXT_SCALE);
	uct__text(view, "CHIPTUNE", PALETTE_TEXT, TITLE_TEXT_SCALE);
	leaf({ .size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(1) } });
	uct__text(view, view->status, PALETTE_TEXT_DIM, 1);
	uct__end();
}

static void uct__declare_preset_bar(GuiView* view)
{
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 4, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER);
	uct__text(view, "PATCH", PALETTE_TEXT_FAINT, 1);
	uct__stepper(view, 150, 12, PARAM_COUNT, view->preset_label, PALETTE_ACCENT_ARP, PARAM_COUNT, 0);
	leaf({ .size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(1) } });
	uct__reset_button(view, 36, 12, view->reset_armed ? "SURE?" : "RESET", PALETTE_ACCENT_LOFI);
	uct__end();
}

static void uct__declare_oscillator(GuiView* view)
{
	uct__begin_panel(view, 214, ROW_A_HEIGHT, "OSC", PALETTE_ACCENT_OSC);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);

	for(int i = 0; i < SYNTH_WAVE_COUNT; i++)
	{
		uct__choice(view, WAVE_BUTTON_WIDTH, WAVE_BUTTON_HEIGHT, PARAM_WAVEFORM, i, k_wave_labels[i], PALETTE_ACCENT_OSC, PARAM_COUNT, 0);
	}

	uct__end();
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 1, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__begin_stack(view, LEAF_DIRECTION_VERTICAL, 3, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__display(view, WIDGET_OSC_PREVIEW, 50, 24, PALETTE_ACCENT_OSC, PARAM_COUNT, 0);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, PRESET_GAP, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);

	for(int i = 0; i < 4; i++)
	{
		uct__preset(view, i);
	}

	uct__end();
	uct__end();

	uct__knob(view, KNOB_CELL_WIDTH, PARAM_SHAPE, "SHAPE", PALETTE_ACCENT_OSC, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_SHAPE_MOD_DEPTH, "MOD D", PALETTE_ACCENT_OSC, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_SHAPE_MOD_RATE, "MOD R", PALETTE_ACCENT_OSC, PARAM_COUNT, 0);

	uct__begin_stack(view, LEAF_DIRECTION_VERTICAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__text(view, "NOISE", PALETTE_TEXT_FAINT, 1);
	uct__choice(view, 30, SMALL_BUTTON_HEIGHT, PARAM_NOISE_MODE, SYNTH_NOISE_LONG, "LONG", PALETTE_ACCENT_OSC, PARAM_WAVEFORM, SYNTH_WAVE_NOISE);
	uct__choice(view, 30, SMALL_BUTTON_HEIGHT, PARAM_NOISE_MODE, SYNTH_NOISE_SHORT, "SHORT", PALETTE_ACCENT_OSC, PARAM_WAVEFORM, SYNTH_WAVE_NOISE);
	uct__end();

	uct__end();
	uct__end();
}

static void uct__declare_envelope(GuiView* view)
{
	uct__begin_panel(view, 170, ROW_A_HEIGHT, "ENVELOPE", PALETTE_ACCENT_ENV);
	uct__display(view, WIDGET_ENV_GRAPH, 162, 22, PALETTE_ACCENT_ENV, PARAM_COUNT, 0);

	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 0, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_ATTACK, "ATK", PALETTE_ACCENT_ENV, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_DECAY, "DEC", PALETTE_ACCENT_ENV, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_SUSTAIN, "SUS", PALETTE_ACCENT_ENV, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_RELEASE, "REL", PALETTE_ACCENT_ENV, PARAM_COUNT, 0);
	uct__end();
	uct__end();
}

static void uct__declare_modulation(GuiView* view)
{
	uct__begin_panel(view, 204, ROW_B_HEIGHT, NULL, PALETTE_ACCENT_MOD);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);

	uct__begin_stack(view, LEAF_DIRECTION_VERTICAL, 1, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__text(view, "VIBRATO", PALETTE_ACCENT_MOD, 1);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 0, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_VIBRATO_DEPTH, "DEPTH", PALETTE_ACCENT_MOD, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_VIBRATO_RATE, "RATE", PALETTE_ACCENT_MOD, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_VIBRATO_DELAY, "DELAY", PALETTE_ACCENT_MOD, PARAM_COUNT, 0);
	uct__end();
	uct__end();

	leaf({ .size = { LEAF_SIZE_FIXED(view->pixel_scale), LEAF_SIZE_FIXED(uct__px(view, ROW_B_HEIGHT - 8)) }, .color = uct__fill(PALETTE_PANEL_EDGE) });

	uct__begin_stack(view, LEAF_DIRECTION_VERTICAL, 1, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__text(view, "PITCH SWEEP", PALETTE_ACCENT_MOD, 1);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 0, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_SWEEP_AMOUNT, "PITCH", PALETTE_ACCENT_MOD, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_SWEEP_TIME, "TIME", PALETTE_ACCENT_MOD, PARAM_COUNT, 0);
	uct__end();
	uct__end();

	uct__end();
	uct__end();
}

static void uct__declare_lofi(GuiView* view)
{
	uct__begin_panel(view, 88, ROW_B_HEIGHT, "LO-FI", PALETTE_ACCENT_LOFI);

	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 0, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_BITS, "BITS", PALETTE_ACCENT_LOFI, PARAM_COUNT, 0);
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_DOWNSAMPLE, "DOWN", PALETTE_ACCENT_LOFI, PARAM_COUNT, 0);
	uct__end();
	uct__end();
}

static void uct__declare_voice(GuiView* view)
{
	uct__begin_panel(view, 88, ROW_B_HEIGHT, "VOICE", PALETTE_ACCENT_VOICE);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER);

	uct__begin_stack(view, LEAF_DIRECTION_VERTICAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__choice(view, 34, SMALL_BUTTON_HEIGHT, PARAM_VOICE_MODE, SYNTH_VOICE_POLY, "POLY", PALETTE_ACCENT_VOICE, PARAM_COUNT, 0);
	uct__choice(view, 34, SMALL_BUTTON_HEIGHT, PARAM_VOICE_MODE, SYNTH_VOICE_MONO, "MONO", PALETTE_ACCENT_VOICE, PARAM_COUNT, 0);
	uct__end();

	uct__knob(view, KNOB_CELL_WIDTH, PARAM_GLIDE, "GLIDE", PALETTE_ACCENT_VOICE, PARAM_COUNT, 0);
	uct__end();
	uct__end();
}

static void uct__declare_arpeggio(GuiView* view)
{
	uct__begin_panel(view, 172, ROW_C_HEIGHT, "ARPEGGIO", PALETTE_ACCENT_ARP);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 4, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER);
	uct__begin_stack(view, LEAF_DIRECTION_VERTICAL, 1, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);

	for(int row = 0; row < SYNTH_ARP_COUNT / 4; row++)
	{
		uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 1, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);

		for(int column = 0; column < 4; column++)
		{
			const int mode = row * 4 + column;
			uct__choice(view, 29, SMALL_BUTTON_HEIGHT, PARAM_ARP_MODE, mode, k_arp_labels[mode], PALETTE_ACCENT_ARP, PARAM_COUNT, 0);
		}

		uct__end();
	}

	uct__end();
	uct__knob(view, KNOB_CELL_WIDTH, PARAM_ARP_RATE, "RATE", PALETTE_ACCENT_ARP, PARAM_ARP_MODE, -1);

	uct__end();
	uct__end();
}

static void uct__declare_output(GuiView* view)
{
	uct__begin_panel(view, 212, ROW_C_HEIGHT, "OUTPUT", PALETTE_ACCENT_OUT);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER);
	uct__knob(view, GAIN_CELL_WIDTH, PARAM_GAIN, "GAIN", PALETTE_ACCENT_OUT, PARAM_COUNT, 0);
	uct__display(view, WIDGET_SCOPE, 156, 38, PALETTE_ACCENT_OUT, PARAM_COUNT, 0);
	uct__end();
	uct__end();
}

static void uct__declare_pcm(GuiView* view)
{
	uct__begin_panel(view, 388, ROW_D_HEIGHT, "PCM", PALETTE_ACCENT_PCM);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 4, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__begin_stack(view, LEAF_DIRECTION_VERTICAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__stepper(view, 108, 12, PARAM_PCM_SAMPLE, NULL, PALETTE_ACCENT_PCM, PARAM_WAVEFORM, SYNTH_WAVE_PCM);
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__choice(view, 53, SMALL_BUTTON_HEIGHT, PARAM_PCM_LOOP, 0, "ONE-SHOT", PALETTE_ACCENT_PCM, PARAM_WAVEFORM, SYNTH_WAVE_PCM);
	uct__choice(view, 53, SMALL_BUTTON_HEIGHT, PARAM_PCM_LOOP, 1, "LOOP", PALETTE_ACCENT_PCM, PARAM_WAVEFORM, SYNTH_WAVE_PCM);
	uct__end();

	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, 2, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
	uct__choice(view, 53, SMALL_BUTTON_HEIGHT, PARAM_PCM_PITCH, 0, "FIXED", PALETTE_ACCENT_PCM, PARAM_WAVEFORM, SYNTH_WAVE_PCM);
	uct__choice(view, 53, SMALL_BUTTON_HEIGHT, PARAM_PCM_PITCH, 1, "KEYS", PALETTE_ACCENT_PCM, PARAM_WAVEFORM, SYNTH_WAVE_PCM);
	uct__end();
	uct__end();

	uct__knob(view, KNOB_CELL_WIDTH, PARAM_PCM_TUNE, "TUNE", PALETTE_ACCENT_PCM, PARAM_WAVEFORM, SYNTH_WAVE_PCM);
	uct__display(view, WIDGET_PCM_DISPLAY, 222, 40, PALETTE_ACCENT_PCM, PARAM_WAVEFORM, SYNTH_WAVE_PCM);
	uct__end();
	uct__end();
}

static void uct__declare_keyboard(GuiView* view)
{
	Leaf_ElementConfig config =
	{
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(uct__px(view, KEYBOARD_STRIP_HEIGHT)) },
		.color = uct__fill(PALETTE_LCD),
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER },
	};

	leaf_begin_element(config);
	uct__display(view, WIDGET_KEYBOARD, LOGICAL_KEYBOARD_WIDTH, KEYBOARD_HEIGHT, PALETTE_ACCENT_ARP, PARAM_COUNT, 0);
	uct__end();
}

static void uct__declare_preset_bar_row(GuiView* view)
{
	Leaf_ElementConfig config =
	{
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIXED(uct__px(view, PRESET_BAR_HEIGHT)) },
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
	};

	leaf_begin_element(config);
	uct__declare_preset_bar(view);
	uct__end();
}

static void uct__begin_row(const GuiView* view)
{
	uct__begin_stack(view, LEAF_DIRECTION_HORIZONTAL, ROW_GAP, LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP);
}

static void uct__compose_status(GuiView* view)
{
	const int focus = view->active_widget >= 0 ? view->active_widget : view->hover_widget;
	if(focus >= 0 && focus < view->hit_count && view->widgets[focus].kind == WIDGET_KNOB)
	{
		const GuiWidget* widget = &view->widgets[focus];
		char name[VALUE_TEXT_SIZE];
		char value[VALUE_TEXT_SIZE];

		uct__to_upper(params_info(widget->param)->name, name, sizeof(name));
		params_format(view->plugin, widget->param, params_get(view->plugin, widget->param), value, sizeof(value));
		snprintf(view->status, sizeof(view->status), "%s %s", name, value);
		return;
	}

	snprintf(view->status, sizeof(view->status), "VOICES %02u/%02d", view->voice_count, SYNTH_MAX_VOICES);
}

static void uct__declare_ui(GuiView* view)
{
	view->widget_count = 0;
	Leaf_ElementConfig root =
	{
		.id = leaf_id(ROOT_ID_LABEL),
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_GROW },
		.padding = { uct__px(view, SIDE_MARGIN), uct__px(view, SIDE_MARGIN), 0, 0 },
		.color = uct__fill(PALETTE_BG),
		.child_gap = uct__px(view, SECTION_GAP),
	};

	leaf_begin_element(root);
	uct__declare_title_bar(view);
	uct__declare_preset_bar_row(view);

	uct__begin_row(view);
	uct__declare_oscillator(view);
	uct__declare_envelope(view);
	uct__end();

	uct__begin_row(view);
	uct__declare_modulation(view);
	uct__declare_lofi(view);
	uct__declare_voice(view);
	uct__end();

	uct__begin_row(view);
	uct__declare_arpeggio(view);
	uct__declare_output(view);
	uct__end();

	uct__declare_pcm(view);
	uct__declare_keyboard(view);
	uct__end();

	view->hit_count = view->widget_count;
}

static void uct__poll_state(GuiView* view)
{
	synth_scope_copy(view->plugin->synth, view->scope, SCOPE_SAMPLES);
	float peak = 0.0f;
	for(int i = 0; i < SCOPE_SAMPLES; i++)
	{
		peak = fmaxf(peak, fabsf(view->scope[i]));
	}

	view->scope_active = peak > SCOPE_SILENCE;
	synth_active_keys(view->plugin->synth, &view->keys_low, &view->keys_high);
	view->voice_count = synth_active_voice_count(view->plugin->synth);
}

static bool uct__widget_box(const GuiView* view, int index, Leaf_BoundingBox* out)
{
	if(index < 0 || index >= view->hit_count)
		return false;

	*out = leaf_get_bounding_box(uct__widget_id(index));
	return true;
}

static int uct__find_widget(const GuiView* view, int x, int y)
{
	for(int i = 0; i < view->hit_count; i++)
	{
		if(view->widgets[i].kind == WIDGET_TEXT)
			continue;

		Leaf_BoundingBox box;
		if(!uct__widget_box(view, i, &box))
			continue;

		if(x >= box.x && y >= box.y && x < box.x + box.width && y < box.y + box.height)
			return i;
	}

	return -1;
}

static void uct__set_param_gesture(GuiView* view, ParamId param, double value)
{
	params_edit_begin(view->plugin, param);
	params_edit_set(view->plugin, param, value);
	params_edit_end(view->plugin, param);
}

static void uct__release_key(GuiView* view)
{
	if(view->pressed_key < 0)
		return;

	plugin_gui_note(view->plugin, false, view->pressed_key);
	view->pressed_key = -1;
}

static void uct__press_key(GuiView* view, int key)
{
	if(key == view->pressed_key)
		return;

	uct__release_key(view);

	if(key < 0)
		return;

	view->pressed_key = key;
	plugin_gui_note(view->plugin, true, key);
}

static int uct__key_under_pointer(const GuiView* view, int index, int x, int y)
{
	Leaf_BoundingBox box;
	if(!uct__widget_box(view, index, &box))
		return -1;

	return uct__key_at((int)((x - box.x) / view->pixel_scale), (int)((y - box.y) / view->pixel_scale));
}

static void uct__drag_knob(GuiView* view, int y, bool fine)
{
	const GuiWidget* widget = &view->widgets[view->active_widget];
	double sensitivity = 1.0 / KNOB_DRAG_PIXELS;

	if(fine)
		sensitivity *= KNOB_FINE_FACTOR;

	const double travelled = (double)(view->drag_last_y - y) / view->pixel_scale;
	view->drag_last_y = y;
	view->drag_position += travelled * sensitivity;

	if(view->drag_position < 0.0)
		view->drag_position = 0.0;

	if(view->drag_position > 1.0)
		view->drag_position = 1.0;

	const double value = params_from_position(widget->param, view->drag_position);
	if(value == params_get(view->plugin, widget->param))
		return;

	params_edit_set(view->plugin, widget->param, value);
}

static void uct__wheel_knob(GuiView* view, const GuiWidget* widget, int notches, bool fine)
{
	const double current = params_get(view->plugin, widget->param);
	double value = 0.0;

	if(params_is_stepped(widget->param))
		value = current + (notches > 0 ? 1.0 : -1.0);
	else
	{
		const double step = fine ? KNOB_WHEEL_STEP * KNOB_FINE_FACTOR : KNOB_WHEEL_STEP;
		value = params_from_position(widget->param, params_to_position(widget->param, current) + notches * step);
	}

	value = params_clamp(widget->param, value);
	if(value == current)
		return;

	uct__set_param_gesture(view, widget->param, value);
}

static void uct__apply_preset(GuiView* view, int index)
{
	const PresetList* list = view->plugin->presets;
	if(index < 0 || index >= list->count)
		return;

	const PresetEntry* entry = &list->entries[index];
	for(int i = 0; i < PARAM_COUNT; i++)
	{
		uct__set_param_gesture(view, (ParamId)i, entry->values[i]);
	}

	const int sample = presets_resolve_sample(entry, view->plugin->pcm_bank);
	if(sample >= 0)
		uct__set_param_gesture(view, PARAM_PCM_SAMPLE, sample);

	view->preset_index = index;
}

static void uct__step_preset(GuiView* view, int direction)
{
	int next = view->preset_index + direction;
	if(next < 0)
		next = 0;

	if(next > view->plugin->presets->count - 1)
		next = view->plugin->presets->count - 1;

	uct__apply_preset(view, next);
}

static bool uct__preset_modified(const GuiView* view)
{
	const PresetList* list = view->plugin->presets;
	if(view->preset_index >= list->count)
		return false;

	const PresetEntry* entry = &list->entries[view->preset_index];

	for(int i = 0; i < PARAM_COUNT; i++)
	{
		double wanted = entry->values[i];
		if(i == PARAM_PCM_SAMPLE)
		{
			const int sample = presets_resolve_sample(entry, view->plugin->pcm_bank);
			if(sample >= 0)
				wanted = sample;
		}

		if(fabs(params_get(view->plugin, (ParamId)i) - wanted) > PRESET_VALUE_TOLERANCE)
			return true;
	}

	return false;
}

static void uct__compose_preset_label(GuiView* view)
{
	const PresetList* list = view->plugin->presets;
	if(view->preset_index >= list->count)
		view->preset_index = list->count - 1;

	snprintf(view->preset_label, sizeof(view->preset_label), "%s%s", list->entries[view->preset_index].name, uct__preset_modified(view) ? "*" : "");
}

static void uct__reset_all(GuiView* view)
{
	for(int i = 0; i < PARAM_COUNT; i++)
	{
		uct__set_param_gesture(view, (ParamId)i, params_info((ParamId)i)->default_value);
	}

	view->preset_index = 0;
}

static void uct__step_sample(GuiView* view, const GuiWidget* widget, int direction)
{
	const int count = view->plugin->pcm_bank->count;
	const int current = (int)params_get(view->plugin, widget->param);
	int next = current + direction;

	if(next < 0)
		next = 0;

	if(next > count - 1)
		next = count - 1;

	if(next == current)
		return;

	uct__set_param_gesture(view, widget->param, next);
}

static void uct__click_widget(GuiView* view, int index, int x, int y)
{
	const GuiWidget* widget = &view->widgets[index];
	if(widget->kind == WIDGET_RESET)
	{
		if(view->reset_armed)
			uct__reset_all(view);

		view->reset_armed = !view->reset_armed;
		return;
	}

	if(widget->kind == WIDGET_STEPPER)
	{
		Leaf_BoundingBox box;
		if(!uct__widget_box(view, index, &box))
			return;

		const bool left = (x - box.x) / view->pixel_scale < widget->width / 2;
		if(widget->param == PARAM_COUNT)
			uct__step_preset(view, left ? -1 : 1);
		else
			uct__step_sample(view, widget, left ? -1 : 1);

		return;
	}

	if(widget->kind == WIDGET_CHOICE || widget->kind == WIDGET_PRESET)
	{
		uct__set_param_gesture(view, widget->param, widget->value);
		return;
	}

	if(widget->kind == WIDGET_KEYBOARD)
	{
		view->active_widget = index;
		uct__press_key(view, uct__key_under_pointer(view, index, x, y));
		return;
	}

	if(widget->kind != WIDGET_KNOB)
		return;

	if(upl_mouse_double_clicked(UPL_MOUSE_LEFT))
	{
		uct__set_param_gesture(view, widget->param, params_info(widget->param)->default_value);
		return;
	}

	view->active_widget = index;
	view->drag_last_y = y;
	view->drag_position = params_to_position(widget->param, params_get(view->plugin, widget->param));
	params_edit_begin(view->plugin, widget->param);
}

static void uct__end_drag(GuiView* view)
{
	if(view->active_widget < 0)
		return;

	const GuiWidget* widget = &view->widgets[view->active_widget];
	if(widget->kind == WIDGET_KNOB)
		params_edit_end(view->plugin, widget->param);

	if(widget->kind == WIDGET_KEYBOARD)
		uct__release_key(view);

	view->active_widget = -1;
}

static void uct__handle_input(GuiView* view)
{
	const int x = upl_mouse_x();
	const int y = upl_mouse_y();
	const bool fine = upl_shift_down();

	if(view->active_widget < 0)
		view->hover_widget = uct__find_widget(view, x, y);

	if(view->hover_widget < 0 || view->widgets[view->hover_widget].kind != WIDGET_RESET)
		view->reset_armed = false;

	if(upl_mouse_pressed(UPL_MOUSE_LEFT) && view->hover_widget >= 0)
		uct__click_widget(view, view->hover_widget, x, y);

	if(view->active_widget >= 0)
	{
		const WidgetKind kind = view->widgets[view->active_widget].kind;

		if(kind == WIDGET_KNOB)
			uct__drag_knob(view, y, fine);

		if(kind == WIDGET_KEYBOARD)
			uct__press_key(view, uct__key_under_pointer(view, view->active_widget, x, y));
	}

	if(upl_mouse_released(UPL_MOUSE_LEFT))
		uct__end_drag(view);

	const int scroll = uct__wheel_notches();
	if(scroll != 0 && view->active_widget < 0 && view->hover_widget >= 0)
	{
		const GuiWidget* hovered = &view->widgets[view->hover_widget];
		if(hovered->kind == WIDGET_KNOB)
			uct__wheel_knob(view, hovered, scroll, fine);

		if(hovered->kind == WIDGET_STEPPER && hovered->param == PARAM_COUNT)
			uct__step_preset(view, scroll > 0 ? 1 : -1);

		if(hovered->kind == WIDGET_STEPPER && hovered->param != PARAM_COUNT)
			uct__step_sample(view, hovered, scroll > 0 ? 1 : -1);
	}
}

#pragma endregion

#pragma region Public API

GuiView* gui_view_create(Plugin* plugin)
{
	GuiView* view = (GuiView*)calloc(1, sizeof(GuiView));
	if(view == NULL)
		return NULL;

	view->plugin = plugin;
	view->pixel_scale = GUI_BASE_PIXEL_SCALE;
	view->hover_widget = -1;
	view->active_widget = -1;
	view->pressed_key = -1;
	return view;
}

void gui_view_destroy(GuiView* view)
{
	if(view == NULL)
		return;

	uct__end_drag(view);
	uct__release_key(view);
	free(view);
}

void gui_view_frame(GuiView* view)
{
	view->pixel_scale = uct__pixel_scale();
	uct__poll_state(view);
	uct__handle_input(view);
	uct__compose_status(view);
	uct__compose_preset_label(view);
	uct__declare_ui(view);
	upl_redraw();
}

#pragma endregion
