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
}
Uph_DialogState;
static Uph_DialogState s_dialog;

static void _uph_dialog_copy(char *dest, size_t dest_size, const char *source)
{
	snprintf(dest, dest_size, "%s", source ? source : "");
}

bool uph_dialog_open(const Uph_DialogConfig *config)
{
	if (s_dialog.open || !config || !config->buttons[0])
		return false;

	s_dialog = (Uph_DialogState){0};
	for (int32_t i = 0; i < UPH_DIALOG_MAX_BUTTONS && config->buttons[i]; i++)
	{
		_uph_dialog_copy(s_dialog.buttons[i], sizeof(s_dialog.buttons[i]), config->buttons[i]);
		s_dialog.button_count++;
	}

	_uph_dialog_copy(s_dialog.title, sizeof(s_dialog.title), config->title);
	_uph_dialog_copy(s_dialog.message, sizeof(s_dialog.message), config->message);

	s_dialog.default_button = (config->default_button >= 0 && config->default_button < s_dialog.button_count) ? config->default_button : -1;
	s_dialog.cancel_button = (config->cancel_button >= 0 && config->cancel_button < s_dialog.button_count) ? config->cancel_button : -1;
	s_dialog.callback = config->callback;
	s_dialog.user_data = config->user_data;
	s_dialog.opened_this_frame = true;
	s_dialog.open = true;
	return true;
}

bool uph_dialog_is_open(void)
{
	return s_dialog.open;
}

bool uph_dialog_blocks_input(void)
{
	return s_dialog.open && !s_dialog.drawing;
}

static void _uph_dialog_finish(int32_t button)
{
	const Uph_DialogCallback callback = s_dialog.callback;
	void *user_data = s_dialog.user_data;
	s_dialog = (Uph_DialogState){0};

	if (callback)
		callback(button, user_data);
}

void uph_dialog_render(void)
{
	if (!s_dialog.open)
		return;

	naui_occlude_all_panels();

	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const float font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
	const float rounding = NAUI_DPI(naui_theme_float("uph_ui_frame_rounding"));

	int32_t pressed = -1;
	s_dialog.drawing = true;

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
		.color = {leaf_rgba(0, 0, 0, 150)},
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_DIALOG_WIDTH)), LEAF_SIZE_FIT},
			.direction = LEAF_DIRECTION_VERTICAL,
			.padding = LEAF_PADDING_ALL(NAUI_DPI(18.0f)),
			.child_gap = NAUI_DPI(14.0f),
			.color = {naui_theme_color("uph_ui_dropdown_bg_color")},
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL),
			.shadow = {
				.blur_radius = NAUI_DPI(24.0f),
				.color = naui_theme_color("uph_ui_dropdown_shadow_color")
			}
		})
		{
			leaf_text(s_dialog.title, {
				.color = {text_color},
				.font_size = {font_size * 1.15f}
			});

			leaf({
				.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
				.direction = LEAF_DIRECTION_VERTICAL
			})
			{
				leaf_text(s_dialog.message, {
					.color = {text_color},
					.font_size = {font_size},
					.wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
				});
			}

			leaf({
				.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER},
				.child_gap = NAUI_DPI(8.0f)
			})
			{
				for (int32_t i = 0; i < s_dialog.button_count; i++)
				{
					const Naui_Color bg = (i == s_dialog.default_button) ? naui_theme_color("naui_widget_accent_color") : naui_theme_color("uph_ui_frame_bg_color");
					if (uph_ui_text_button_ex(s_dialog.buttons[i], leaf_id_indexed("uph_dialog_button", (uint32_t)i), bg, NAUI_CORNER_ALL))
						pressed = i;
				}
			}
		}
	}

	s_dialog.drawing = false;
	if (s_dialog.opened_this_frame)
	{
		s_dialog.opened_this_frame = false;
		return;
	}

	if (pressed < 0)
	{
	    if (s_dialog.default_button >= 0 && naui_key_pressed(NAUI_KEY_ENTER))
	        pressed = s_dialog.default_button;
	    else if (s_dialog.cancel_button >= 0 && naui_key_pressed(NAUI_KEY_ESCAPE))
	        pressed = s_dialog.cancel_button;
	}
	
	if (pressed >= 0)
	    _uph_dialog_finish(pressed);
}