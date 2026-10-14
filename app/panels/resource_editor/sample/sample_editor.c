#define UPH_SAMPLE_EDITOR_TOOLBAR_FIELD_GAP		16
#define UPH_SAMPLE_EDITOR_RULER_HEIGHT			24
#define UPH_SAMPLE_EDITOR_LABEL_SPACING			100.0f
#define UPH_SAMPLE_EDITOR_LANE_PADDING			4.0f

#define UPH_SAMPLE_EDITOR_BIN_FRAMES			128
#define UPH_SAMPLE_EDITOR_MAX_CHANNELS			2

#define UPH_SAMPLE_EDITOR_MIN_FRAMES_PER_PIXEL	(1.0 / 32.0)
#define UPH_SAMPLE_EDITOR_ZOOM_SPEED			0.15
#define UPH_SAMPLE_EDITOR_SHIFT_SCROLL_SPEED	0.1

#define UPH_SAMPLE_EDITOR_STEM_MIN_PIXELS		6.0f
#define UPH_SAMPLE_EDITOR_DOT_SIZE				4.0f

typedef struct
{
	float min;
	float max;
}
Uph_SamplePeakBin;

typedef struct
{
	Uph_SamplePeakBin *bins[UPH_SAMPLE_EDITOR_MAX_CHANNELS];
	uint64_t bin_count;
	uint64_t bin_capacity;
	uint64_t frame_count;
	const float *frames;
	Uph_ResourceIndex data_index;
	uint32_t channels;
	bool valid;
}
Uph_SamplePeakCache;

typedef struct
{
	const float *frames;
	uint64_t frame_count;
	uint32_t channels;
	uint32_t sample_rate;
	bool valid;
}
Uph_SampleEditorSource;

typedef struct
{
	Uph_SamplePeakCache cache;
	Uph_SampleEditorSource source;

	double start_frame;
	double frames_per_pixel;
	Naui_Vec2 pan_last_mouse;
	Leaf_BoundingBox wave_box;

	Uph_ResourceIndex last_sample;
	bool has_last_sample;
	bool fit;
	bool panning;
	bool wave_hovered;
}
Uph_SampleEditorData;

static Uph_SampleEditorData uph_sample_editor_data;

#pragma region Peak cache

static void _uph_sample_editor_cache_free(Uph_SamplePeakCache *cache)
{
	for (uint32_t c = 0; c < UPH_SAMPLE_EDITOR_MAX_CHANNELS; c++)
		free(cache->bins[c]);

	memset(cache, 0, sizeof(*cache));
}

static void _uph_sample_editor_cache_update(Uph_SamplePeakCache *cache, const Uph_SampleEditorSource *source, Uph_ResourceIndex data_index)
{
	const bool rebuild = !cache->valid
		|| cache->data_index != data_index
		|| cache->frames != source->frames
		|| cache->channels != source->channels
		|| source->frame_count < cache->frame_count;

	if (!rebuild && cache->frame_count == source->frame_count)
		return;

	if (rebuild)
		_uph_sample_editor_cache_free(cache);

	const uint64_t bin_count = (source->frame_count + UPH_SAMPLE_EDITOR_BIN_FRAMES - 1) / UPH_SAMPLE_EDITOR_BIN_FRAMES;
	if (bin_count > cache->bin_capacity)
	{
		uint64_t capacity = cache->bin_capacity * 2;
		if (capacity < bin_count)
			capacity = bin_count;

		for (uint32_t c = 0; c < source->channels; c++)
		{
			Uph_SamplePeakBin *grown = (Uph_SamplePeakBin*)realloc(cache->bins[c], (size_t)capacity * sizeof(Uph_SamplePeakBin));
			if (!grown)
			{
				_uph_sample_editor_cache_free(cache);
				return;
			}
			cache->bins[c] = grown;
		}
		cache->bin_capacity = capacity;
	}

	const uint64_t first_bin = rebuild ? 0 : cache->frame_count / UPH_SAMPLE_EDITOR_BIN_FRAMES;
	for (uint64_t bin = first_bin; bin < bin_count; bin++)
	{
		const uint64_t frame_start = bin * UPH_SAMPLE_EDITOR_BIN_FRAMES;
		uint64_t frame_end = frame_start + UPH_SAMPLE_EDITOR_BIN_FRAMES;
		if (frame_end > source->frame_count)
			frame_end = source->frame_count;

		for (uint32_t c = 0; c < source->channels; c++)
		{
			float min_v = source->frames[frame_start * source->channels + c];
			float max_v = min_v;
			for (uint64_t f = frame_start + 1; f < frame_end; f++)
			{
				const float v = source->frames[f * source->channels + c];
				if (v < min_v) min_v = v;
				if (v > max_v) max_v = v;
			}
			cache->bins[c][bin] = (Uph_SamplePeakBin) { min_v, max_v };
		}
	}

	cache->bin_count = bin_count;
	cache->frame_count = source->frame_count;
	cache->frames = source->frames;
	cache->data_index = data_index;
	cache->channels = source->channels;
	cache->valid = true;
}

#pragma endregion

#pragma region View

static void _uph_sample_editor_view_resolve(Uph_SampleEditorData *editor, double width)
{
	const Uph_SampleEditorSource *source = &editor->source;
	if (!source->valid || width < 1.0)
		return;

	const double total = (double)source->frame_count;
	double max_fpp = total / width;
	if (max_fpp < UPH_SAMPLE_EDITOR_MIN_FRAMES_PER_PIXEL)
		max_fpp = UPH_SAMPLE_EDITOR_MIN_FRAMES_PER_PIXEL;

	if (editor->fit || editor->frames_per_pixel <= 0.0)
	{
		editor->fit = true;
		editor->frames_per_pixel = max_fpp;
		editor->start_frame = 0.0;
		return;
	}

	editor->frames_per_pixel = NAUI_CLAMP(editor->frames_per_pixel, UPH_SAMPLE_EDITOR_MIN_FRAMES_PER_PIXEL, max_fpp);
	if (editor->frames_per_pixel >= max_fpp)
	{
		editor->fit = true;
		editor->start_frame = 0.0;
		return;
	}

	const double max_start = fmax(0.0, total - width * editor->frames_per_pixel);
	editor->start_frame = NAUI_CLAMP(editor->start_frame, 0.0, max_start);
}

static inline float _uph_sample_editor_frame_to_x(const Uph_SampleEditorData *editor, Leaf_BoundingBox box, double frame)
{
	return box.x + (float)((frame - editor->start_frame) / editor->frames_per_pixel);
}

static void _uph_sample_editor_update_input(Uph_SampleEditorData *editor, Leaf_BoundingBox box)
{
	if (!editor->source.valid || box.width < 1.0f)
		return;

	_uph_sample_editor_view_resolve(editor, box.width);

	const bool hovered = editor->wave_hovered;
	const float wheel = (float)naui_mouse_scroll_delta();

	if (hovered && wheel != 0.0f)
	{
		if (naui_key_down(NAUI_KEY_LSHIFT))
		{
			const double visible = (double)box.width * editor->frames_per_pixel;
			editor->start_frame -= (double)wheel * visible * UPH_SAMPLE_EDITOR_SHIFT_SCROLL_SPEED;
		}
		else
		{
			const double mouse_px = (double)naui_mouse_x() - (double)box.x;
			const double anchor = editor->start_frame + mouse_px * editor->frames_per_pixel;
			const double new_fpp = editor->frames_per_pixel * pow(1.0 - UPH_SAMPLE_EDITOR_ZOOM_SPEED, (double)wheel);

			editor->fit = false;
			editor->frames_per_pixel = new_fpp;
			editor->start_frame = anchor - mouse_px * new_fpp;
		}
	}

	if (hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
		editor->fit = true;

	if (hovered && naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
	{
		editor->panning = true;
		editor->pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	}

	if (editor->panning)
	{
		const Naui_Vec2 current = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
		editor->start_frame -= (double)(current.x - editor->pan_last_mouse.x) * editor->frames_per_pixel;
		editor->pan_last_mouse = current;

		if (naui_mouse_released(NAUI_MOUSE_MIDDLE) || !naui_mouse_down(NAUI_MOUSE_MIDDLE))
			editor->panning = false;
	}

	_uph_sample_editor_view_resolve(editor, box.width);
}

#pragma endregion

#pragma region Formatting

static void _uph_sample_editor_format_time(char *out, size_t size, double seconds, int32_t decimals)
{
	if (seconds < 0.0)
		seconds = 0.0;
	decimals = NAUI_CLAMP(decimals, 0, 6);

	int64_t scale = 1;
	for (int32_t i = 0; i < decimals; i++)
		scale *= 10;

	const int64_t units = (int64_t)llround(seconds * (double)scale);
	const int64_t whole = units / scale;
	const int64_t fraction = units % scale;
	const int64_t minutes = whole / 60;
	const int64_t secs = whole % 60;

	if (decimals > 0)
		snprintf(out, size, "%lld:%02lld.%0*lld", (long long)minutes, (long long)secs, (int)decimals, (long long)fraction);
	else
		snprintf(out, size, "%lld:%02lld", (long long)minutes, (long long)secs);
}

static double _uph_sample_editor_major_step(double pixels_per_second)
{
	const double min_seconds = (double)NAUI_DPI(UPH_SAMPLE_EDITOR_LABEL_SPACING) / pixels_per_second;
	const double magnitude = pow(10.0, floor(log10(min_seconds)));

	static const double mantissas[] = { 1.0, 2.0, 5.0, 10.0 };
	for (uint32_t i = 0; i < 4; i++)
	{
		if (magnitude * mantissas[i] >= min_seconds)
			return magnitude * mantissas[i];
	}
	return magnitude * 10.0;
}

static int32_t _uph_sample_editor_step_decimals(double step)
{
	if (step >= 1.0 - 1e-9)
		return 0;
	return (int32_t)ceil(-log10(step) - 1e-9);
}

static uint32_t _uph_sample_editor_step_subdivisions(double step)
{
	const double magnitude = pow(10.0, floor(log10(step) + 1e-9));
	return lround(step / magnitude) == 2 ? 4 : 5;
}

static const char *_uph_sample_editor_file_name(const char *path)
{
	const char *name = path;
	for (const char *c = path; *c; c++)
	{
		if (*c == '/' || *c == '\\')
			name = c + 1;
	}
	return name;
}

#pragma endregion

#pragma region Draw callbacks

static void _uph_sample_editor_ruler_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	const Uph_SampleEditorSource *source = &editor->source;
	if (!source->valid)
		return;

	_uph_sample_editor_view_resolve(editor, box.width);

	const Leaf_Color beat_color = naui_theme_color("uph_timeline_top_ruler_grid_beat_color");
	const Leaf_Color bar_color = naui_theme_color("uph_timeline_top_ruler_grid_bar_color");
	const Leaf_Color number_color = naui_theme_color("uph_timeline_top_ruler_grid_text_color");

	const double sample_rate = (double)source->sample_rate;
	const double fpp = editor->frames_per_pixel;
	const double step = _uph_sample_editor_major_step(sample_rate / fpp);
	const uint32_t subdivisions = _uph_sample_editor_step_subdivisions(step);
	const double minor_step = step / (double)subdivisions;
	const int32_t decimals = _uph_sample_editor_step_decimals(step);
	const bool draw_minor = (minor_step * sample_rate / fpp) >= 4.0;

	const double time_start = editor->start_frame / sample_rate;
	const double time_end = time_start + (double)box.width * fpp / sample_rate;
	const int64_t first = (int64_t)floor(time_start / minor_step);
	const int64_t last = (int64_t)ceil(time_end / minor_step);

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	for (int64_t i = first; i <= last; i++)
	{
		const bool is_major = (i % (int64_t)subdivisions) == 0;
		if (!is_major && !draw_minor)
			continue;

		const double time = (double)i * minor_step;
		const float x = _uph_sample_editor_frame_to_x(editor, box, time * sample_rate);

		naui_draw_line(
			(Naui_Vec2) { x, box.y + box.height * (is_major ? 0.4f : 0.7f) },
			(Naui_Vec2) { x, box.y + box.height },
			is_major ? bar_color : beat_color,
			1.0f
		);

		if (is_major)
		{
			char label[32];
			_uph_sample_editor_format_time(label, sizeof(label), time, decimals);
			naui_draw_text((Naui_Vec2) { x + 4.0f, box.y + box.height * 0.2f }, label, NAUI_DPI(12.0f), 0, number_color);
		}
	}
	naui_pop_clip_rect();
}

static void _uph_sample_editor_draw_grid(const Uph_SampleEditorData *editor, Leaf_BoundingBox box)
{
	const Uph_SampleEditorSource *source = &editor->source;
	const Leaf_Color major_color = naui_theme_color("uph_timeline_grid_bar_color");
	const Leaf_Color minor_color = naui_theme_color("uph_timeline_grid_subbeat_color");

	const double sample_rate = (double)source->sample_rate;
	const double fpp = editor->frames_per_pixel;
	const double step = _uph_sample_editor_major_step(sample_rate / fpp);
	const uint32_t subdivisions = _uph_sample_editor_step_subdivisions(step);
	const double minor_step = step / (double)subdivisions;
	const bool draw_minor = (minor_step * sample_rate / fpp) >= 6.0;

	const double time_start = editor->start_frame / sample_rate;
	const double time_end = time_start + (double)box.width * fpp / sample_rate;
	const int64_t first = (int64_t)floor(time_start / minor_step);
	const int64_t last = (int64_t)ceil(time_end / minor_step);

	for (int64_t i = first; i <= last; i++)
	{
		const bool is_major = (i % (int64_t)subdivisions) == 0;
		if (!is_major && !draw_minor)
			continue;

		const float x = _uph_sample_editor_frame_to_x(editor, box, (double)i * minor_step * sample_rate);
		naui_draw_line((Naui_Vec2) { x, box.y }, (Naui_Vec2) { x, box.y + box.height }, is_major ? major_color : minor_color, 1.0f);
	}
}

static inline float _uph_sample_editor_clamp_amplitude(float v)
{
	return v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
}

static void _uph_sample_editor_draw_channel(const Uph_SampleEditorData *editor, uint32_t channel, Leaf_BoundingBox lane, Leaf_Color color)
{
	const Uph_SampleEditorSource *source = &editor->source;
	const Uph_SamplePeakCache *cache = &editor->cache;

	const double fpp = editor->frames_per_pixel;
	const double start = editor->start_frame;
	const uint64_t total = source->frame_count;
	const uint32_t channels = source->channels;

	const float padding = NAUI_DPI(UPH_SAMPLE_EDITOR_LANE_PADDING);
	const float mid_y = lane.y + lane.height * 0.5f;
	const float half = fmaxf(1.0f, (lane.height - padding * 2.0f) * 0.5f);

	if (fpp <= 1.0)
	{
		const float pixels_per_frame = (float)(1.0 / fpp);
		const bool draw_stems = pixels_per_frame >= NAUI_DPI(UPH_SAMPLE_EDITOR_STEM_MIN_PIXELS);

		Leaf_Color stem_color = color;
		stem_color.a = (uint8_t)(color.a / 3);

		int64_t first = (int64_t)floor(start) - 1;
		int64_t last = (int64_t)ceil(start + (double)lane.width * fpp) + 1;
		if (first < 0) first = 0;
		if (last > (int64_t)total - 1) last = (int64_t)total - 1;

		Naui_Vec2 previous = { 0.0f, 0.0f };
		for (int64_t i = first; i <= last; i++)
		{
			const float v = _uph_sample_editor_clamp_amplitude(source->frames[(uint64_t)i * channels + channel]);
			const Naui_Vec2 point = { _uph_sample_editor_frame_to_x(editor, lane, (double)i), mid_y - v * half };

			if (draw_stems)
				naui_draw_line((Naui_Vec2) { point.x, mid_y }, point, stem_color, 1.0f);

			if (i > first)
				naui_draw_line(previous, point, color, 1.0f);

			if (draw_stems)
			{
				const float dot = NAUI_DPI(UPH_SAMPLE_EDITOR_DOT_SIZE);
				naui_fill_rect((Naui_Vec2) { point.x - dot * 0.5f, point.y - dot * 0.5f }, (Naui_Vec2) { dot, dot }, color, 0.0f, NAUI_CORNER_NONE);
			}

			previous = point;
		}

		return;
	}

	const bool use_bins = fpp >= (double)UPH_SAMPLE_EDITOR_BIN_FRAMES && cache->valid;
	const uint64_t scan_stride = (!use_bins && fpp > 256.0) ? (uint64_t)(fpp / 256.0) : 1;

	const int32_t width = (int32_t)ceilf(lane.width);
	float previous_min = 0.0f;
	float previous_max = 0.0f;
	bool has_previous = false;

	for (int32_t x = 0; x < width; x++)
	{
		const double frame_start = start + (double)x * fpp;
		if (frame_start >= (double)total)
			break;

		double frame_end = frame_start + fpp;
		if (frame_end > (double)total)
			frame_end = (double)total;

		float min_v = FLT_MAX;
		float max_v = -FLT_MAX;

		if (use_bins)
		{
			uint64_t bin_start = (uint64_t)(frame_start / UPH_SAMPLE_EDITOR_BIN_FRAMES);
			uint64_t bin_end = (uint64_t)ceil(frame_end / UPH_SAMPLE_EDITOR_BIN_FRAMES);
			if (bin_end <= bin_start) bin_end = bin_start + 1;
			if (bin_end > cache->bin_count) bin_end = cache->bin_count;

			for (uint64_t b = bin_start; b < bin_end; b++)
			{
				if (cache->bins[channel][b].min < min_v) min_v = cache->bins[channel][b].min;
				if (cache->bins[channel][b].max > max_v) max_v = cache->bins[channel][b].max;
			}
		}
		else
		{
			uint64_t i_start = (uint64_t)frame_start;
			uint64_t i_end = (uint64_t)ceil(frame_end);
			if (i_end <= i_start) i_end = i_start + 1;
			if (i_end > total) i_end = total;

			for (uint64_t i = i_start; i < i_end; i += scan_stride)
			{
				const float v = source->frames[i * channels + channel];
				if (v < min_v) min_v = v;
				if (v > max_v) max_v = v;
			}
		}

		if (max_v < min_v)
			continue;

		min_v = _uph_sample_editor_clamp_amplitude(min_v);
		max_v = _uph_sample_editor_clamp_amplitude(max_v);

		const float raw_min = min_v;
		const float raw_max = max_v;

		if (has_previous)
		{
			if (min_v > previous_max) min_v = previous_max;
			if (max_v < previous_min) max_v = previous_min;
		}
		previous_min = raw_min;
		previous_max = raw_max;
		has_previous = true;

		float top = mid_y - max_v * half;
		float bottom = mid_y - min_v * half;
		if (bottom - top < 1.0f)
		{
			const float center = (top + bottom) * 0.5f;
			top = center - 0.5f;
			bottom = center + 0.5f;
		}

		const float px = lane.x + (float)x + 0.5f;
		naui_draw_line((Naui_Vec2) { px, top }, (Naui_Vec2) { px, bottom }, color, 1.0f);
	}
}

static void _uph_sample_editor_draw_readout(const Uph_SampleEditorData *editor, Leaf_BoundingBox box)
{
	const Uph_SampleEditorSource *source = &editor->source;

	const float mouse_x = (float)naui_mouse_x();
	double frame = editor->start_frame + (double)(mouse_x - box.x) * editor->frames_per_pixel;
	frame = NAUI_CLAMP(frame, 0.0, (double)(source->frame_count - 1));
	const uint64_t frame_index = (uint64_t)frame;

	char time[32];
	_uph_sample_editor_format_time(time, sizeof(time), (double)frame_index / (double)source->sample_rate, 4);

	char text[160];
	int32_t length = snprintf(text, sizeof(text), "%s  |  frame %llu", time, (unsigned long long)frame_index);
	if (source->channels == 1)
	{
		length += snprintf(text + length, sizeof(text) - (size_t)length, "  |  %+.4f", source->frames[frame_index]);
	}
	else
	{
		static const char *channel_names[UPH_SAMPLE_EDITOR_MAX_CHANNELS] = { "L", "R" };
		for (uint32_t c = 0; c < source->channels && (size_t)length < sizeof(text); c++)
		{
			length += snprintf(text + length, sizeof(text) - (size_t)length, "  |  %s %+.4f", channel_names[c], source->frames[frame_index * source->channels + c]);
		}
	}

	const float font_size = NAUI_DPI(12.0f);
	const float padding = NAUI_DPI(5.0f);
	const Naui_Vec2 text_size = naui_measure_text(text, (uint32_t)strlen(text), font_size, 0);

	Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	Leaf_Color cursor_color = text_color;
	cursor_color.a = (uint8_t)(text_color.a / 3);

	naui_draw_line((Naui_Vec2) { mouse_x, box.y }, (Naui_Vec2) { mouse_x, box.y + box.height }, cursor_color, 1.0f);

	Leaf_Color background = naui_theme_color("uph_toolbox_bg_color");
	background.a = 230;
	naui_fill_rect(
		(Naui_Vec2) { box.x + padding, box.y + padding },
		(Naui_Vec2) { text_size.x + padding * 2.0f, text_size.y + padding * 2.0f },
		background,
		NAUI_DPI(3.0f),
		NAUI_CORNER_ALL
	);
	naui_draw_text((Naui_Vec2) { box.x + padding * 2.0f, box.y + padding * 2.0f }, text, font_size, 0, text_color);
}

static void _uph_sample_editor_wave_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	const Uph_SampleEditorSource *source = &editor->source;
	if (!source->valid)
		return;

	_uph_sample_editor_view_resolve(editor, box.width);

	const uint32_t channels = source->channels;
	const float lane_height = box.height / (float)channels;
	Leaf_Color accent_color = naui_theme_color("uph_resource_list_item_fg_color");
	accent_color.a = 255;
	const Leaf_Color guide_color = naui_theme_color("uph_timeline_grid_subbeat_color");
	const Leaf_Color zero_color = naui_theme_color("uph_timeline_grid_beat_color");
	const Leaf_Color label_color = naui_theme_color("uph_ui_text_disabled_color");

	naui_push_clip_rect(box.x, box.y, box.width, box.height);

	for (uint32_t c = 0; c < channels; c++)
	{
		naui_fill_rect(
			(Naui_Vec2) { box.x, box.y + lane_height * (float)c },
			(Naui_Vec2) { box.width, lane_height },
			naui_theme_color((c % 2) == 0 ? "uph_timeline_row_bg1_color" : "uph_timeline_row_bg2_color"),
			0.0f,
			NAUI_CORNER_NONE
		);
	}

	_uph_sample_editor_draw_grid(editor, box);

	const float padding = NAUI_DPI(UPH_SAMPLE_EDITOR_LANE_PADDING);
	for (uint32_t c = 0; c < channels; c++)
	{
		const Leaf_BoundingBox lane = { box.x, box.y + lane_height * (float)c, box.width, lane_height };
		const float mid_y = lane.y + lane.height * 0.5f;
		const float half = fmaxf(1.0f, (lane.height - padding * 2.0f) * 0.5f);

		naui_draw_line((Naui_Vec2) { lane.x, mid_y - half * 0.5f }, (Naui_Vec2) { lane.x + lane.width, mid_y - half * 0.5f }, guide_color, 1.0f);
		naui_draw_line((Naui_Vec2) { lane.x, mid_y + half * 0.5f }, (Naui_Vec2) { lane.x + lane.width, mid_y + half * 0.5f }, guide_color, 1.0f);
		naui_draw_line((Naui_Vec2) { lane.x, mid_y }, (Naui_Vec2) { lane.x + lane.width, mid_y }, zero_color, 1.0f);

		_uph_sample_editor_draw_channel(editor, c, lane, accent_color);

		const char *label = channels == 1 ? "M" : (c == 0 ? "L" : "R");
		naui_draw_text((Naui_Vec2) { lane.x + NAUI_DPI(6.0f), lane.y + NAUI_DPI(4.0f) }, label, NAUI_DPI(12.0f), 0, label_color);

		if (c > 0)
			naui_draw_line((Naui_Vec2) { lane.x, lane.y }, (Naui_Vec2) { lane.x + lane.width, lane.y }, zero_color, 1.0f);
	}

	const float end_x = _uph_sample_editor_frame_to_x(editor, box, (double)source->frame_count);
	if (end_x < box.x + box.width)
	{
		const float shade_x = fmaxf(end_x, box.x);
		naui_fill_rect((Naui_Vec2) { shade_x, box.y }, (Naui_Vec2) { box.x + box.width - shade_x, box.height }, leaf_rgba(0, 0, 0, 90), 0.0f, NAUI_CORNER_NONE);
		naui_draw_line((Naui_Vec2) { end_x, box.y }, (Naui_Vec2) { end_x, box.y + box.height }, accent_color, 1.0f);
	}

	if (editor->wave_hovered && !editor->panning)
		_uph_sample_editor_draw_readout(editor, box);

	naui_pop_clip_rect();
}

#pragma endregion

#pragma region Layout

static void _uph_sample_editor_render_message(const char *message)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
	})
	{
		leaf_text(message, {
			.color = {naui_theme_color("uph_ui_text_disabled_color")},
			.font_size = {NAUI_DPI(naui_theme_float("uph_ui_font_size"))}
		});
	}
}

static void _uph_sample_editor_render_info(const Uph_Sample *sample, const Uph_SampleData *data, const Uph_SampleEditorSource *source)
{
	const float font_size = naui_theme_float("uph_ui_font_size");

	char info[320];
	if (source->valid)
	{
		char duration[32];
		_uph_sample_editor_format_time(duration, sizeof(duration), (double)source->frame_count / (double)source->sample_rate, 3);

		int32_t length = snprintf(info, sizeof(info), "%s  |  %u Hz", source->channels == 1 ? "Mono" : "Stereo", source->sample_rate);
		if (data->original_sample_rate && data->original_sample_rate != source->sample_rate)
			length += snprintf(info + length, sizeof(info) - (size_t)length, " (resampled from %u)", data->original_sample_rate);

		length += snprintf(info + length, sizeof(info) - (size_t)length, "  |  %s  |  %llu frames", duration, (unsigned long long)source->frame_count);

		const char *file_name = _uph_sample_editor_file_name(data->file_path.data);
		snprintf(info + length, sizeof(info) - (size_t)length, "  |  %s", file_name[0] ? file_name : "Recorded");
	}
	else
	{
		snprintf(info, sizeof(info), "No audio data");
	}

	leaf({
		.direction = LEAF_DIRECTION_VERTICAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), NAUI_DPI(6.0f)),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_gap = NAUI_DPI(2.0f)
	})
	{
		leaf_text(sample->name.data, {
			.color = {naui_theme_color("uph_ui_text_color")},
			.font_size = {NAUI_DPI(font_size + 2.0f)}
		});
		leaf_text(info, {
			.color = {naui_theme_color("uph_ui_text_disabled_color")},
			.font_size = {NAUI_DPI(font_size)}
		});
	}
}

static void _uph_sample_editor_render_label(const char *label)
{
	leaf_text(label, {
		.color = {naui_theme_color("uph_ui_text_color")},
		.font_size = {NAUI_DPI(naui_theme_float("uph_ui_font_size"))}
	});
}

static void _uph_sample_editor_render_controls(Uph_Sample *sample)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(32))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(UPH_SAMPLE_EDITOR_TOOLBAR_FIELD_GAP),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_BOTTOM,
			.color = naui_theme_color("uph_timeline_top_ruler_border_color")
		}
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(6)
		})
		{
			_uph_sample_editor_render_label("Pitch");
			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(90)), LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
			}) uph_ui_drag_int(&sample->pitch, leaf_id("uph_sample_editor_pitch"), 1, -1200, 1200, "%i", UPH_UI_DRAG_CLAMPED);
		}

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(6)
		})
		{
			_uph_sample_editor_render_label("Time scale");
			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(90)), LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
			}) uph_ui_drag_float(&sample->time_scale, leaf_id("uph_sample_editor_time_scale"), 0.01f, 0.0001f, 10000.0f, "%.4f", UPH_UI_DRAG_CLAMPED);
		}

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
			.child_gap = NAUI_DPI(6)
		})
		{
			_uph_sample_editor_render_label("Mode");

			static const char *sample_modes[] = {
				"Resample",
				"Stretch"
			};

			uint32_t mode = (uint32_t)sample->mode;
			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(90)), LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
			})
			{
				if (uph_ui_dropdown(sample_modes, 2, &mode, leaf_id("uph_sample_editor_mode")))
					sample->mode = (Uph_SampleMode)mode;
			}
		}
	}
}

#pragma endregion

void uph_sample_editor_init(void)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	_uph_sample_editor_cache_free(&editor->cache);
	memset(editor, 0, sizeof(*editor));
	editor->fit = true;
}

void uph_sample_editor_reset_interaction(void)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	editor->panning = false;
	editor->wave_hovered = false;
	editor->has_last_sample = false;
}

void uph_sample_editor_release(void)
{
	_uph_sample_editor_cache_free(&uph_sample_editor_data.cache);
	uph_sample_editor_reset_interaction();
}

void uph_sample_editor_update(void)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	Uph_Project *project = &uph_state.project;
	const Uph_ResourceIndex index = uph_state.shared.selected_resource.index;

	if (index >= (Uph_ResourceIndex)naui_list_len(project->samples))
	{
		uph_sample_editor_reset_interaction();
		_uph_sample_editor_render_message("No sample is selected.");
		return;
	}

	Uph_Sample *sample = &project->samples[index];
	const Uph_SampleData *data = NULL;
	if (sample->data_index < (Uph_ResourceIndex)naui_list_len(project->sample_data))
		data = &project->sample_data[sample->data_index];

	if (!data)
	{
		uph_sample_editor_reset_interaction();
		_uph_sample_editor_render_message("This sample has no audio data.");
		return;
	}

	if (!editor->has_last_sample || editor->last_sample != index)
	{
		editor->last_sample = index;
		editor->has_last_sample = true;
		editor->fit = true;
		editor->panning = false;
	}

	Uph_SampleEditorSource *source = &editor->source;
	source->frames = data->frames;
	source->frame_count = data->frame_count;
	source->channels = data->channel_type == UPH_SAMPLE_STEREO ? 2 : 1;
	source->sample_rate = uph_state.settings.audio.sample_rate;
	source->valid = source->frames && source->frame_count > 0 && source->sample_rate > 0;

	if (source->valid)
		_uph_sample_editor_cache_update(&editor->cache, source, sample->data_index);

	const Leaf_ID wave_id = leaf_id("uph_sample_editor_wave");
	editor->wave_box = leaf_get_bounding_box(wave_id);
	editor->wave_hovered = source->valid && leaf_hovered(wave_id) && naui_panel_is_hovered(naui_panel_current());
	_uph_sample_editor_update_input(editor, editor->wave_box);

	_uph_sample_editor_render_info(sample, data, source);
	_uph_sample_editor_render_controls(sample);

	if (!source->valid)
	{
		_uph_sample_editor_render_message("Nothing to display yet.");
		return;
	}

	const int32_t ruler_height = (int32_t)NAUI_DPI(UPH_SAMPLE_EDITOR_RULER_HEIGHT);
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(ruler_height)},
		.custom_draw = _uph_sample_editor_ruler_draw,
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_BOTTOM,
			.color = naui_theme_color("uph_timeline_top_ruler_border_color")
		},
		.color = naui_theme_color("uph_timeline_top_ruler_bg_color")
	});

	leaf({
		.id = wave_id,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
		.custom_draw = _uph_sample_editor_wave_draw
	});
}