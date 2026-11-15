#define UPH_LEVEL_METER_LOW_DB		UPH_MIXER_DB_MIN
#define UPH_LEVEL_METER_MEDIUM_DB	-12.0f
#define UPH_LEVEL_METER_HIGH_DB		0.0f

static inline float uph_level_meter_fraction(float db)
{
	const float normalized = (db - UPH_MIXER_DB_MIN) / (UPH_MIXER_DB_MAX - UPH_MIXER_DB_MIN);
	return NAUI_CLAMP(normalized, 0.0f, 1.0f);
}

static void _uph_level_meter_draw_half(Leaf_BoundingBox bounding_box, float x, float width, float clip_x, float clip_width, float fraction, Naui_Color low_color, Naui_Color medium_color, Naui_Color high_color)
{
	if (fraction <= 0.0f)
		return;

	const float top = bounding_box.y;
	const float bottom = bounding_box.y + bounding_box.height;
	const float y_low = bottom - uph_level_meter_fraction(UPH_LEVEL_METER_LOW_DB) * bounding_box.height;
	const float y_medium = bottom - uph_level_meter_fraction(UPH_LEVEL_METER_MEDIUM_DB) * bounding_box.height;
	const float y_high = bottom - uph_level_meter_fraction(UPH_LEVEL_METER_HIGH_DB) * bounding_box.height;
	naui_push_clip_rect(clip_x, bottom - fraction * bounding_box.height, clip_width, bounding_box.height * fraction);

	const float lower_height = bottom - y_medium;
	naui_fill_gradient_rect(
		(Naui_Vec2){x, y_medium},
		(Naui_Vec2){width, lower_height},
		(Naui_Gradient){
			.angle = 90 * NAUI_DEG2RAD,
			.color1 = medium_color, .color2 = low_color,
			.percent1 = 0.0f, .percent2 = (y_low - y_medium) / lower_height
		},
		0.0f, NAUI_CORNER_NONE
	);

	const float upper_height = y_medium - top;
	naui_fill_gradient_rect(
		(Naui_Vec2){x, top},
		(Naui_Vec2){width, upper_height},
		(Naui_Gradient){
			.angle = 90 * NAUI_DEG2RAD,
			.color1 = high_color, .color2 = medium_color,
			.percent1 = (y_high - top) / upper_height, .percent2 = 1.0f
		},
		0.0f, NAUI_CORNER_NONE
	);

	naui_pop_clip_rect();
}

void uph_level_meter_smooth(Uph_Track *track)
{
	const float speed = naui_delta_time() * 30.0f;
	track->smooth_peak_left = NAUI_LERP(track->smooth_peak_left, track->peak_left, speed);
	track->smooth_peak_right = NAUI_LERP(track->smooth_peak_right, track->peak_right, speed);
}

void uph_level_meter_draw(Leaf_BoundingBox bounding_box, Uph_ActionTrackRef *data)
{
	const Uph_Track *track = uph_action_track_resolve(*data);
	if (!track)
		return;

	const int32_t gap = NAUI_DPI(1);
	const float left_fraction = uph_level_meter_fraction(uph_linear_to_db(track->smooth_peak_left));
	const float right_fraction = uph_level_meter_fraction(uph_linear_to_db(track->smooth_peak_right));

	const Naui_Color low_color = naui_theme_color("uph_mixer_bar_low_color");
	const Naui_Color medium_color = naui_theme_color("uph_mixer_bar_medium_color");
	const Naui_Color high_color = naui_theme_color("uph_mixer_bar_high_color");

	const float half_width = bounding_box.width * 0.5f;
	_uph_level_meter_draw_half(
		bounding_box, bounding_box.x, half_width,
		bounding_box.x, half_width - gap,
		left_fraction, low_color, medium_color, high_color
	);

	_uph_level_meter_draw_half(
		bounding_box, bounding_box.x + half_width, half_width,
		bounding_box.x + half_width + gap, half_width - gap,
		right_fraction, low_color, medium_color, high_color
	);
}
