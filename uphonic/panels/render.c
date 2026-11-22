NAUI_PANEL(uph_render)

#define UPH_RENDER_FILE_DIALOG_KEY "uph_render_audio"
#define UPH_RENDER_PANEL_WIDTH 460
#define UPH_RENDER_PANEL_HEIGHT 380
#define UPH_RENDER_LABEL_WIDTH 130
#define UPH_RENDER_ROW_HEIGHT 28
#define UPH_RENDER_TAIL_MAX 30.0f
#define UPH_RENDER_FRAME_BUDGET (1.0 / 90.0)

typedef uint8_t Uph_RenderRange;
enum
{
	UPH_RENDER_RANGE_SONG,
	UPH_RENDER_RANGE_LOOP,
	UPH_RENDER_RANGE_COUNT
};

static const uint32_t uph_render_sample_rates[] = { 0, 44100, 48000, 88200, 96000, 192000 };
#define UPH_RENDER_SAMPLE_RATE_COUNT ((uint32_t)(sizeof(uph_render_sample_rates) / sizeof(uph_render_sample_rates[0])))

typedef struct
{
	uint32_t range;
	uint32_t sample_rate_index;
	uint32_t bit_depth;
	uint32_t channels_index;
	float tail_seconds;
	bool normalize;
	bool dither;
	float progress;
}
Uph_ProjectRenderState;
static Uph_ProjectRenderState uph__project_render_state = {
	.range = UPH_RENDER_RANGE_SONG,
	.bit_depth = UPH_RENDER_BIT_DEPTH_24,
	.dither = true
};

static bool uph__render_loop_valid(void)
{
	const Uph_LoopRegion *loop = &uph_state.project.loop_region;
	return loop->end > loop->start;
}

static Uph_RenderSettings uph__render_settings(void)
{
	const Uph_ProjectRenderState *state = &uph__project_render_state;
	Uph_RenderSettings settings = {
		.tail_seconds = state->tail_seconds,
		.sample_rate = uph_render_sample_rates[state->sample_rate_index],
		.channels = state->channels_index == 1 ? 1 : 2,
		.bit_depth = (Uph_RenderBitDepth)state->bit_depth,
		.normalize = state->normalize,
		.dither = state->dither
	};

	if (state->range == UPH_RENDER_RANGE_LOOP && uph__render_loop_valid())
	{
		settings.start_beat = uph_state.project.loop_region.start;
		settings.end_beat = uph_state.project.loop_region.end;
	}
	else
	{
		settings.start_beat = 0.0;
		settings.end_beat = uph_audio_engine_get_song_length_beats();
	}

	return settings;
}

static void uph__render_row_begin(const char *label)
{
	leaf_begin_element((Leaf_ElementConfig){
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_RENDER_ROW_HEIGHT)) },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(12),
		.direction = LEAF_DIRECTION_HORIZONTAL
	});

	leaf({
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_RENDER_LABEL_WIDTH)), .height = LEAF_SIZE_FULL },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.clip_children = true
	})
	leaf_text(label, {
		.color = uph_state.colors.ui_text,
		.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
	});

	leaf_begin_element((Leaf_ElementConfig){
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL
	});
}

static void uph__render_row_end(void)
{
	leaf_end_element();
	leaf_end_element();
}

#define UPH_RENDER_ROW(label) \
	for (int uph_render_row_once = (uph__render_row_begin(label), 1); \
		 uph_render_row_once; \
		 uph_render_row_once = (uph__render_row_end(), 0))

static void uph__render_options(void)
{
	Uph_ProjectRenderState *state = &uph__project_render_state;

	const bool loop_valid = uph__render_loop_valid();
	if (!loop_valid)
		state->range = UPH_RENDER_RANGE_SONG;

	UPH_RENDER_ROW(NAUI_TR("render.range"))
	{
		const char *items[] = { NAUI_TR("render.range.song"), NAUI_TR("render.range.loop") };
		uph_ui_dropdown(items, loop_valid ? UPH_RENDER_RANGE_COUNT : 1, &state->range, leaf_id("uph_render_range"));
	}

	UPH_RENDER_ROW(NAUI_TR("render.tail"))
	{
		uph_ui_drag_float(&state->tail_seconds, leaf_id("uph_render_tail"), 0.05f, 0.0f, UPH_RENDER_TAIL_MAX, NAUI_TR("render.tail.format"), UPH_UI_DRAG_FLAG_CLAMPED);
	}

	UPH_RENDER_ROW(NAUI_TR("render.sample_rate"))
	{
		static char labels[UPH_RENDER_SAMPLE_RATE_COUNT][48];
		const char *items[UPH_RENDER_SAMPLE_RATE_COUNT];
		snprintf(labels[0], sizeof(labels[0]), "%s (%u Hz)", NAUI_TR("render.sample_rate.project"), uph_audio_engine_sample_rate());
		items[0] = labels[0];
		for (uint32_t i = 1; i < UPH_RENDER_SAMPLE_RATE_COUNT; i++)
		{
			snprintf(labels[i], sizeof(labels[i]), "%u Hz", uph_render_sample_rates[i]);
			items[i] = labels[i];
		}

		uph_ui_dropdown(items, UPH_RENDER_SAMPLE_RATE_COUNT, &state->sample_rate_index, leaf_id("uph_render_sample_rate"));
	}

	UPH_RENDER_ROW(NAUI_TR("render.bit_depth"))
	{
		const char *items[] = { NAUI_TR("render.bit_depth.16"), NAUI_TR("render.bit_depth.24"), NAUI_TR("render.bit_depth.32f") };
		uph_ui_dropdown(items, UPH_RENDER_BIT_DEPTH_COUNT, &state->bit_depth, leaf_id("uph_render_bit_depth"));
	}

	UPH_RENDER_ROW(NAUI_TR("render.channels"))
	{
		const char *items[] = { NAUI_TR("render.channels.stereo"), NAUI_TR("render.channels.mono") };
		uph_ui_dropdown(items, 2, &state->channels_index, leaf_id("uph_render_channels"));
	}

	UPH_RENDER_ROW(NAUI_TR("render.normalize"))
	{
		uph_ui_checkbox(&state->normalize, leaf_id("uph_render_normalize"));
	}

	if (state->bit_depth != UPH_RENDER_BIT_DEPTH_32_FLOAT)
	{
		UPH_RENDER_ROW(NAUI_TR("render.dither"))
		{
			uph_ui_checkbox(&state->dither, leaf_id("uph_render_dither"));
		}
	}

	const Uph_RenderSettings settings = uph__render_settings();
	const double seconds = settings.end_beat > settings.start_beat
		? uph_tempo_beat_to_seconds(settings.end_beat) - uph_tempo_beat_to_seconds(settings.start_beat) + settings.tail_seconds
		: 0.0;
	const uint32_t rate = settings.sample_rate ? settings.sample_rate : uph_audio_engine_sample_rate();
	const uint32_t bytes_per_sample = settings.bit_depth == UPH_RENDER_BIT_DEPTH_16 ? 2 : settings.bit_depth == UPH_RENDER_BIT_DEPTH_24 ? 3 : 4;
	const double megabytes = seconds * rate * settings.channels * bytes_per_sample / (1024.0 * 1024.0);

	const int32_t total_tenths = (int32_t)(seconds * 10.0 + 0.5);
	char info[128];
	snprintf(info, sizeof(info), "%s %d:%02d.%d  ·  ~%.1f MB",
		NAUI_TR("render.length"),
		total_tenths / 600, (total_tenths / 10) % 60, total_tenths % 10,
		megabytes);

	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
		.padding = { .top = NAUI_DPI(6) }
	})
	leaf_text(info, {
		.color = uph_state.colors.panel_text,
		.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
	});
}

static void uph__render_failed(void)
{
	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("render.failed.title"),
		.message = NAUI_TR("render.failed.message"),
		.buttons = { NAUI_TR("dialog.ok") },
		.default_button = 0,
		.cancel_button = 0
	});
}

static void uph__render_progress_closed(int32_t button, void *user_data)
{
	uph_audio_engine_render_cancel();
}

static void uph__render_path_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	Uph_ProjectRenderState *state = &uph__project_render_state;
	if (is_cancelled || result->count == 0)
		return;

	const Uph_RenderSettings settings = uph__render_settings();
	if (!uph_audio_engine_render_begin(result->paths[0].data, &settings))
	{
		uph__render_failed();
		return;
	}

	state->progress = 0.0f;
	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("render.progress.title"),
		.message = NAUI_TR("render.progress.message"),
		.buttons = { NAUI_TR("dialog.cancel") },
		.default_button = -1,
		.cancel_button = 0,
		.callback = uph__render_progress_closed,
		.progress = &state->progress
	});
}

void uph_render_update(void)
{
	Uph_ProjectRenderState *state = &uph__project_render_state;
	if (!uph_audio_engine_render_active())
		return;

	const Uph_RenderStatus status = uph_audio_engine_render_step(UPH_RENDER_FRAME_BUDGET);
	state->progress = uph_audio_engine_render_progress();

	if (status == UPH_RENDER_STATUS_RUNNING)
		return;

	uph_dialog_close();
	if (status == UPH_RENDER_STATUS_FAILED)
		uph__render_failed();
}

static void uph__render_start(void)
{
	const Uph_RenderSettings settings = uph__render_settings();
	if (settings.end_beat - settings.start_beat < NAUI_EPSILON)
	{
		uph_dialog_open(&(Uph_DialogConfig){
			.title = NAUI_TR("render.empty.title"),
			.message = NAUI_TR("render.empty.message"),
			.buttons = { NAUI_TR("dialog.ok") },
			.default_button = 0,
			.cancel_button = 0
		});
		return;
	}

	if (!naui_dialog_save_file(UPH_RENDER_FILE_DIALOG_KEY, NAUI_EXTENSIONS(".wav"), uph__render_path_picked, NULL))
		return;

	Naui_String name = uph_state.project.title;
	naui_string_append_cstr(&name, ".wav");
	naui_dialog_title_set(UPH_RENDER_FILE_DIALOG_KEY, NAUI_TR("render.title"));
	naui_dialog_name_set(UPH_RENDER_FILE_DIALOG_KEY, name.data);
}

static void uph_render_on_attach(void)
{
	const Naui_PanelID this = naui_panel_current();
	naui_panel_enable_flags(this, NAUI_PANEL_FLAG_NO_DOCK);
	float scale = naui_app_dpi_scale();
	if (scale <= 0.0f)
		scale = 1.0f;

	naui_panel_set_size(this, (Naui_Vec2){ UPH_RENDER_PANEL_WIDTH * scale, UPH_RENDER_PANEL_HEIGHT * scale });
	naui_panel_set_min_size(this, (Naui_Vec2){ UPH_RENDER_PANEL_WIDTH * 0.75f * scale, UPH_RENDER_PANEL_HEIGHT * 0.75f * scale });
}

static void uph_render_on_detach(void)
{

}

static void uph_render_on_open(void)
{

}

static void uph_render_on_close(void)
{

}

static void uph_render_on_update(void)
{
	const Naui_PanelID this = naui_panel_current();

	bool start = false;
	bool cancel = false;

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(18), NAUI_DPI(14)),
		.color = uph_state.colors.panel_title_bg,
		.child_gap = NAUI_DPI(4),
		.direction = LEAF_DIRECTION_VERTICAL,
		.clip_children = true
	})
	{
		uph__render_options();

		leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW } });

		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(8),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			cancel = uph_ui_text_button(NAUI_TR("dialog.cancel"), leaf_id("uph_render_cancel"));
			start = uph_ui_primary_button(NAUI_TR("render.button"), leaf_id("uph_render_start"));
		}
	}

	if (cancel || start)
		naui_panel_close(this);

	if (start)
		uph__render_start();
}
