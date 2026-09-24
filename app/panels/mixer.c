NAUI_PANEL(uph_mixer)

#define UPH_MIXER_PEAK_GREEN_TOP 0.70f
#define UPH_MIXER_PEAK_YELLOW_TOP 0.90f

#define UPH_MIXER_PEAK_SMOOTH_RATE 30.0f
#define UPH_MIXER_PEAK_CAP_HOLD_TIME 0.8f 
#define UPH_MIXER_PEAK_CAP_DECAY_RATE 0.6f

typedef struct
{
	Uph_Track *track;
}
Uph_VolumePeaksCustomDrawData;

typedef struct
{
	Leaf_ID id;
}
Uph_MeterMarkerData;
static Uph_MeterMarkerData uph_global_meter_marker_data;

static void uph_mixer_meter_marker_draw(Leaf_BoundingBox box, void *user_data)
{
	const struct { float t; Leaf_Color color; } *data = user_data;
	const float y = box.y + box.height * (1.0f - data->t);
	const float size = NAUI_DPI(9.0f);

	const Naui_Vec2 points[3] =
	{
		{ box.x + box.width, y - size * 0.5f },
		{ box.x + box.width, y + size * 0.5f },
		{ box.x + box.width - size, y }
	};
	naui_fill_polygon(points, 3, data->color);
}

bool uph_mixer_meter_gain_marker(float *value, const Leaf_ID id, const float min, const float max, const char *format)
{
	Uph_TextfieldData *tdata = &uph_global_widget_data.textfield_data;
	Uph_MeterMarkerData *mdata = &uph_global_meter_marker_data;

	bool result = false;
	static Naui_String edit_buffer;
	bool text_editing = (tdata->id.value == id.value) && (tdata->mode == UPH_UI_EDIT_MODE_TEXT) && (tdata->target_number == value);

	const Leaf_BoundingBox box = leaf_get_bounding_box(id);
	const bool hovered = uph_ui_widget_hovered(id);
	const bool dragging_this = (mdata->id.value == id.value);

	if (hovered)
		naui_set_cursor(NAUI_CURSOR_HAND);

	if (!text_editing && hovered && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
	{
		edit_buffer = naui_string_format(format ? (char*)format : "%.3f", *value);
		uph_ui__begin_text_edit(tdata, id, &edit_buffer, edit_buffer.length);
		tdata->target_number = value;
		mdata->id.value = 0;
		text_editing = true;
	}

	if (text_editing)
	{
		if (uph_ui_textfield(&edit_buffer, id, UPH_UI_TEXTFIELD_NUMBER_ONLY, NULL))
		{
			*value = NAUI_CLAMP((float)atof(edit_buffer.data), min, max);
			result = true;
		}

		if (tdata->id.value != id.value || tdata->mode != UPH_UI_EDIT_MODE_TEXT)
			tdata->target_number = NULL;

		return result;
	}

	if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
		mdata->id = id;
	else if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		mdata->id.value = 0;

	if (dragging_this)
	{
		const float local_y = (float)naui_mouse_y() - box.y;
		const float t = 1.0f - NAUI_CLAMP(local_y / NAUI_MAX(box.height, 1.0f), 0.0f, 1.0f);
		const float new_value = min + t * (max - min);

		if (new_value != *value)
		{
			*value = new_value;
			result = true;
		}
	}

	const float t = (max > min) ? NAUI_CLAMP((*value - min) / (max - min), 0.0f, 1.0f) : 0.0f;
	struct { float t; Leaf_Color color; } draw_data = { .t = t, .color = naui_theme_color("uph_ui_slider_fill_color") };

	leaf({
		.id = id,
		.size = { LEAF_SIZE_PERCENT(0.90), LEAF_SIZE_FULL },
		.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
		.floating = {
			.parent_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP },
			.self_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP }
		},
		.custom_draw = uph_mixer_meter_marker_draw,
		.custom_draw_data = LEAF_DATA_SLICE(draw_data)
	});

	return result;
}

static void uph_mixer_on_attach(void)
{
	Naui_PanelID this = naui_current_panel();
	naui_panel_set_title(this, NAUI_TR("mixer.title"));
}

static void uph_mixer_on_detach(void)
{
	
}

static void uph_mixer_on_open(void)
{
	
}

static void uph_mixer_on_close(void)
{
	
}

static void uph_mixer_draw_peak_bands(Leaf_BoundingBox box, float x, float width)
{
	const Naui_Color green = leaf_rgb(80, 210, 90);
	const Naui_Color yellow = leaf_rgb(230, 195, 60);
	const Naui_Color red = leaf_rgb(225, 70, 65);

	naui_fill_rect((Naui_Vec2){ x, box.y + box.height * (1.0f - UPH_MIXER_PEAK_GREEN_TOP) },
		(Naui_Vec2){ width, box.height * UPH_MIXER_PEAK_GREEN_TOP }, green, 0.0f, NAUI_CORNER_NONE);

	naui_fill_rect((Naui_Vec2){ x, box.y + box.height * (1.0f - UPH_MIXER_PEAK_YELLOW_TOP) },
		(Naui_Vec2){ width, box.height * (UPH_MIXER_PEAK_YELLOW_TOP - UPH_MIXER_PEAK_GREEN_TOP) }, yellow, 0.0f, NAUI_CORNER_NONE);

	naui_fill_rect((Naui_Vec2){ x, box.y },
		(Naui_Vec2){ width, box.height * (1.0f - UPH_MIXER_PEAK_YELLOW_TOP) }, red, 0.0f, NAUI_CORNER_NONE);
}

static void uph_mixer_draw_peak_bar(Leaf_BoundingBox box, float x, float width, float peak)
{
	naui_fill_rect(
		(Naui_Vec2) { x, box.y },
		(Naui_Vec2) { width, box.height },
		naui_theme_color("uph_track_header_border_color"),
		0.0f,
		NAUI_CORNER_NONE
	);
	naui_push_clip_rect(x, box.y + box.height * (1.0f - peak), width, box.height);
	uph_mixer_draw_peak_bands(box, x, width);
	naui_pop_clip_rect();
}

static void uph_mixer_update_peak_cap(float *cap, float *hold_timer, const float peak, const float dt)
{
	if (peak >= *cap)
	{
		*cap = peak;
		*hold_timer = UPH_MIXER_PEAK_CAP_HOLD_TIME;
	}
	else if (*hold_timer > 0.0f)
		*hold_timer -= dt;
	else
		*cap = NAUI_MAX(0.0f, *cap - UPH_MIXER_PEAK_CAP_DECAY_RATE * dt);
}

static void uph_mixer_draw_peak_cap(Leaf_BoundingBox box, float x, float width, float cap)
{
	if (cap <= NAUI_EPSILON_F)
		return;

	const float thickness = NAUI_DPI(2.0f);
	const float y = box.y + box.height * (1.0f - cap) - thickness * 0.5f;
	naui_fill_rect((Naui_Vec2){ x, NAUI_CLAMP(y, box.y, box.y + box.height - thickness) }, (Naui_Vec2){ width, thickness }, naui_theme_color("uph_playhead_color"), 0.0f, NAUI_CORNER_NONE);
}

static void uph_mixer_volume_peaks_custom_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_VolumePeaksCustomDrawData *data = (Uph_VolumePeaksCustomDrawData*)user_data;
	Uph_Track *track = data->track;
	const float gap = NAUI_DPI(1.0f);
	const float dt = naui_delta_time();

	if (!uph_state.shared.song_timeline_playing)
	{
		track->smooth_peak_left = 0.0f;
		track->smooth_peak_right = 0.0f;
	}
	else
	{
		track->smooth_peak_left = NAUI_LERP(track->smooth_peak_left, track->peak_left, dt * UPH_MIXER_PEAK_SMOOTH_RATE);
		track->smooth_peak_right = NAUI_LERP(track->smooth_peak_right, track->peak_right, dt * UPH_MIXER_PEAK_SMOOTH_RATE);
	}

	uph_mixer_update_peak_cap(&track->peak_cap_left, &track->peak_cap_hold_left, track->smooth_peak_left, dt);
	uph_mixer_update_peak_cap(&track->peak_cap_right, &track->peak_cap_hold_right, track->smooth_peak_right, dt);

	float half_width = box.width * 0.5f;
	float bar_width = half_width - gap * 0.5f;

	uph_mixer_draw_peak_bar(box, box.x, bar_width, track->smooth_peak_left);
	uph_mixer_draw_peak_bar(box, box.x + half_width + gap * 0.5f, bar_width, track->smooth_peak_right);
	uph_mixer_draw_peak_cap(box, box.x, bar_width, track->peak_cap_left);
	uph_mixer_draw_peak_cap(box, box.x + half_width + gap * 0.5f, bar_width, track->peak_cap_right);
}

static void uph_mixer_render_track(Uph_Track *track)
{
	leaf({
		.size = {LEAF_SIZE_FIXED(NAUI_DPI(70)), LEAF_SIZE_FULL},
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(10), NAUI_DPI(10)),
		.color = naui_theme_color("uph_track_header_color"),
		.child_gap = NAUI_DPI(12),
		.border = {
			.width = 1,
			.color = {naui_theme_color("uph_track_header_border_color")},
			.sides = LEAF_SIDE_ALL
		},
	})
	{
		leaf({
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(4))},
			.color = uph_resources_track_color(track->color_index),
			.rounding = {
				.value = NAUI_DPI(4),
				.corners = LEAF_CORNER_ALL
			}
		});
		leaf_text(track->name.length ? track->name.data : NAUI_TR("song_timeline.track.title"), {
			.font_size = NAUI_DPI(12),
			.color = {track->name.length ? naui_theme_color("uph_ui_text_color") : naui_theme_color("uph_ui_text_disabled_color")}
		});

		uph_ui_knob(&track->pan, leaf_id_indexed("uph_mixer_pan", track->index), -1.0f, 1.0f, 0.0f, "%.2f");
		leaf({
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
			.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
		})
		{
			Uph_VolumePeaksCustomDrawData peaks_data = {
				.track = track
			};
			leaf({
				.size = {LEAF_SIZE_PERCENT(0.5f), LEAF_SIZE_FULL},
				.custom_draw = uph_mixer_volume_peaks_custom_draw,
				.custom_draw_data = LEAF_DATA_SLICE(peaks_data)
			});

			uph_mixer_meter_gain_marker(&track->volume, leaf_id_indexed("uph_mixer_gain", track->index), 0.0f, 2.0f, "%.2f");
		}
	}
}

static void uph_mixer_on_update(void)
{
	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
	})
	{
		for (uint32_t i = 0; i < naui_list_len(uph_state.project.tracks); i++)
		{
			Uph_Track *track = &uph_state.project.tracks[i];
			uph_mixer_render_track(track);
		}
	}
}
