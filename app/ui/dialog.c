#define UPH_DIALOG_TEXT_MAX 512
#define UPH_DIALOG_BUTTON_TEXT_MAX 64
#define UPH_DIALOG_WIDTH 440.0f

typedef struct
{
	bool open;
	bool drawing;
	bool opened_this_frame;
	int32_t button_count;
	int32_t default_button;
	int32_t cancel_button;
	char title[UPH_DIALOG_BUTTON_TEXT_MAX * 2];
	char message[UPH_DIALOG_TEXT_MAX];
	char buttons[UPH_DIALOG_MAX_BUTTONS][UPH_DIALOG_BUTTON_TEXT_MAX];
	Uph_DialogCallback callback;
	void *user_data;
	Naui_String *input;
	char input_placeholder[UPH_DIALOG_BUTTON_TEXT_MAX];
	const float *progress;
	struct
	{
		float lifetime;
	}
	visual;
}
Uph_DialogState;
static Uph_DialogState _uph_dialog_state;

static void _uph_dialog_copy(char *dest, size_t dest_size, const char *source)
{
	snprintf(dest, dest_size, "%s", source ? source : "");
}

bool uph_dialog_open(const Uph_DialogConfig *config)
{
	if (_uph_dialog_state.open || !config || !config->buttons[0])
		return false;

	_uph_dialog_state = (Uph_DialogState){0};
	for (int32_t i = 0; i < UPH_DIALOG_MAX_BUTTONS && config->buttons[i]; i++)
	{
		_uph_dialog_copy(_uph_dialog_state.buttons[i], sizeof(_uph_dialog_state.buttons[i]), config->buttons[i]);
		_uph_dialog_state.button_count++;
	}

	_uph_dialog_copy(_uph_dialog_state.title, sizeof(_uph_dialog_state.title), config->title);
	_uph_dialog_copy(_uph_dialog_state.message, sizeof(_uph_dialog_state.message), config->message);
	_uph_dialog_copy(_uph_dialog_state.input_placeholder, sizeof(_uph_dialog_state.input_placeholder), config->input_placeholder);
	_uph_dialog_state.input = config->input;
	_uph_dialog_state.progress = config->progress;

	_uph_dialog_state.default_button = (config->default_button >= 0 && config->default_button < _uph_dialog_state.button_count) ? config->default_button : -1;
	_uph_dialog_state.cancel_button = (config->cancel_button >= 0 && config->cancel_button < _uph_dialog_state.button_count) ? config->cancel_button : -1;
	_uph_dialog_state.callback = config->callback;
	_uph_dialog_state.user_data = config->user_data;
	_uph_dialog_state.opened_this_frame = true;
	_uph_dialog_state.open = true;
	return true;
}

bool uph_dialog_is_open(void)
{
	return _uph_dialog_state.open;
}

void uph_dialog_close(void)
{
	_uph_dialog_state = (Uph_DialogState){0};
}

bool uph_dialog_blocks_input(void)
{
	return _uph_dialog_state.open && !_uph_dialog_state.drawing;
}

static void _uph_dialog_finish(int32_t button)
{
	const Uph_DialogCallback callback = _uph_dialog_state.callback;
	void *user_data = _uph_dialog_state.user_data;
	_uph_dialog_state = (Uph_DialogState){0};

	if (callback)
		callback(button, user_data);
}

void uph_dialog_render(void)
{
	if (!_uph_dialog_state.open)
		return;

	naui_panel_occlude_all();

	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const float rounding = NAUI_DPI(UPH_UI_FRAME_ROUNDING);

	int32_t pressed = -1;
	_uph_dialog_state.drawing = true;

	_uph_dialog_state.visual.lifetime = naui_app_animations_enabled() ? NAUI_CLAMP01(_uph_dialog_state.visual.lifetime + naui_delta_time() * NAUI_UI_ANIM_SPEED) : 1.0f;
	const float reveal = naui_ease_out_cubic(_uph_dialog_state.visual.lifetime);
	leaf({
		.transparency = 1.0f - reveal,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
		.color = leaf_rgba(0, 0, 0, 150),
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.child_offset = {0.0f, (1.0f - reveal) * NAUI_DPI(-8.0f)},
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_DIALOG_WIDTH)), LEAF_SIZE_FIT},
			.direction = LEAF_DIRECTION_VERTICAL,
			.padding = LEAF_PADDING_ALL(NAUI_DPI(18.0f)),
			.child_gap = NAUI_DPI(14.0f),
			.color = naui_theme_color("uph_ui_dropdown_bg_color"),
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL),
			.shadow = {
				.blur_radius = NAUI_DPI(24.0f),
				.color = naui_theme_color("uph_ui_dropdown_shadow_color")
			}
		})
		{
			leaf_text(_uph_dialog_state.title, {
				.color = text_color,
				.font_size = {font_size * 1.15f}
			});

			if (_uph_dialog_state.message[0])
			{
				leaf({
					.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
					.direction = LEAF_DIRECTION_VERTICAL
				})
				{
					leaf_text(_uph_dialog_state.message, {
						.color = text_color,
						.font_size = {font_size},
						.wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
					});
				}
			}

			if (_uph_dialog_state.input)
			{
				leaf({
					.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
					.direction = LEAF_DIRECTION_HORIZONTAL
				})
				{
					uph_ui_textfield(_uph_dialog_state.input, leaf_id("uph_dialog_input"), UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE, _uph_dialog_state.input_placeholder[0] ? _uph_dialog_state.input_placeholder : NULL);
				}
			}

			if (_uph_dialog_state.progress)
			{
				const float progress = NAUI_CLAMP01(*_uph_dialog_state.progress);
				char percent[16];
				snprintf(percent, sizeof(percent), "%d%%", (int32_t)(progress * 100.0f));

				leaf({
					.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
					.direction = LEAF_DIRECTION_HORIZONTAL,
					.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
					.child_gap = NAUI_DPI(10.0f)
				})
				{
					leaf({
						.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIXED(NAUI_DPI(8.0f))},
						.color = naui_theme_color("uph_ui_frame_bg_color"),
						.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL)
					})
					leaf({
						.size = {LEAF_SIZE_PERCENT(progress), LEAF_SIZE_FULL},
						.color = naui_theme_color("uph_ui_progress_bar_color"),
						.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL)
					});

					leaf({
						.size = {LEAF_SIZE_FIXED(NAUI_DPI(40.0f)), LEAF_SIZE_FIT},
						.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
					})
					leaf_text(percent, {
						.color = text_color,
						.font_size = {font_size}
					});
				}
			}

			leaf({
				.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER},
				.child_gap = NAUI_DPI(8.0f)
			})
			{
				for (int32_t i = 0; i < _uph_dialog_state.button_count; i++)
				{
					const Naui_Color bg = (i == _uph_dialog_state.default_button) ? naui_theme_color("naui_widget_accent_color") : naui_theme_color("uph_ui_frame_bg_color");
					if (uph_ui_text_button_ex(_uph_dialog_state.buttons[i], leaf_id_indexed("uph_dialog_button", (uint32_t)i), bg, NAUI_CORNER_ALL))
						pressed = i;
				}
			}
		}
	}


	_uph_dialog_state.drawing = false;
	if (_uph_dialog_state.opened_this_frame)
	{
		_uph_dialog_state.opened_this_frame = false;
		return;
	}

	if (pressed < 0)
	{
	    if (_uph_dialog_state.default_button >= 0 && naui_key_pressed(NAUI_KEY_ENTER))
	        pressed = _uph_dialog_state.default_button;
	    else if (_uph_dialog_state.cancel_button >= 0 && naui_key_pressed(NAUI_KEY_ESCAPE))
	        pressed = _uph_dialog_state.cancel_button;
	}
	
	if (pressed >= 0)
	    _uph_dialog_finish(pressed);
}