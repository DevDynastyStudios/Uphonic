#define UPH_SAMPLE_EDITOR_RULER_HEIGHT			28
#define UPH_SAMPLE_EDITOR_LABEL_SPACING			100.0f
#define UPH_SAMPLE_EDITOR_LANE_PADDING			4.0f

#define UPH_SAMPLE_EDITOR_MAX_CHANNELS			2

#define UPH_SAMPLE_EDITOR_MIN_FRAMES_PER_PIXEL	(1.0 / 32.0)
#define UPH_SAMPLE_EDITOR_ZOOM_SPEED			0.15
#define UPH_SAMPLE_EDITOR_SHIFT_SCROLL_SPEED	0.1

#define UPH_SAMPLE_EDITOR_STEM_MIN_PIXELS		6.0f
#define UPH_SAMPLE_EDITOR_DOT_SIZE				4.0f

#define UPH_SAMPLE_EDITOR_GUTTER_WIDTH			48.0f
#define UPH_SAMPLE_EDITOR_SIDEBAR_WIDTH			240.0f
#define UPH_SAMPLE_EDITOR_SIDEBAR_CONTROL_WIDTH	112.0f
#define UPH_SAMPLE_EDITOR_STATUS_HEIGHT			26.0f
#define UPH_SAMPLE_EDITOR_SECTION_PADDING		12.0f
#define UPH_SAMPLE_EDITOR_PROPERTY_LABEL_WIDTH	84.0f
#define UPH_SAMPLE_EDITOR_PROPERTY_VALUE_WIDTH	120.0f
#define UPH_SAMPLE_EDITOR_BUTTON_ZOOM_FACTOR	1.5

#define UPH_SAMPLE_EDITOR_GAIN_DB_MIN			-60.0f
#define UPH_SAMPLE_EDITOR_GAIN_DB_MAX			6.0f

#define UPH_SAMPLE_EDITOR_TIME_SCALE_MIN		0.0001
#define UPH_SAMPLE_EDITOR_TIME_SCALE_MAX		10000.0

typedef enum
{
	UPH_SAMPLE_EDITOR_TIME_UNIT_RATIO,
	UPH_SAMPLE_EDITOR_TIME_UNIT_BEATS,
	UPH_SAMPLE_EDITOR_TIME_UNIT_SECONDS,
	UPH_SAMPLE_EDITOR_TIME_UNIT_BPM,
	UPH_SAMPLE_EDITOR_TIME_UNIT_COUNT
}
Uph_SampleEditorTimeUnit;

typedef struct
{
	const Uph_SampleData *data;
	const float *frames;
	uint64_t frame_count;
	uint32_t channels;
	uint32_t sample_rate;
	bool valid;
}
Uph_SampleEditorSource;

typedef struct
{
	Uph_SampleEditorSource source;

	double start_frame;
	double frames_per_pixel;
	float gain;
	float time_value;
	uint32_t time_unit;
	Naui_Vec2 pan_last_mouse;
	Leaf_BoundingBox wave_box;
	uint64_t hover_frame;
	double playback_length;
	double playhead_drag_beat;
	bool playhead_dragging;
	bool panel_hovered;
	Uph_LoopRegionDrag loop_drag;

	Uph_ResourceIndex last_sample;
	bool has_last_sample;
	bool fit;
	bool panning;
	bool wave_hovered;
}
Uph_SampleEditorData;

static Uph_SampleEditorData uph_sample_editor_data;

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

	if (editor->wave_hovered)
	{
		const double frame = editor->start_frame + (double)((float)naui_mouse_x() - box.x) * editor->frames_per_pixel;
		editor->hover_frame = (uint64_t)NAUI_CLAMP(frame, 0.0, (double)(editor->source.frame_count - 1));
	}
}

static double _uph_sample_editor_fit_frames_per_pixel(const Uph_SampleEditorData *editor)
{
	const double width = editor->wave_box.width;
	if (width < 1.0)
		return 0.0;

	return fmax((double)editor->source.frame_count / width, UPH_SAMPLE_EDITOR_MIN_FRAMES_PER_PIXEL);
}

static void _uph_sample_editor_set_zoom(Uph_SampleEditorData *editor, double frames_per_pixel)
{
	const double width = editor->wave_box.width;
	if (!editor->source.valid || width < 1.0)
		return;

	const double center = editor->start_frame + width * editor->frames_per_pixel * 0.5;
	editor->fit = false;
	editor->frames_per_pixel = frames_per_pixel;
	editor->start_frame = center - width * frames_per_pixel * 0.5;
	_uph_sample_editor_view_resolve(editor, width);
}

static double _uph_sample_editor_playhead_frame(const Uph_SampleEditorData *editor)
{
	if (editor->playback_length <= 0.0)
		return 0.0;

	const double t = NAUI_CLAMP(uph_sample_preview_position() / editor->playback_length, 0.0, 1.0);
	return t * (double)editor->source.frame_count;
}

static void _uph_sample_editor_toggle_playback(void)
{
	if (uph_sample_preview_playing())
	{
		uph_sample_preview_pause();
		return;
	}

	uph_state.shared.song_timeline_playing = false;
	uph_midi_preview_pause();
	uph_sample_preview_play(uph_state.shared.selected_resource.index);
}

static void _uph_sample_editor_update_playhead_drag(Uph_SampleEditorData *editor, Leaf_BoundingBox box)
{
	const float mouse_x = (float)naui_mouse_x();
	const float mouse_y = (float)naui_mouse_y();
	const bool over_ruler = mouse_x >= box.x && mouse_x < box.x + box.width && mouse_y >= box.y && mouse_y < box.y + box.height;

	if (!editor->playhead_dragging && over_ruler && editor->panel_hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
	{
		editor->playhead_dragging = true;
		editor->playhead_drag_beat = -1.0;
	}

	if (!editor->playhead_dragging)
		return;

	const double total = (double)editor->source.frame_count;
	const double frame = NAUI_CLAMP(editor->start_frame + (double)(mouse_x - box.x) * editor->frames_per_pixel, 0.0, total);
	const double beat = total > 0.0 ? frame / total * editor->playback_length : 0.0;
	if (beat != editor->playhead_drag_beat)
	{
		editor->playhead_drag_beat = beat;
		uph_sample_preview_seek(beat);
	}

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		editor->playhead_dragging = false;
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

static Uph_Sample *_uph_sample_editor_current_sample(void)
{
	const Uph_ResourceIndex index = uph_state.shared.selected_resource.index;
	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_SAMPLE || index >= (Uph_ResourceIndex)naui_list_len(uph_state.project.samples))
		return NULL;

	return &uph_state.project.samples[index];
}

static float _uph_sample_editor_seconds_zoom(const Uph_SampleEditorData *editor)
{
	return (float)((double)editor->source.sample_rate / editor->frames_per_pixel);
}

static float _uph_sample_editor_seconds_scroll(const Uph_SampleEditorData *editor)
{
	return (float)(editor->start_frame / editor->frames_per_pixel);
}

static double _uph_sample_editor_minor_step(const Uph_SampleEditorData *editor)
{
	const double step = _uph_sample_editor_major_step((double)editor->source.sample_rate / editor->frames_per_pixel);
	return step / (double)_uph_sample_editor_step_subdivisions(step);
}

static void _uph_sample_editor_ruler_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	const Uph_SampleEditorSource *source = &editor->source;
	if (!source->valid)
		return;

	_uph_sample_editor_view_resolve(editor, box.width);

	Uph_Sample *sample = _uph_sample_editor_current_sample();
	const float zoom = _uph_sample_editor_seconds_zoom(editor);
	const float scroll = _uph_sample_editor_seconds_scroll(editor);

	bool loop_busy = false;
	if (sample)
	{
		loop_busy = uph_loop_region_update(
			&sample->loop_region, &editor->loop_drag, box,
			zoom, scroll, _uph_sample_editor_minor_step(editor),
			!editor->playhead_dragging && editor->panel_hovered
		);
		if (editor->loop_drag.created)
		{
			editor->loop_drag.created = false;
			uph_sample_preview_set_loop(true);
		}
		uph_loop_region_render_ruler(&sample->loop_region, box, zoom, scroll, uph_sample_preview_looping());
	}
	else
		editor->loop_drag.mode = UPH_LOOP_DRAG_NONE;

	if (!loop_busy)
		_uph_sample_editor_update_playhead_drag(editor, box);

	const Leaf_Color sub_color = naui_theme_color("uph_timeline_top_ruler_grid_subbeat_color");
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
			(Naui_Vec2) { x, box.y + box.height * (is_major ? 0.4f : 0.6f) },
			(Naui_Vec2) { x, box.y + box.height },
			is_major ? bar_color : sub_color,
			1.0f
		);

		if (is_major)
		{
			char label[32];
			_uph_sample_editor_format_time(label, sizeof(label), time, decimals);
			naui_draw_text((Naui_Vec2) { x + 4.0f, box.y + box.height * 0.4f }, label, NAUI_DPI(13.0f), 0, number_color);
		}
	}

	const float playhead_x = _uph_sample_editor_frame_to_x(editor, box, _uph_sample_editor_playhead_frame(editor));
	const float icon_size = NAUI_DPI(16.0f);
	const Naui_Color playhead_color = naui_theme_color("uph_playhead_color");
	naui_draw_line((Naui_Vec2) { playhead_x, box.y + box.height - icon_size * 0.5f }, (Naui_Vec2) { playhead_x, box.y + box.height }, playhead_color, NAUI_DPI(1.0f));
	naui_draw_image(
		naui_asset_image("uph_icon_playhead"),
		(Naui_Vec2) { playhead_x - icon_size * 0.5f, box.y + box.height - icon_size },
		(Naui_Vec2) { icon_size, icon_size },
		playhead_color,
		0.0f,
		NAUI_CORNER_NONE
	);
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
	const Naui_List(Uph_ChannelPeak) peaks = source->data->channel_peaks;
	const uint64_t bin_count = (uint64_t)naui_list_len(peaks) / source->channels;

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
			const float v = _uph_sample_editor_clamp_amplitude(source->frames[(uint64_t)i * channels + channel] * editor->gain);
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

	const bool use_bins = fpp >= (double)UPH_WAVEFORM_BIN_FRAMES && bin_count > 0;
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
			uint64_t bin_start = (uint64_t)(frame_start / UPH_WAVEFORM_BIN_FRAMES);
			uint64_t bin_end = (uint64_t)ceil(frame_end / UPH_WAVEFORM_BIN_FRAMES);
			if (bin_end <= bin_start) bin_end = bin_start + 1;
			if (bin_end > bin_count) bin_end = bin_count;

			for (uint64_t b = bin_start; b < bin_end; b++)
			{
				const Uph_ChannelPeak *peak = &peaks[b * channels + channel];
				if (peak->min < min_v) min_v = peak->min;
				if (peak->max > max_v) max_v = peak->max;
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

		min_v = _uph_sample_editor_clamp_amplitude(min_v * editor->gain);
		max_v = _uph_sample_editor_clamp_amplitude(max_v * editor->gain);

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

static float _uph_sample_editor_db_to_amplitude(float db)
{
	return powf(10.0f, db / 20.0f);
}

static Leaf_BoundingBox _uph_sample_editor_lane_box(Leaf_BoundingBox box, uint32_t channel, uint32_t channels)
{
	const float lane_height = box.height / (float)channels;
	const int32_t lane_top = (int32_t)roundf((float)box.y + lane_height * (float)channel);
	const int32_t lane_bottom = (int32_t)roundf((float)box.y + lane_height * (float)(channel + 1));
	return (Leaf_BoundingBox) { box.x, lane_top, box.width, lane_bottom - lane_top };
}

static void _uph_sample_editor_gutter_draw(Leaf_BoundingBox box, void *user_data)
{
	const Uph_SampleEditorData *editor = &uph_sample_editor_data;
	const Uph_SampleEditorSource *source = &editor->source;
	if (!source->valid)
		return;

	static const float marks[] = { 0.0f, -6.0f, -12.0f };
	const uint32_t channels = source->channels;
	const float font_size = NAUI_DPI(10.0f);
	const float tick = NAUI_DPI(4.0f);
	const float padding = NAUI_DPI(UPH_SAMPLE_EDITOR_LANE_PADDING);
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_disabled_color");
	const Leaf_Color tick_color = naui_theme_color("uph_timeline_top_ruler_grid_beat_color");

	naui_fill_rect((Naui_Vec2) { box.x, box.y }, (Naui_Vec2) { box.width, box.height }, naui_theme_color("uph_toolbox_bg_color"), 0.0f, NAUI_CORNER_NONE);
	naui_push_clip_rect(box.x, box.y, box.width, box.height);

	for (uint32_t c = 0; c < channels; c++)
	{
		const Leaf_BoundingBox lane = _uph_sample_editor_lane_box(box, c, channels);
		const float mid_y = lane.y + lane.height * 0.5f;
		const float half = fmaxf(1.0f, (lane.height - padding * 2.0f) * 0.5f);
		const bool compact = half < NAUI_DPI(36.0f);

		const char *channel_label = channels == 1 ? "M" : (c == 0 ? "L" : "R");
		const Naui_Vec2 channel_size = naui_measure_text(channel_label, 1, NAUI_DPI(12.0f), 0);
		naui_draw_text((Naui_Vec2) { box.x + NAUI_DPI(6.0f), mid_y - channel_size.y * 0.5f }, channel_label, NAUI_DPI(12.0f), 0, naui_theme_color("uph_ui_text_color"));

		for (int32_t side = -1; side <= 1; side += 2)
		{
			for (uint32_t m = 0; m < (uint32_t)(sizeof(marks) / sizeof(marks[0])); m++)
			{
				if (compact && m > 0)
					break;

				const float y = mid_y - (float)side * _uph_sample_editor_db_to_amplitude(marks[m]) * half;
				char label[16];
				snprintf(label, sizeof(label), "%d", (int)marks[m]);
				const Naui_Vec2 size = naui_measure_text(label, (uint32_t)strlen(label), font_size, 0);

				naui_draw_line((Naui_Vec2) { box.x + box.width - tick, y }, (Naui_Vec2) { box.x + box.width, y }, tick_color, 1.0f);
				naui_draw_text((Naui_Vec2) { box.x + box.width - tick - NAUI_DPI(3.0f) - size.x, NAUI_CLAMP(y - size.y * 0.5f, (float)lane.y, (float)(lane.y + lane.height) - size.y) }, label, font_size, 0, text_color);
			}
		}

		const char *inf_label = "-inf";
		const Naui_Vec2 inf_size = naui_measure_text(inf_label, (uint32_t)strlen(inf_label), font_size, 0);
		naui_draw_line((Naui_Vec2) { box.x + box.width - tick, mid_y }, (Naui_Vec2) { box.x + box.width, mid_y }, tick_color, 1.0f);
		if (!compact)
			naui_draw_text((Naui_Vec2) { box.x + box.width - tick - NAUI_DPI(3.0f) - inf_size.x, mid_y - inf_size.y * 0.5f }, inf_label, font_size, 0, text_color);

		if (c > 0)
			naui_draw_line((Naui_Vec2) { box.x, (float)lane.y }, (Naui_Vec2) { box.x + box.width, (float)lane.y }, naui_theme_color("uph_timeline_grid_beat_color"), 1.0f);
	}

	naui_pop_clip_rect();
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
	const float guide_amplitude = _uph_sample_editor_db_to_amplitude(-6.0f);
	for (uint32_t c = 0; c < channels; c++)
	{
		const Leaf_BoundingBox lane = _uph_sample_editor_lane_box(box, c, channels);
		const float mid_y = lane.y + lane.height * 0.5f;
		const float half = fmaxf(1.0f, (lane.height - padding * 2.0f) * 0.5f);

		naui_draw_line((Naui_Vec2) { lane.x, mid_y - half * guide_amplitude }, (Naui_Vec2) { lane.x + lane.width, mid_y - half * guide_amplitude }, guide_color, 1.0f);
		naui_draw_line((Naui_Vec2) { lane.x, mid_y + half * guide_amplitude }, (Naui_Vec2) { lane.x + lane.width, mid_y + half * guide_amplitude }, guide_color, 1.0f);
		naui_draw_line((Naui_Vec2) { lane.x, mid_y }, (Naui_Vec2) { lane.x + lane.width, mid_y }, zero_color, 1.0f);

		_uph_sample_editor_draw_channel(editor, c, lane, accent_color);

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

	const Uph_Sample *loop_sample = _uph_sample_editor_current_sample();
	if (loop_sample)
		uph_loop_region_render_bounds(&loop_sample->loop_region, (float)box.x, (float)box.y, (float)(box.y + box.height), _uph_sample_editor_seconds_zoom(editor), _uph_sample_editor_seconds_scroll(editor), uph_sample_preview_looping());

	const float playhead_x = _uph_sample_editor_frame_to_x(editor, box, _uph_sample_editor_playhead_frame(editor));
	naui_draw_line((Naui_Vec2) { playhead_x, box.y }, (Naui_Vec2) { playhead_x, box.y + box.height }, naui_theme_color("uph_playhead_color"), NAUI_DPI(1.0f));

	if (editor->wave_hovered && !editor->panning)
	{
		const float x = _uph_sample_editor_frame_to_x(editor, box, (double)editor->hover_frame);
		Leaf_Color cursor_color = naui_theme_color("uph_ui_text_color");
		cursor_color.a = (uint8_t)(cursor_color.a / 3);
		naui_draw_line((Naui_Vec2) { x, box.y }, (Naui_Vec2) { x, box.y + box.height }, cursor_color, 1.0f);
	}

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
			.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE)}
		});
	}
}

static void _uph_sample_editor_render_label(const char *label)
{
	leaf_text(label, {
		.color = {naui_theme_color("uph_ui_text_color")},
		.font_size = {NAUI_DPI(UPH_TOOLBOX_FONT_SIZE)}
	});
}

#define _uph_sample_editor_sidebar_row(label) \
	leaf({ \
		.direction = LEAF_DIRECTION_HORIZONTAL, \
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FRAME_HEIGHT))}, \
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}, \
		.child_gap = NAUI_DPI(8) \
	}) \
	for (bool _row_once = (_uph_sample_editor_sidebar_label(label), true); _row_once; _row_once = false) \
	leaf({ \
		.size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_SAMPLE_EDITOR_SIDEBAR_CONTROL_WIDTH)), LEAF_SIZE_FULL}, \
		.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER} \
	})

static void _uph_sample_editor_sidebar_label(const char *label)
{
	leaf({ .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT} })
	{
		_uph_sample_editor_render_label(label);
	}
}

static double _uph_sample_editor_time_from_scale(uint32_t unit, double scale, double seconds, double bpm)
{
	switch (unit)
	{
		case UPH_SAMPLE_EDITOR_TIME_UNIT_BEATS: return seconds * scale * bpm / 60.0;
		case UPH_SAMPLE_EDITOR_TIME_UNIT_SECONDS: return seconds * scale;
		case UPH_SAMPLE_EDITOR_TIME_UNIT_BPM: return bpm / scale;
		default: return scale;
	}
}

static double _uph_sample_editor_time_to_scale(uint32_t unit, double value, double seconds, double bpm)
{
	switch (unit)
	{
		case UPH_SAMPLE_EDITOR_TIME_UNIT_BEATS: return value * 60.0 / (bpm * seconds);
		case UPH_SAMPLE_EDITOR_TIME_UNIT_SECONDS: return value / seconds;
		case UPH_SAMPLE_EDITOR_TIME_UNIT_BPM: return bpm / value;
		default: return value;
	}
}

static void _uph_sample_editor_render_time_scale(Uph_SampleEditorData *editor, Uph_Sample *sample, const Uph_SampleEditorSource *source)
{
	static const char *units[UPH_SAMPLE_EDITOR_TIME_UNIT_COUNT] = {
		"Ratio",
		"Beats",
		"Seconds",
		"BPM"
	};
	static const char *formats[UPH_SAMPLE_EDITOR_TIME_UNIT_COUNT] = {
		"%.4f",
		"%.3f",
		"%.3f",
		"%.2f"
	};
	static const float speeds[UPH_SAMPLE_EDITOR_TIME_UNIT_COUNT] = {
		0.01f,
		0.05f,
		0.01f,
		0.1f
	};

	const double seconds = source->valid ? (double)source->frame_count / (double)source->sample_rate : 0.0;
	const double bpm = uph_state.project.bpm > 0.0f ? (double)uph_state.project.bpm : 120.0;

	if (seconds <= 0.0)
		editor->time_unit = UPH_SAMPLE_EDITOR_TIME_UNIT_RATIO;

	const uint32_t unit = editor->time_unit < UPH_SAMPLE_EDITOR_TIME_UNIT_COUNT ? editor->time_unit : UPH_SAMPLE_EDITOR_TIME_UNIT_RATIO;
	const double scale = uph_sample_effective_time_scale(sample, bpm);

	double min_value = _uph_sample_editor_time_from_scale(unit, UPH_SAMPLE_EDITOR_TIME_SCALE_MIN, seconds, bpm);
	double max_value = _uph_sample_editor_time_from_scale(unit, UPH_SAMPLE_EDITOR_TIME_SCALE_MAX, seconds, bpm);
	if (min_value > max_value)
	{
		const double swap = min_value;
		min_value = max_value;
		max_value = swap;
	}

	static const char *ids[UPH_SAMPLE_EDITOR_TIME_UNIT_COUNT] = {
		"uph_sample_editor_time_scale",
		"uph_sample_editor_time_beats",
		"uph_sample_editor_time_seconds",
		"uph_sample_editor_time_bpm"
	};

	editor->time_value = (float)_uph_sample_editor_time_from_scale(unit, scale, seconds, bpm);
	_uph_sample_editor_sidebar_row("Time")
	{
		if (uph_ui_drag_float(&editor->time_value, leaf_id(ids[unit]), speeds[unit], (float)min_value, (float)max_value, formats[unit], UPH_UI_DRAG_FLAG_CLAMPED)
			&& editor->time_value > 0.0f)
		{
			const double new_scale = _uph_sample_editor_time_to_scale(unit, (double)editor->time_value, seconds, bpm);
			sample->tempo_bpm = (float)bpm;
			sample->time_scale = (float)NAUI_CLAMP(new_scale, UPH_SAMPLE_EDITOR_TIME_SCALE_MIN, UPH_SAMPLE_EDITOR_TIME_SCALE_MAX);
		}
	}

	uint32_t selected = unit;
	_uph_sample_editor_sidebar_row("Time unit")
	{
		if (uph_ui_dropdown(units, seconds > 0.0 ? UPH_SAMPLE_EDITOR_TIME_UNIT_COUNT : 1, &selected, leaf_id("uph_sample_editor_time_unit")))
			editor->time_unit = selected;
	}
}

static void _uph_sample_editor_set_tempo_follow(Uph_Sample *sample, Uph_SampleMode mode, bool follow_tempo)
{
	const double bpm = uph_state.project.bpm > 0.0f ? (double)uph_state.project.bpm : 120.0;
	const double scale = uph_sample_effective_time_scale(sample, bpm);

	sample->mode = mode;
	sample->follow_tempo = follow_tempo;
	sample->tempo_bpm = (float)bpm;
	sample->time_scale = (float)NAUI_CLAMP(scale, UPH_SAMPLE_EDITOR_TIME_SCALE_MIN, UPH_SAMPLE_EDITOR_TIME_SCALE_MAX);
}

static void _uph_sample_editor_render_toolbar(Uph_SampleEditorData *editor)
{
	const Uph_SampleEditorSource *source = &editor->source;

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_TOOLBOX_HEIGHT))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_TOOLBOX_PADDING_X), 0.0f),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_BOTTOM,
			.color = naui_theme_color("uph_toolbox_border_color")
		}
	})
	{
		const Naui_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
		const Naui_Color icon_color = naui_theme_color("uph_tool_icon_color");
		const int32_t button_size = NAUI_DPI(UPH_TOOLBOX_ICON_SIZE);
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
		})
		{
			const bool playing = uph_sample_preview_playing();

			if (uph_ui_image_button_ex(
				naui_asset_image(playing ? "uph_icon_pause" : "uph_icon_play"),
				leaf_id("uph_sample_editor_play"),
				(Naui_Vec2){button_size, button_size},
				naui_theme_color(playing ? "uph_pause_icon_color" : "uph_play_icon_color"),
				bg_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL
			)) _uph_sample_editor_toggle_playback();

			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_stop"),
				leaf_id("uph_sample_editor_stop"),
				(Naui_Vec2){button_size, button_size},
				naui_theme_color("uph_stop_icon_color"),
				bg_color,
				NAUI_CORNER_NONE
			)) uph_sample_preview_stop();

			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_reset"),
				leaf_id("uph_sample_editor_reset"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_NONE
			)) uph_sample_preview_seek(0.0);

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_loop"),
				leaf_id("uph_sample_editor_loop"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				uph_sample_preview_looping()
			)) uph_sample_preview_set_loop(!uph_sample_preview_looping());
		}

		leaf({ .size = {LEAF_SIZE_FIXED(NAUI_DPI(22)), LEAF_SIZE_FULL} });

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
		})
		{
			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_zoom_out"),
				leaf_id("uph_sample_editor_zoom_out"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL
			)) _uph_sample_editor_set_zoom(editor, editor->frames_per_pixel * UPH_SAMPLE_EDITOR_BUTTON_ZOOM_FACTOR);

			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_zoom_in"),
				leaf_id("uph_sample_editor_zoom_in"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR
			)) _uph_sample_editor_set_zoom(editor, editor->frames_per_pixel / UPH_SAMPLE_EDITOR_BUTTON_ZOOM_FACTOR);
		}

		leaf({ .size = {LEAF_SIZE_FIXED(NAUI_DPI(8)), LEAF_SIZE_FULL} });

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
		})
		{
			if (uph_ui_text_button_ex("Fit", leaf_id("uph_sample_editor_zoom_fit"), bg_color, NAUI_CORNER_TL | NAUI_CORNER_BL))
				editor->fit = true;

			if (uph_ui_text_button_ex("1:1", leaf_id("uph_sample_editor_zoom_one"), bg_color, NAUI_CORNER_TR | NAUI_CORNER_BR))
				_uph_sample_editor_set_zoom(editor, 1.0);
		}

		leaf({ .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL} });

		if (source->valid)
		{
			const double sample_rate = (double)source->sample_rate;
			char total[32];
			_uph_sample_editor_format_time(total, sizeof(total), (double)source->frame_count / sample_rate, 3);

			char current[32];
			_uph_sample_editor_format_time(current, sizeof(current), _uph_sample_editor_playhead_frame(editor) / sample_rate, 3);

			char length[40];
			snprintf(length, sizeof(length), "/ %s", total);

			leaf({
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
				.child_gap = NAUI_DPI(8)
			})
			{
				leaf_text(current, {
					.color = {naui_theme_color("uph_ui_text_color")},
					.font_size = {NAUI_DPI(20.0f)}
				});
				leaf_text(length, {
					.color = {naui_theme_color("uph_ui_text_disabled_color")},
					.font_size = {NAUI_DPI(UPH_TOOLBOX_FONT_SIZE)}
				});
			}
		}
	}
}

static void _uph_sample_editor_render_heading(const char *title)
{
	leaf_text(title, {
		.color = {naui_theme_color("uph_ui_text_color")},
		.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE + 2.0f)},
		.wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
	});
}

static void _uph_sample_editor_render_separator(void)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(1)},
		.color = {naui_theme_color("uph_toolbox_separator_color")}
	});
}

static void _uph_sample_editor_render_property(const char *label, const char *value)
{
	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
	})
	{
		leaf({ .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_SAMPLE_EDITOR_PROPERTY_LABEL_WIDTH)), LEAF_SIZE_FIT} })
		{
			leaf_text(label, {
				.color = {naui_theme_color("uph_ui_text_disabled_color")},
				.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE - 1.0f)}
			});
		}
		leaf({ .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_SAMPLE_EDITOR_PROPERTY_VALUE_WIDTH)), LEAF_SIZE_FIT}, .clip_children = true })
		{
			leaf_text(value, {
				.color = {naui_theme_color("uph_ui_text_color")},
				.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE - 1.0f)}
			});
		}
	}
}

static void _uph_sample_editor_format_db(char *out, size_t size, double amplitude)
{
	if (amplitude <= 1e-9)
		snprintf(out, size, "-inf dB");
	else
		snprintf(out, size, "%.1f dB", 20.0 * log10(amplitude));
}

static void _uph_sample_editor_render_properties(const Uph_SampleEditorData *editor, const Uph_SampleData *data)
{
	const Uph_SampleEditorSource *source = &editor->source;

	char sample_rate[32], channels[16], length[32], frames[32], peak[24], rms[24], original_rate[32], file[256];
	snprintf(sample_rate, sizeof(sample_rate), "%u Hz", source->sample_rate);
	snprintf(channels, sizeof(channels), "%s", source->channels == 1 ? "Mono" : "Stereo");
	_uph_sample_editor_format_time(length, sizeof(length), (double)source->frame_count / (double)source->sample_rate, 3);
	snprintf(frames, sizeof(frames), "%llu", (unsigned long long)source->frame_count);

	const uint64_t sample_count = source->frame_count * source->channels;
	_uph_sample_editor_format_db(peak, sizeof(peak), (double)data->peak * editor->gain);
	_uph_sample_editor_format_db(rms, sizeof(rms), sample_count ? sqrt(data->sum_squares / (double)sample_count) * editor->gain : 0.0);

	if (data->original_sample_rate)
		snprintf(original_rate, sizeof(original_rate), "%u Hz", data->original_sample_rate);
	else
		snprintf(original_rate, sizeof(original_rate), "-");

	const char *file_name = _uph_sample_editor_file_name(data->file_path.data);
	snprintf(file, sizeof(file), "%s", file_name[0] ? file_name : "Recorded");

	leaf({
		.direction = LEAF_DIRECTION_VERTICAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_SAMPLE_EDITOR_SECTION_PADDING), NAUI_DPI(10.0f)),
		.child_gap = NAUI_DPI(6),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.clip_children = true,
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_TOP,
			.color = naui_theme_color("uph_toolbox_border_color")
		}
	})
	{
		_uph_sample_editor_render_heading("Properties");

		leaf({ .direction = LEAF_DIRECTION_HORIZONTAL, .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT}, .child_gap = NAUI_DPI(12) })
		{
			_uph_sample_editor_render_property("Sample Rate", sample_rate);
			_uph_sample_editor_render_property("Channels", channels);
			_uph_sample_editor_render_property("Length", length);
			_uph_sample_editor_render_property("Frames", frames);
		}
		leaf({ .direction = LEAF_DIRECTION_HORIZONTAL, .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT}, .child_gap = NAUI_DPI(12) })
		{
			_uph_sample_editor_render_property("Peak", peak);
			_uph_sample_editor_render_property("RMS", rms);
			_uph_sample_editor_render_property("Source Rate", original_rate);
			_uph_sample_editor_render_property("File", file);
		}
	}
}

static void _uph_sample_editor_render_sidebar(Uph_SampleEditorData *editor, Uph_Sample *sample, const Uph_SampleEditorSource *source)
{
	leaf({
		.direction = LEAF_DIRECTION_VERTICAL,
		.size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_SAMPLE_EDITOR_SIDEBAR_WIDTH)), LEAF_SIZE_FULL},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_SAMPLE_EDITOR_SECTION_PADDING), NAUI_DPI(UPH_SAMPLE_EDITOR_SECTION_PADDING)),
		.child_gap = NAUI_DPI(8),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.clip_children = true,
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_LEFT,
			.color = naui_theme_color("uph_toolbox_border_color")
		}
	})
	{
		_uph_sample_editor_render_heading(sample->name.data);
		_uph_sample_editor_render_separator();

		leaf_text("Playback", {
			.color = {naui_theme_color("uph_ui_text_disabled_color")},
			.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE - 1.0f)}
		});

		_uph_sample_editor_sidebar_row("Pitch")
		{
			uph_ui_drag_int(&sample->pitch, leaf_id("uph_sample_editor_pitch"), 1, -1200, 1200, "%i", UPH_UI_DRAG_FLAG_CLAMPED);
		}

		_uph_sample_editor_render_time_scale(editor, sample, source);

		static const char *sample_modes[] = {
			"Resample",
			"Stretch"
		};

		uint32_t mode = (uint32_t)sample->mode;
		_uph_sample_editor_sidebar_row("Mode")
		{
			if (uph_ui_dropdown(sample_modes, 2, &mode, leaf_id("uph_sample_editor_mode")))
				_uph_sample_editor_set_tempo_follow(sample, (Uph_SampleMode)mode, sample->follow_tempo);
		}

		_uph_sample_editor_sidebar_row("Follow tempo")
		{
			bool follow_tempo = sample->follow_tempo;
			if (uph_ui_checkbox(&follow_tempo, leaf_id("uph_sample_editor_follow_tempo")))
				_uph_sample_editor_set_tempo_follow(sample, sample->mode, follow_tempo);
		}

		_uph_sample_editor_render_separator();

		leaf_text("Level", {
			.color = {naui_theme_color("uph_ui_text_disabled_color")},
			.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE - 1.0f)}
		});

		_uph_sample_editor_sidebar_row("Gain")
		{
			float gain_db = NAUI_CLAMP(uph_linear_to_db(sample->gain), UPH_SAMPLE_EDITOR_GAIN_DB_MIN, UPH_SAMPLE_EDITOR_GAIN_DB_MAX);
			if (uph_ui_drag_float(&gain_db, leaf_id("uph_sample_editor_gain"), 0.1f, UPH_SAMPLE_EDITOR_GAIN_DB_MIN, UPH_SAMPLE_EDITOR_GAIN_DB_MAX, "%.1f dB", UPH_UI_DRAG_FLAG_CLAMPED))
				sample->gain = uph_db_to_linear(gain_db);
		}

		_uph_sample_editor_sidebar_row("Normalize")
		{
			uph_ui_checkbox(&sample->normalized, leaf_id("uph_sample_editor_normalized"));
		}
	}
}

static void _uph_sample_editor_render_status_item(const char *text, bool separator)
{
	const Leaf_Color dim_color = naui_theme_color("uph_ui_text_disabled_color");
	if (separator)
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(1), LEAF_SIZE_FIXED(NAUI_DPI(12))},
			.color = {naui_theme_color("uph_toolbox_separator_color")}
		});
	}

	leaf_text(text, {
		.color = {dim_color},
		.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE - 2.0f)}
	});
}

static void _uph_sample_editor_render_status_bar(const Uph_SampleEditorData *editor, const Uph_SampleData *data)
{
	const Uph_SampleEditorSource *source = &editor->source;
	const char *file_name = _uph_sample_editor_file_name(data->file_path.data);

	char readout[128] = { 0 };
	if (editor->wave_hovered)
	{
		const uint64_t frame = editor->hover_frame;
		char time[32];
		_uph_sample_editor_format_time(time, sizeof(time), (double)frame / (double)source->sample_rate, 3);
		int32_t length = snprintf(readout, sizeof(readout), "%s   Frame %llu", time, (unsigned long long)frame);
		if (source->channels == 1)
			snprintf(readout + length, sizeof(readout) - (size_t)length, "   %+.4f", source->frames[frame]);
		else
			snprintf(readout + length, sizeof(readout) - (size_t)length, "   L %+.4f   R %+.4f", source->frames[frame * 2], source->frames[frame * 2 + 1]);
	}

	char sample_rate[32], zoom[32];
	snprintf(sample_rate, sizeof(sample_rate), "%u Hz", source->sample_rate);
	const double fit_fpp = _uph_sample_editor_fit_frames_per_pixel(editor);
	snprintf(zoom, sizeof(zoom), "Zoom: %.0f%%", editor->frames_per_pixel > 0.0 && fit_fpp > 0.0 ? fit_fpp / editor->frames_per_pixel * 100.0 : 100.0);

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_SAMPLE_EDITOR_STATUS_HEIGHT))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_SAMPLE_EDITOR_SECTION_PADDING), 0.0f),
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.child_gap = NAUI_DPI(10),
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.clip_children = true,
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_TOP,
			.color = naui_theme_color("uph_toolbox_border_color")
		}
	})
	{
		leaf_text(file_name[0] ? file_name : "Recorded", {
			.color = {naui_theme_color("uph_ui_text_color")},
			.font_size = {NAUI_DPI(UPH_UI_FONT_SIZE - 2.0f)}
		});

		leaf({ .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL} });

		if (readout[0])
			_uph_sample_editor_render_status_item(readout, false);

		_uph_sample_editor_render_status_item(sample_rate, readout[0] != 0);
		_uph_sample_editor_render_status_item(source->channels == 1 ? "Mono" : "Stereo", true);
		_uph_sample_editor_render_status_item(zoom, true);
	}
}

#pragma endregion

void uph_sample_editor_init(void)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	memset(editor, 0, sizeof(*editor));
	editor->fit = true;
}

void uph_sample_editor_reset_interaction(void)
{
	Uph_SampleEditorData *editor = &uph_sample_editor_data;
	editor->panning = false;
	editor->wave_hovered = false;
	editor->has_last_sample = false;
	editor->playhead_dragging = false;
	editor->loop_drag.mode = UPH_LOOP_DRAG_NONE;
	uph_sample_preview_pause();
}

void uph_sample_editor_release(void)
{
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
		editor->playhead_dragging = false;
		editor->loop_drag.mode = UPH_LOOP_DRAG_NONE;
		uph_sample_preview_stop();
	}

	Uph_SampleEditorSource *source = &editor->source;
	source->data = data;
	source->frames = data->frames;
	source->frame_count = data->frame_count;
	source->channels = data->channel_type == UPH_SAMPLE_STEREO ? 2 : 1;
	source->sample_rate = uph_state.settings.audio.sample_rate;
	source->valid = source->frames && source->frame_count > 0 && source->sample_rate > 0;
	editor->gain = uph_sample_gain(sample, data);
	editor->playback_length = uph_sample_preview_length(sample, data);

	if (naui_panel_focused() == uph_state.panels.resource_editor && naui_keybind_pressed(uph_state.settings.keybinds.play_pause) && !uph_ui_any_menu_open() && !uph_ui_any_textfield_active())
		_uph_sample_editor_toggle_playback();


	const Leaf_ID wave_id = leaf_id("uph_sample_editor_wave");
	editor->wave_box = leaf_get_bounding_box(wave_id);
	editor->panel_hovered = naui_panel_is_hovered(naui_panel_current());
	editor->wave_hovered = source->valid && leaf_hovered(wave_id) && editor->panel_hovered;
	_uph_sample_editor_update_input(editor, editor->wave_box);

	_uph_sample_editor_render_toolbar(editor);

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW}
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_VERTICAL,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.clip_children = true
		})
		{
			if (!source->valid)
				_uph_sample_editor_render_message("Nothing to display yet.");
			else
			{
				const int32_t ruler_height = (int32_t)NAUI_DPI(UPH_SAMPLE_EDITOR_RULER_HEIGHT);
				const int32_t gutter_width = (int32_t)NAUI_DPI(UPH_SAMPLE_EDITOR_GUTTER_WIDTH);
				leaf({
					.direction = LEAF_DIRECTION_HORIZONTAL,
					.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(ruler_height)},
					.child_gap = 1
				})
				{
					leaf({
						.size = {LEAF_SIZE_FIXED(gutter_width), LEAF_SIZE_FULL}
					});
					leaf({
						.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIXED(ruler_height - 2)},
						.custom_draw = _uph_sample_editor_ruler_draw,
						.border = {
							.width = 1,
							.sides = LEAF_SIDE_ALL,
							.color = naui_theme_color("uph_timeline_top_ruler_border_color")
						},
						.color = naui_theme_color("uph_timeline_top_ruler_bg_color")
					});
				}

				leaf({
					.direction = LEAF_DIRECTION_HORIZONTAL,
					.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
					.child_gap = 1
				})
				{
					leaf({
						.size = {LEAF_SIZE_FIXED(gutter_width), LEAF_SIZE_FULL},
						.custom_draw = _uph_sample_editor_gutter_draw
					});
					leaf({
						.id = wave_id,
						.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
						.custom_draw = _uph_sample_editor_wave_draw
					});
				}

				_uph_sample_editor_render_properties(editor, data);
			}
		}

		_uph_sample_editor_render_sidebar(editor, sample, source);
	}

	_uph_sample_editor_render_status_bar(editor, data);
}