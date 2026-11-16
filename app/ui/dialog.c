#define UPH_DIALOG_TEXT_MAX 512
#define UPH_DIALOG_BUTTON_TEXT_MAX 64
#define UPH_DIALOG_WIDTH 440

typedef struct
{
	bool open;
	bool drawing;
	bool opened_this_frame;
	float lifetime;
	int32_t button_count;
	int32_t default_button;
	int32_t cancel_button;
	char title[UPH_DIALOG_BUTTON_TEXT_MAX * 2];
	char message[UPH_DIALOG_TEXT_MAX];
	char buttons[UPH_DIALOG_MAX_BUTTONS][UPH_DIALOG_BUTTON_TEXT_MAX];
	char input_placeholder[UPH_DIALOG_BUTTON_TEXT_MAX];
	Naui_String *input;
	const float *progress;
	Uph_DialogCallback callback;
	void *user_data;
}
Uph_DialogState;

static Uph_DialogState _uph_dialog_state;

static void _uph_dialog_copy(char *dest, size_t dest_size, const char *source)
{
	snprintf(dest, dest_size, "%s", source ? source : "");
}

static int32_t _uph_dialog_valid_button(int32_t button)
{
	return button >= 0 && button < _uph_dialog_state.button_count ? button : -1;
}

bool uph_dialog_open(const Uph_DialogConfig *config)
{
	Uph_DialogState *state = &_uph_dialog_state;
	if (state->open || !config || !config->buttons[0])
		return false;

	*state = (Uph_DialogState){
		.open = true,
		.opened_this_frame = true,
		.input = config->input,
		.progress = config->progress,
		.callback = config->callback,
		.user_data = config->user_data
	};

	while (state->button_count < UPH_DIALOG_MAX_BUTTONS && config->buttons[state->button_count])
	{
		_uph_dialog_copy(state->buttons[state->button_count], sizeof(state->buttons[0]), config->buttons[state->button_count]);
		state->button_count++;
	}

	_uph_dialog_copy(state->title, sizeof(state->title), config->title);
	_uph_dialog_copy(state->message, sizeof(state->message), config->message);
	_uph_dialog_copy(state->input_placeholder, sizeof(state->input_placeholder), config->input_placeholder);

	state->default_button = _uph_dialog_valid_button(config->default_button);
	state->cancel_button = _uph_dialog_valid_button(config->cancel_button);
	return true;
}

bool uph_dialog_is_open(void)
{
	return _uph_dialog_state.open;
}

void uph_dialog_close(void)
{
	Uph_DialogState *state = &_uph_dialog_state;
	*state = (Uph_DialogState){0};
}

bool uph_dialog_blocks_input(void)
{
	return _uph_dialog_state.open && !_uph_dialog_state.drawing;
}

static void _uph_dialog_finish(int32_t button)
{
	Uph_DialogState *state = &_uph_dialog_state;
	const Uph_DialogCallback callback = state->callback;
	void *user_data = state->user_data;
	uph_dialog_close();

	if (callback)
		callback(button, user_data);
}

static void _uph_dialog_render_progress(float progress)
{
	const int32_t rounding = NAUI_DPI(UPH_UI_FRAME_ROUNDING);
	char percent[16];
	snprintf(percent, sizeof(percent), "%d%%", (int32_t)(progress * 100.0f));

	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(10),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIXED(NAUI_DPI(8)) },
			.color = uph_state.colors.ui_frame_bg,
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL)
		})
		leaf({
			.size = { .width = LEAF_SIZE_PERCENT(progress), .height = LEAF_SIZE_FULL },
			.color = uph_state.colors.ui_progress_bar,
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL)
		});

		leaf({
			.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(40)), .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER }
		})
		leaf_text(percent, {
			.color = uph_state.colors.ui_text,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
		});
	}
}

static int32_t _uph_dialog_render_buttons(void)
{
	const Uph_DialogState *state = &_uph_dialog_state;
	int32_t pressed = -1;

	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
		.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		for (int32_t i = 0; i < state->button_count; i++)
		{
			const Naui_Color color = i == state->default_button ? uph_state.colors.widget_accent : uph_state.colors.ui_frame_bg;
			if (uph_ui_text_button_ex(state->buttons[i], leaf_id_indexed("uph_dialog_button", (uint32_t)i), color, NAUI_CORNER_ALL))
				pressed = i;
		}
	}

	return pressed;
}

static int32_t _uph_dialog_key_button(void)
{
	const Uph_DialogState *state = &_uph_dialog_state;
	if (state->default_button >= 0 && naui_key_pressed(NAUI_KEY_ENTER))
		return state->default_button;

	if (state->cancel_button >= 0 && naui_key_pressed(NAUI_KEY_ESCAPE))
		return state->cancel_button;

	return -1;
}

void uph_dialog_render(void)
{
	Uph_DialogState *state = &_uph_dialog_state;
	if (!state->open)
		return;

	naui_panel_occlude_all();
	state->drawing = true;
	state->lifetime = naui_app_animations_enabled() ? NAUI_CLAMP01(state->lifetime + naui_delta_time() * NAUI_UI_ANIM_SPEED) : 1.0f;

	const float reveal = naui_ease_out_cubic(state->lifetime);
	const Leaf_Color text_color = uph_state.colors.ui_text;
	const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	int32_t pressed = -1;

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.color = leaf_rgba(0, 0, 0, 150),
		.child_offset = { .x = 0.0f, .y = (1.0f - reveal) * NAUI_DPI(-8) },
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.transparency = 1.0f - reveal,
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	})
	leaf({
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_DIALOG_WIDTH)), .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(18)),
		.color = uph_state.colors.ui_dropdown_bg,
		.shadow = {
			.blur_radius = NAUI_DPI(24),
			.color = uph_state.colors.ui_dropdown_shadow
		},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.child_gap = NAUI_DPI(14),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		leaf_text(state->title, {
			.color = text_color,
			.font_size = LEAF_SIZE_FIXED(font_size * 1.15f)
		});

		if (state->message[0])
		{
			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
				.direction = LEAF_DIRECTION_VERTICAL
			})
			leaf_text(state->message, {
				.color = text_color,
				.font_size = LEAF_SIZE_FIXED(font_size),
				.wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
			});
		}

		if (state->input)
		{
			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
				.direction = LEAF_DIRECTION_HORIZONTAL
			})
			uph_ui_textfield(state->input, leaf_id("uph_dialog_input"), UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE, state->input_placeholder[0] ? state->input_placeholder : NULL);
		}

		if (state->progress)
			_uph_dialog_render_progress(NAUI_CLAMP01(*state->progress));

		pressed = _uph_dialog_render_buttons();
	}

	state->drawing = false;
	if (state->opened_this_frame)
	{
		state->opened_this_frame = false;
		return;
	}

	if (pressed < 0)
		pressed = _uph_dialog_key_button();

	if (pressed >= 0)
		_uph_dialog_finish(pressed);
}
