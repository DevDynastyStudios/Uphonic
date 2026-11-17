#define UPH_TRACK_STRIP_WIDTH					5
#define UPH_TRACK_STRIP_HOVER_WIDTH				6
#define UPH_TRACK_STRIP_HIT_WIDTH				10
#define UPH_TRACK_HEADER_CHILD_GAP				10
#define UPH_TRACK_HEADER_ROW_GAP				6
#define UPH_TRACK_HEADER_PADDING_Y				7
#define UPH_TRACK_HEADER_CARD_INSET				4
#define UPH_TRACK_HEADER_CARD_INSET_Y			4
#define UPH_TRACK_HEADER_CARD_INSET_EDGE		6
#define UPH_TRACK_HEADER_CARD_ROUNDING			6
#define UPH_TRACK_HEADER_CONTENT_TOP_PADDING	3
#define UPH_TRACK_HEADER_KNOB_SIZE				24
#define UPH_TRACK_HEADER_METER_WIDTH			12
#define UPH_TRACK_HEADER_METER_PADDING			5
#define UPH_SONG_TIMELINE_ICON_SIZE				16
#define UPH_SONG_TIMELINE_TRACK_DRAG_THRESHOLD	6

typedef struct
{
	Uph_ActionTrackRef owner;
	uint32_t index;
	Leaf_BoundingBox row;
	bool after;
	bool valid;
}
Uph_TrackDropTarget;

static bool uph__track_list_contains(Naui_List(Uph_TrackPtr) list, const Uph_Track *track)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(list); i++)
	{
		if (list[i] == track)
			return true;
	}
	return false;
}

static bool uph__song_timeline_track_selected(const Uph_Track *track)
{
	return uph__track_list_contains(uph__song_timeline_state.selected_tracks, track);
}

#pragma region Selection

static void uph__song_timeline_remember_record_track(Uph_Track *track)
{
	if (uph_track_can_record(track) && !(uph_state.shared.song_timeline_recording && uph_state.shared.song_timeline_playing))
		uph_state.shared.record_track = track;
}

static void uph__song_timeline_select_track(Uph_Track *track)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	naui_list_clear(state->selected_tracks);
	state->selection_anchor = track;

	if (track)
		naui_list_push(state->selected_tracks, track);
	uph__song_timeline_remember_record_track(track);
}

static void uph__song_timeline_set_selection(Naui_List(Uph_TrackPtr) tracks)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	naui_list_clear(state->selected_tracks);
	state->selection_anchor = NULL;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		if (!tracks[i])
			continue;

		naui_list_push(state->selected_tracks, tracks[i]);
		state->selection_anchor = tracks[i];
	}
}

static void uph__song_timeline_toggle_track(Uph_Track *track)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->selected_tracks); i++)
	{
		if (state->selected_tracks[i] != track)
			continue;

		naui_list_remove(state->selected_tracks, i);

		const uint32_t remaining = (uint32_t)naui_list_len(state->selected_tracks);
		if (state->selection_anchor == track)
			state->selection_anchor = remaining > 0 ? state->selected_tracks[remaining - 1] : NULL;
		return;
	}

	naui_list_push(state->selected_tracks, track);
	state->selection_anchor = track;
	uph__song_timeline_remember_record_track(track);
}

static void uph__song_timeline_select_track_range(Uph_Track *to)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_Track *from = state->selection_anchor;
	if (!from || from->parent != to->parent)
	{
		uph__song_timeline_select_track(to);
		return;
	}

	Naui_List(Uph_Track) list = to->parent ? to->parent->subtracks : uph_state.project.tracks;
	const uint32_t low = from->index < to->index ? from->index : to->index;
	const uint32_t high = from->index < to->index ? to->index : from->index;

	naui_list_clear(state->selected_tracks);
	for (uint32_t i = low; i <= high && i < (uint32_t)naui_list_len(list); i++)
		naui_list_push(state->selected_tracks, &list[i]);
	uph__song_timeline_remember_record_track(to);
}

static void uph__song_timeline_click_select(Uph_Track *track, bool allow_multi)
{
	if (allow_multi && uph_ui_ctrl_down())
		uph__song_timeline_toggle_track(track);
	else if (allow_multi && uph_ui_shift_down())
		uph__song_timeline_select_track_range(track);
	else
		uph__song_timeline_select_track(track);
}

static bool uph__song_timeline_press_keeps_selection(const Uph_Track *track)
{
	return uph__song_timeline_track_selected(track)
		&& naui_list_len(uph__song_timeline_state.selected_tracks) >= 2
		&& !uph_ui_ctrl_down()
		&& !uph_ui_shift_down();
}

#pragma endregion

#pragma region Header

static void uph__song_timeline_begin_rename(Uph_Track *track)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	state->disable_space_to_play = true;
	state->rename_track = track;
	state->rename_old_name = track->name;
}

static void uph__song_timeline_finish_rename(Uph_Track *track)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	if (!track->name.length)
		track->name = naui_string_from_cstr(NAUI_TR("song_timeline.track.title"));

	if (!naui_strings_equal(track->name, state->rename_old_name, true))
	{
		Uph_ActionTrackRename data = {
			.track = uph_action_track_ref(track),
			.old_name = state->rename_old_name,
			.new_name = track->name
		};
		naui_action_execute_stack(UPH_ACTION_TRACK_RENAME, data);
	}

	state->disable_space_to_play = false;
	state->rename_track = NULL;
}

static void uph__song_timeline_open_options(Uph_Track *track, Uph_UIMenuID options_menu)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	if (track && !uph__song_timeline_track_selected(track))
		uph__song_timeline_click_select(track, false);

	state->current_options_track = track;
	uph_ui_open_context_menu(options_menu);
}

static void uph__song_timeline_open_instrument_list(Uph_Track *track)
{
	uph_state.shared.plugin_list_target = UPH_PLUGIN_LIST_TRACK_INSTRUMENT;
	uph_state.shared.current_plugin_list_track = track;
	naui_panel_open(uph_state.panels.plugin_list);
}

static Leaf_ID uph__song_timeline_track_name_text_id(const Uph_Track *track)
{
	return leaf_id_indexed("uph_song_timeline_name_text", (uint64_t)track);
}

static void uph__song_timeline_press_header(Uph_Track *track)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const bool was_record_track = uph_state.shared.record_track == track;
	const bool title_hovered = leaf_hovered_clipped(uph__song_timeline_track_name_text_id(track));
	const bool keep_selection = uph__song_timeline_press_keeps_selection(track);
	if (!keep_selection)
		uph__song_timeline_click_select(track, true);

	const bool master = track->type == UPH_RESOURCE_MASTER;
	state->track_drag = (Uph_TrackDragState){
		.source = uph_action_track_ref(track),
		.press_position = { (float)naui_mouse_x(), (float)naui_mouse_y() },
		.pending = true,
		.locked = master,
		.select_on_release = keep_selection && !master,
		.deselect_record_on_release = was_record_track && !title_hovered
	};
}

static void uph__song_timeline_update_header_input(Uph_Track *track, Leaf_ID header_id, Uph_UIMenuID options_menu)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	if (state->rename_track == track || uph_ui_any_widget_hovered())
		return;

	if (!state->panel_hovered || !leaf_hovered_clipped(header_id))
		return;

	if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		uph__song_timeline_press_header(track);
	else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
		uph__song_timeline_open_options(track, options_menu);
}

static const char *uph__song_timeline_automation_target_name(const Uph_Track *track)
{
	switch (track->automation_target)
	{
		case UPH_AUTOMATION_TARGET_PLUGIN_PARAM: return track->automation_param ? track->automation_param->name.data : NULL;
		case UPH_AUTOMATION_TARGET_TRACK_VOLUME: return NAUI_TR("automation.target.volume");
		case UPH_AUTOMATION_TARGET_TRACK_PAN: return NAUI_TR("automation.target.pan");
		case UPH_AUTOMATION_TARGET_TEMPO: return NAUI_TR("automation.target.tempo");
	}
	return NULL;
}

static Naui_Image *uph__song_timeline_track_icon(Uph_ResourceType type)
{
	switch (type)
	{
		case UPH_RESOURCE_SAMPLE: return uph_state.icons.wave;
		case UPH_RESOURCE_PATTERN: return uph_state.icons.piano;
		case UPH_RESOURCE_AUTOMATION: return uph_state.icons.automation;
		case UPH_RESOURCE_GROUP: return uph_state.icons.folder;
		case UPH_RESOURCE_MASTER: return uph_state.icons.headphones;
		default: return NULL;
	}
}

static Leaf_Color uph__song_timeline_header_color(const Uph_Track *track)
{
	const Leaf_Color base = uph_state.colors.song_timeline_header;
	const bool selected = uph__song_timeline_track_selected(track);

	const Uph_Track *tint = track->type == UPH_RESOURCE_GROUP ? track : track->parent;
	if (track->type == UPH_RESOURCE_AUTOMATION && tint && tint->type != UPH_RESOURCE_GROUP)
		tint = tint->parent;

	if (tint)
		return uph_color_mix(base, uph_resources_track_color(tint->color_index), selected ? 0.22f : 0.09f);

	return selected ? uph_state.colors.song_timeline_header_selected : base;
}

static Naui_Vec2 uph__song_timeline_toggle_size(void)
{
	const int32_t size = NAUI_DPI(UPH_UI_FONT_SIZE);
	return (Naui_Vec2){ size, size };
}

static Naui_Vec2 uph__song_timeline_icon_button_size(void)
{
	const int32_t size = (int32_t)NAUI_DPI(UPH_UI_FONT_SIZE) + NAUI_DPI(1);
	return (Naui_Vec2){ size, size };
}

static bool uph__song_timeline_icon_button(Naui_Image *icon, const char *id, const Uph_Track *track)
{
	return uph_ui_image_button_ex(icon, leaf_id_indexed(id, (uint64_t)track), uph__song_timeline_icon_button_size(), uph_state.colors.ui_text, LEAF_COLOR_TRANSPARENT, NAUI_CORNER_ALL);
}

static bool uph__song_timeline_toggle(Naui_Image *icon, const char *id, const Uph_Track *track, bool enabled)
{
	return uph_ui_image_toggle_button(icon, leaf_id_indexed(id, (uint64_t)track), uph__song_timeline_toggle_size(), uph_state.colors.ui_text, enabled);
}

static void uph__song_timeline_mute_toggle(Uph_Track *track)
{
	if (uph__song_timeline_toggle(uph_state.icons.mute, "uph_song_timeline_mute_toggle", track, track->state & UPH_TRACK_MUTED))
		uph_resources_toggle_track_state(track, UPH_TRACK_MUTED, UPH_ACTION_TRACK_MUTE);
}

static void uph__song_timeline_solo_toggle(Uph_Track *track)
{
	if (uph__song_timeline_toggle(uph_state.icons.solo, "uph_song_timeline_solo_toggle", track, uph_state.project.soloed_track == track))
	{
		Uph_ActionTrackSolo data = { .track = uph_action_track_ref(track) };
		naui_action_execute_stack(UPH_ACTION_TRACK_SOLO, data);
	}
}

static void uph__song_timeline_render_color_strip(Uph_Track *track, Uph_UIMenuID color_menu)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const Leaf_ID strip_id = leaf_id_indexed("uph_track_head_strip", (uint64_t)track);
	const bool hovered = uph_ui_widget_hovered(strip_id);
	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
		{
			state->current_color_options_track = track;
			uph_ui_open_context_menu(color_menu);
		}
	}

	const int32_t hit_width = (int32_t)NAUI_DPI(UPH_TRACK_STRIP_HIT_WIDTH);
	const int32_t width = (int32_t)NAUI_DPI(hovered ? UPH_TRACK_STRIP_HOVER_WIDTH : UPH_TRACK_STRIP_WIDTH);

	leaf({
		.id = strip_id,
		.size = { .width = LEAF_SIZE_FIXED(hit_width), .height = LEAF_SIZE_FULL }
	})
	leaf({
		.size = { .width = LEAF_SIZE_FIXED(width), .height = LEAF_SIZE_FULL },
		.color = uph_resources_track_color(track->color_index),
		.rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
	});
}

static void uph__song_timeline_render_track_name(Uph_Track *track)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const Leaf_Color text_color = uph_state.colors.ui_text;
	const Leaf_ID name_id = leaf_id_indexed("uph_song_timeline_name", (uint64_t)track);
	const Leaf_ID text_id = uph__song_timeline_track_name_text_id(track);

	if (track->type == UPH_RESOURCE_MASTER)
	{
		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
			.clip_children = true
		})
		leaf({ .id = text_id, .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT } })
		leaf_text(NAUI_TR("master.track.title"), {
			.color = text_color,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(14))
		});
		return;
	}

	if (state->rename_track == track)
	{
		if (uph_ui_textfield(&track->name, name_id, UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE | UPH_UI_TEXTFIELD_FLAG_TINTED, NAUI_TR("song_timeline.track.title")))
			uph__song_timeline_finish_rename(track);
		return;
	}

	if (state->panel_hovered && leaf_hovered_clipped(name_id) && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
		uph__song_timeline_begin_rename(track);

	leaf({
		.id = name_id,
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
		.clip_children = true
	})
	leaf({ .id = text_id, .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT } })
	leaf_text(track->name.data, {
		.color = text_color,
		.font_size = LEAF_SIZE_FIXED(NAUI_DPI(14))
	});
}

static void uph__song_timeline_render_title_row(Uph_Track *track, Uph_UIMenuID options_menu, bool compact)
{
	Naui_Image *icon = uph__song_timeline_track_icon(track->type);
	const int32_t icon_size = NAUI_DPI(UPH_SONG_TIMELINE_ICON_SIZE);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED((int32_t)NAUI_DPI(UPH_UI_FRAME_HEIGHT)) },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(6),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		if (icon)
		{
			leaf({
				.image = icon,
				.size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
				.color = uph_state.colors.ui_text
			});
		}

		uph__song_timeline_render_track_name(track);

		if (uph_track_is_folder(track))
		{
			Naui_Image *arrow = track->state & UPH_TRACK_TIMELINE_COLLAPSED ? uph_state.icons.arrow_right : uph_state.icons.arrow_down;
			if (uph__song_timeline_icon_button(arrow, "uph_song_timeline_collapse", track))
				track->state ^= UPH_TRACK_TIMELINE_COLLAPSED;
		}

		if (compact)
		{
			uph__song_timeline_mute_toggle(track);

			if (track->type != UPH_RESOURCE_MASTER && track->type != UPH_RESOURCE_AUTOMATION)
				uph__song_timeline_solo_toggle(track);
		}

		if (uph__song_timeline_icon_button(uph_state.icons.gear, "uph_song_timeline_options", track))
			uph__song_timeline_open_options(track, options_menu);
	}
}

static void uph__song_timeline_render_pan_knob(Uph_Track *track)
{
	const float old_pan = track->pan;
	float shown_pan = old_pan;
	const bool automated = uph_track_playing_automation_value(track, UPH_AUTOMATION_TARGET_TRACK_PAN, &shown_pan);

	leaf({ .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_TRACK_HEADER_KNOB_SIZE)), .height = LEAF_SIZE_FIT } })
	{
		const Naui_Color color = uph_resources_track_color(track->color_index);
		if (uph_ui_knob_float(&shown_pan, leaf_id_indexed("uph_song_timeline_pan", (uint64_t)track), -1.0f, 1.0f, 0.0f, color, UPH_UI_KNOB_START_TYPE_CENTER) && !automated)
		{
			uph_action_track_value_begin(uph_action_track_ref(track), UPH_ACTION_TRACK_VALUE_PAN, old_pan);
			track->pan = shown_pan;
		}
	}
}

static void uph__song_timeline_render_volume_slider(Uph_Track *track)
{
	float shown_volume = track->volume;
	const bool automated = uph_track_playing_automation_value(track, UPH_AUTOMATION_TARGET_TRACK_VOLUME, &shown_volume);
	float volume_db = uph_linear_to_db(shown_volume);

	if (uph_ui_slider_float_ex(&volume_db, leaf_id_indexed("uph_song_timeline_volume", (uint64_t)track), UPH_MIXER_DB_MIN, UPH_MIXER_DB_MAX, "%.1f dB", UPH_UI_SLIDER_FLAG_NONE, uph__song_timeline_header_color(track), uph_resources_track_color(track->color_index)) && !automated)
	{
		uph_action_track_value_begin(uph_action_track_ref(track), UPH_ACTION_TRACK_VALUE_VOLUME, track->volume);
		track->volume = uph_db_to_linear(volume_db);
	}
}

static void uph__song_timeline_render_mix_row(Uph_Track *track)
{
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(6),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		uph__song_timeline_render_pan_knob(track);
		uph__song_timeline_render_volume_slider(track);
		uph__song_timeline_mute_toggle(track);

		if (track->type != UPH_RESOURCE_MASTER)
			uph__song_timeline_solo_toggle(track);
	}

	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		uph_action_track_value_commit();
}

static void uph__song_timeline_render_automation_row(Uph_Track *track)
{
	const char *target_name = uph__song_timeline_automation_target_name(track);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(6),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		uph__song_timeline_mute_toggle(track);

		if (target_name)
		{
			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
				.padding = { .left = NAUI_DPI(4) },
				.clip_children = true
			})
			leaf_text(target_name, {
				.color = uph_state.colors.ui_text_disabled,
				.font_size = LEAF_SIZE_FIXED(NAUI_DPI(14))
			});
		}
	}
}

static void uph__song_timeline_render_instrument_row(Uph_Track *track)
{
	const char *label = track->instrument.loaded ? track->instrument.name.data : NAUI_TR("plugin.menu.instrument");
	if (!uph_ui_tinted_text_button_full(label, leaf_id_indexed("uph_song_timeline_instrument", (uint64_t)track)))
		return;

	if (!track->instrument.loaded)
		uph__song_timeline_open_instrument_list(track);
	else if (uph_plugin_window_visible(&track->instrument))
		uph_plugin_hide_window(&track->instrument);
	else
		uph_plugin_show_window(&track->instrument);
}

static void uph__song_timeline_render_meter(const Uph_Track *track)
{
	Uph_LevelMeterData meter_data = {
		.track = uph_action_track_ref(track),
		.low_color = uph_state.colors.song_timeline_meter_low,
		.medium_color = uph_state.colors.song_timeline_meter_medium,
		.high_color = uph_state.colors.song_timeline_meter_high
	};
	leaf({
		.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(UPH_TRACK_HEADER_METER_PADDING)),
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_LEFT,
			.color = uph_state.colors.song_timeline_header_card_border
		}
	})
	leaf({
		.custom_draw = (Leaf_CustomDrawFn)uph_level_meter_draw,
		.custom_draw_data = LEAF_DATA_SLICE(meter_data),
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_TRACK_HEADER_METER_WIDTH)), .height = LEAF_SIZE_FULL },
		.color = uph_state.colors.ui_well_bg
	});
}

static void uph__song_timeline_render_header_rows(Uph_Track *track, Uph_UIMenuID options_menu, bool compact)
{
	uph__song_timeline_render_title_row(track, options_menu, compact);

	if (compact || uph_track_is_folder(track))
		return;

	if (track->type == UPH_RESOURCE_AUTOMATION)
	{
		uph__song_timeline_render_automation_row(track);
		return;
	}

	uph__song_timeline_render_mix_row(track);

	if (track->type == UPH_RESOURCE_NONE || track->type == UPH_RESOURCE_PATTERN)
		uph__song_timeline_render_instrument_row(track);
}

static const Uph_Track *uph__song_timeline_next_row(const Uph_Track *track);

static void uph__song_timeline_render_track_header(Uph_Track *track, float depth, Uph_UIMenuID options_menu, Uph_UIMenuID color_menu)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const int32_t inset_y = NAUI_DPI(UPH_TRACK_HEADER_CARD_INSET_Y);
	const int32_t inset_edge = NAUI_DPI(UPH_TRACK_HEADER_CARD_INSET_EDGE);
	const int32_t inset_top = state->visual.settled_row_y == 0.0f ? inset_edge : inset_y;
	const int32_t inset_bottom = uph__song_timeline_next_row(track) ? inset_y : inset_edge;
	const Naui_Vec2 padding = UPH_SONG_TIMELINE_HEADER_PADDING;
	const int32_t strip_overhang = (int32_t)NAUI_DPI(UPH_TRACK_STRIP_HIT_WIDTH) - (int32_t)NAUI_DPI(UPH_TRACK_STRIP_WIDTH);
	const int32_t content_height = (int32_t)NAUI_DPI(state->zoom.y) - inset_edge * 2 - (int32_t)NAUI_DPI(UPH_TRACK_HEADER_PADDING_Y) * 2 - (int32_t)NAUI_DPI(UPH_TRACK_HEADER_CONTENT_TOP_PADDING);
	const int32_t full_rows_height = (int32_t)NAUI_DPI(UPH_UI_FRAME_HEIGHT) * 3 + (int32_t)NAUI_DPI(UPH_TRACK_HEADER_ROW_GAP) * 2;
	const bool folder = uph_track_is_folder(track);
	const bool compact = !folder && content_height < full_rows_height;
	const Leaf_ID header_id = leaf_id_indexed("uph_song_timeline_header", (uint64_t)track);

	if (state->panel_hovered && leaf_hovered_clipped(header_id))
		state->track_header_hovered = true;

	leaf({
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_header_column_width())), .height = LEAF_SIZE_FULL },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(depth * UPH_SONG_TIMELINE_DEPTH_INDENT)), .height = LEAF_SIZE_FULL }
		});

		leaf({
			.id = header_id,
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL },
			.padding = {
				.left = NAUI_DPI(UPH_TRACK_HEADER_CARD_INSET),
				.right = NAUI_DPI(UPH_TRACK_HEADER_CARD_INSET),
				.top = inset_top,
				.bottom = inset_bottom
			},
			.color = uph_state.colors.panel_body_bg
		})
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
			.border = {
				.width = uph_state.shared.record_track == track ? NAUI_DPI(2) : 1,
				.sides = LEAF_SIDE_ALL,
				.color = uph_state.shared.record_track == track ? uph_state.colors.song_timeline_last_track_border : uph_state.colors.song_timeline_header_card_border
			},
			.color = uph__song_timeline_header_color(track),
			.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_TRACK_HEADER_CARD_ROUNDING), LEAF_CORNER_ALL),
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.clip_children = true
		})
		{
			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL },
				.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(UPH_TRACK_HEADER_PADDING_Y)),
				.child_gap = (int32_t)NAUI_DPI(UPH_TRACK_HEADER_CHILD_GAP) - strip_overhang,
				.direction = LEAF_DIRECTION_HORIZONTAL
			})
			{
				if (track->type != UPH_RESOURCE_MASTER)
					uph__song_timeline_render_color_strip(track, color_menu);

				leaf({
					.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL },
					.padding = { .top = compact ? 0 : NAUI_DPI(UPH_TRACK_HEADER_CONTENT_TOP_PADDING) },
					.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = folder || compact ? LEAF_ALIGN_Y_CENTER : LEAF_ALIGN_Y_TOP },
					.child_gap = NAUI_DPI(UPH_TRACK_HEADER_ROW_GAP)
				})
				uph__song_timeline_render_header_rows(track, options_menu, compact);
			}

			if (track->type != UPH_RESOURCE_AUTOMATION)
				uph__song_timeline_render_meter(track);

			if (!uph_track_is_audible(&uph_state.project, track))
			{
				const Leaf_Color header_color = uph__song_timeline_header_color(track);
				leaf({
					.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
					.color = leaf_rgba(header_color.r, header_color.g, header_color.b, 120),
					.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_TRACK_HEADER_CARD_ROUNDING), LEAF_CORNER_ALL),
					.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
				});
			}

			uph__song_timeline_update_header_input(track, header_id, options_menu);
		}
	}
}

#pragma endregion

#pragma region Rows

static float uph__song_timeline_approach(float current, float target, float speed, float epsilon)
{
	const float t = naui_app_animations_enabled() ? 1.0f - expf(-speed * naui_delta_time()) : 1.0f;
	const float next = current + (target - current) * t;
	return fabsf(target - next) < epsilon ? target : next;
}

void uph_song_timeline_render_track(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu, Uph_UIMenuID color_menu)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	const bool folder = uph_track_is_folder(track);
	const float full_height = folder ? NAUI_DPI(UPH_SONG_TIMELINE_FOLDER_ROW_HEIGHT) : NAUI_DPI(state->zoom.y);
	const float target_y = state->visual.settled_row_y;

	if (track->visual.lifetime <= 0.0f || state->visual.snap_track_motion)
	{
		track->visual.y = target_y;
		track->visual.depth = (float)depth;
	}

	track->visual.lifetime = naui_app_animations_enabled() ? NAUI_CLAMP01(track->visual.lifetime + naui_delta_time() * NAUI_UI_ANIM_SPEED * 1.4f) : 1.0f;
	track->visual.y = uph__song_timeline_approach(track->visual.y, target_y, NAUI_UI_MOTION_SPEED, 0.5f);
	track->visual.depth = uph__song_timeline_approach(track->visual.depth, (float)depth, NAUI_UI_MOTION_SPEED, 0.01f);

	const float appear = naui_ease_out_cubic(track->visual.lifetime);
	const float row_height = floorf(full_height * appear + 0.5f);

	leaf({
		.id = leaf_id_indexed("uph_song_timeline_row", (uint64_t)track),
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(row_height) },
		.child_offset = { .x = (1.0f - appear) * NAUI_DPI(-12), .y = target_y - track->visual.y },
		.transparency = 1.0f - appear,
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.clip_children = appear < 1.0f
	})
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(full_height) },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		uph__song_timeline_render_track_header(track, track->visual.depth, options_menu, color_menu);
		if (!folder)
			uph_song_timeline_render_track_timeline(track);
	}

	state->visual.row_y += row_height;
	state->visual.settled_row_y += full_height;

	if (folder && (track->state & UPH_TRACK_TIMELINE_COLLAPSED))
		return;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		state->visual.row_counter++;
		uph_song_timeline_render_track(&track->subtracks[i], depth + 1, options_menu, color_menu);
	}
}

void uph_song_timeline_render_track_plus(void)
{
	const Naui_Vec2 padding = UPH_SONG_TIMELINE_HEADER_PADDING;
	const int32_t header_width = NAUI_DPI(UPH_SONG_TIMELINE_HEADER_WIDTH + uph_song_timeline_header_depth_width());
	const Leaf_ID id = leaf_id("uph_song_timeline_plus");
	const bool hovered = uph_ui_widget_hovered(id);

	if (hovered)
	{
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			naui_action_execute_stack(UPH_ACTION_TRACK_CREATE, (Uph_ActionTrackCreate){0});

		naui_set_cursor(NAUI_CURSOR_HAND);
	}

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIXED(header_width), .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_ALL,
			.color = uph_state.colors.song_timeline_header_card_border
		},
		.color = hovered ? uph_state.colors.ui_frame_hovered_bg : uph_state.colors.song_timeline_header,
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(6), LEAF_CORNER_BL | LEAF_CORNER_BR)
	})
	leaf({
		.image = uph_state.icons.plus,
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(13)), .height = LEAF_SIZE_DERIVED },
		.color = uph_state.colors.ui_text,
		.aspect_ratio = 1.0f
	});
}

#pragma endregion

#pragma region Menus

void uph_song_timeline_render_track_color_menu(Uph_UIMenuID menu)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_Track *track = state->current_color_options_track;
	if (naui_list_len(uph_state.project.tracks) == 0 || !track)
		return;

	uph_resources_set_track_color(track, uph_ui_track_color_palette(track));
}

static Naui_List(Uph_TrackPtr) uph__song_timeline_option_targets(Uph_Track *track)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Naui_List(Uph_TrackPtr) targets = NULL;
	if (!uph__song_timeline_track_selected(track))
	{
		naui_list_push(targets, track);
		return targets;
	}

	Naui_List(Uph_TrackPtr) selected = state->selected_tracks;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
		naui_list_push(targets, selected[i]);

	return targets;
}

static bool uph__song_timeline_render_group_items(Uph_UIMenuID menu, Uph_Track *track, Naui_List(Uph_TrackPtr) targets)
{
	Naui_List(Uph_TrackPtr) groupable = NULL;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(targets); i++)
	{
		if (targets[i]->type != UPH_RESOURCE_AUTOMATION)
			naui_list_push(groupable, targets[i]);
	}

	Uph_Track *group = NULL;
	if (uph_resources_count_groupable_tracks(groupable) >= 1 && uph_ui_menu_item(menu, "Group Tracks", leaf_id("uph_song_timeline_options_group")))
		group = uph_resources_group_tracks(groupable, "Untitled Group", track->color_index);

	naui_list_free(groupable);

	if (group)
	{
		uph__song_timeline_select_track(group);
		return true;
	}

	if (track->type != UPH_RESOURCE_GROUP || !uph_ui_menu_item(menu, "Ungroup", leaf_id("uph_song_timeline_options_ungroup")))
		return false;

	Naui_List(Uph_TrackPtr) children = NULL;
	const bool ungrouped = uph_resources_ungroup_track(track, &children);
	if (ungrouped)
		uph__song_timeline_set_selection(children);

	naui_list_free(children);
	return ungrouped;
}

static void uph__song_timeline_render_instrument_items(Uph_UIMenuID menu, Uph_Track *track)
{
	if (!track->instrument.loaded)
	{
		if (uph_ui_menu_item(menu, "Load Instrument", leaf_id("uph_song_timeline_options_load_instrument")))
			uph__song_timeline_open_instrument_list(track);
		return;
	}

	const bool visible = uph_plugin_window_visible(&track->instrument);
	if (uph_ui_menu_item(menu, visible ? "Hide Instrument" : "Show Instrument", leaf_id("uph_song_timeline_options_show_instrument")))
	{
		if (visible)
			uph_plugin_hide_window(&track->instrument);
		else
			uph_plugin_show_window(&track->instrument);
	}

	if (uph_ui_menu_item(menu, "Remove Instrument", leaf_id("uph_song_timeline_options_remove_instrument")))
	{
		uph_action_track_unload_plugin(track, &track->instrument);
		if (naui_list_len(track->blocks) == 0)
			track->type = UPH_RESOURCE_NONE;
	}
}

static void uph__song_timeline_render_track_items(Uph_UIMenuID menu, Uph_UIMenuID color_menu, Uph_Track *track, Naui_List(Uph_TrackPtr) targets)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	if (track->type != UPH_RESOURCE_MASTER)
	{
		if (uph_ui_menu_item(menu, "Rename", leaf_id("uph_song_timeline_options_rename")))
			uph__song_timeline_begin_rename(track);

		if (uph_ui_menu_item(menu, "Color", leaf_id("uph_song_timeline_options_color")))
		{
			state->current_color_options_track = track;
			uph_ui_open_context_menu(color_menu);
		}
	}

	if (track->type == UPH_RESOURCE_NONE || track->type == UPH_RESOURCE_PATTERN)
		uph__song_timeline_render_instrument_items(menu, track);

	if (track->type != UPH_RESOURCE_AUTOMATION)
	{
		if (uph_ui_menu_item(menu, NAUI_TR("automation.menu.automate"), leaf_id("uph_song_timeline_options_automate")))
			uph_song_timeline_open_automation_list(track);

		if (uph_song_timeline_can_automate_last_touched(track) &&
			uph_ui_menu_item(menu, NAUI_TR("automation.menu.automate_last"), leaf_id("uph_song_timeline_options_automate_last")))
			uph_song_timeline_automate_last_touched(track);
	}

	if (track->type != UPH_RESOURCE_MASTER && uph_ui_menu_item(menu, "Remove", leaf_id("uph_song_timeline_options_remove")))
		uph_resources_delete_tracks(targets);
}

void uph_song_timeline_render_track_options_menu(Uph_UIMenuID menu, Uph_UIMenuID color_menu)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_Track *track = state->current_options_track;
	if (naui_list_len(uph_state.project.tracks) == 0 || !track)
		return;

	Naui_List(Uph_TrackPtr) targets = uph__song_timeline_option_targets(track);
	if (!uph__song_timeline_render_group_items(menu, track, targets))
		uph__song_timeline_render_track_items(menu, color_menu, track, targets);

	naui_list_free(targets);
}

#pragma endregion

#pragma region Track Drag

static bool uph__song_timeline_row_box(const Uph_Track *track, Leaf_BoundingBox *out)
{
	*out = leaf_get_bounding_box(leaf_id_indexed("uph_song_timeline_row", (uint64_t)track));
	return out->height > 0.0f;
}

static Leaf_BoundingBox uph__song_timeline_subtree_box(const Uph_Track *track, Leaf_BoundingBox box)
{
	const uint32_t count = (uint32_t)naui_list_len(track->subtracks);
	if (count == 0 || (uph_track_is_folder(track) && (track->state & UPH_TRACK_TIMELINE_COLLAPSED)))
		return box;

	const Uph_Track *last_track = &track->subtracks[count - 1];
	Leaf_BoundingBox last;
	if (!uph__song_timeline_row_box(last_track, &last))
		return box;

	last = uph__song_timeline_subtree_box(last_track, last);
	box.height = last.y + last.height - box.y;
	return box;
}

static bool uph__song_timeline_drop_on_row(Uph_TrackDropTarget *target, float mouse_y, Leaf_BoundingBox box, Uph_ActionTrackRef owner, uint32_t index)
{
	if (mouse_y >= box.y + box.height)
		return false;

	const bool after = mouse_y >= box.y + box.height * 0.5f;
	*target = (Uph_TrackDropTarget){
		.owner = owner,
		.index = after ? index + 1 : index,
		.row = box,
		.after = after,
		.valid = true
	};
	return true;
}

static Uph_TrackDropTarget uph__song_timeline_drop_after(Uph_ActionTrackRef owner, uint32_t index, Leaf_BoundingBox row)
{
	if (row.height <= 0.0f)
		return (Uph_TrackDropTarget){ 0 };

	return (Uph_TrackDropTarget){
		.owner = owner,
		.index = index,
		.row = row,
		.after = true,
		.valid = true
	};
}

static bool uph__song_timeline_find_drop_in_list(Uph_TrackDropTarget *target, float mouse_y, Naui_List(Uph_Track) tracks, Uph_ActionTrackRef owner, Naui_List(Uph_TrackPtr) dragged, Leaf_BoundingBox *last_box)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		Uph_Track *track = &tracks[i];
		if (track->type == UPH_RESOURCE_MASTER)
			continue;

		Leaf_BoundingBox box;
		if (!uph__song_timeline_row_box(track, &box))
			continue;

		const bool open_group = track->type == UPH_RESOURCE_GROUP && !(track->state & UPH_TRACK_TIMELINE_COLLAPSED) && !uph__track_list_contains(dragged, track);
		if (!open_group)
			box = uph__song_timeline_subtree_box(track, box);

		*last_box = box;

		const Uph_ActionTrackRef group_ref = uph_action_track_ref_child(owner, i);
		if (uph__song_timeline_drop_on_row(target, mouse_y, box, owner, i))
		{
			if (open_group && target->index > i)
			{
				target->owner = group_ref;
				target->index = 0;
			}
			return true;
		}

		if (open_group && uph__song_timeline_find_drop_in_list(target, mouse_y, track->subtracks, group_ref, dragged, last_box))
			return true;
	}

	return false;
}

static Uph_TrackDropTarget uph__song_timeline_find_drop_target(float mouse_y, Naui_List(Uph_TrackPtr) dragged)
{
	Uph_TrackDropTarget target = { 0 };
	Leaf_BoundingBox last_box = { 0 };

	if (uph__song_timeline_find_drop_in_list(&target, mouse_y, uph_state.project.tracks, uph_action_track_ref_root(), dragged, &last_box))
		return target;

	return uph__song_timeline_drop_after(uph_action_track_ref_root(), (uint32_t)naui_list_len(uph_state.project.tracks), last_box);
}

static Uph_TrackDropTarget uph__song_timeline_find_sibling_drop_target(float mouse_y, const Uph_Track *source)
{
	Uph_Track *parent = source->parent;
	Naui_List(Uph_Track) siblings = parent ? parent->subtracks : uph_state.project.tracks;
	const Uph_ActionTrackRef owner = parent ? uph_action_track_ref(parent) : uph_action_track_ref_root();
	const uint32_t count = (uint32_t)naui_list_len(siblings);

	Uph_TrackDropTarget target = { 0 };
	Leaf_BoundingBox last_box = { 0 };
	for (uint32_t i = 0; i < count; i++)
	{
		Leaf_BoundingBox box;
		if (!uph__song_timeline_row_box(&siblings[i], &box))
			continue;

		last_box = uph__song_timeline_subtree_box(&siblings[i], box);
		if (uph__song_timeline_drop_on_row(&target, mouse_y, last_box, owner, i))
			return target;
	}

	return uph__song_timeline_drop_after(owner, count, last_box);
}

static Naui_List(Uph_TrackPtr) uph__song_timeline_drag_tracks(Uph_Track *source)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Naui_List(Uph_TrackPtr) tracks = NULL;
	if (!uph__song_timeline_track_selected(source))
	{
		naui_list_push(tracks, source);
		return tracks;
	}

	const bool automation = source->type == UPH_RESOURCE_AUTOMATION;
	Naui_List(Uph_TrackPtr) selected = state->selected_tracks;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		const Uph_Track *track = selected[i];
		const bool compatible = automation
			? track->parent == source->parent
			: track->type != UPH_RESOURCE_AUTOMATION && track->type != UPH_RESOURCE_MASTER;

		if (compatible)
			naui_list_push(tracks, selected[i]);
	}

	return tracks;
}

static bool uph__song_timeline_drag_passed_threshold(Naui_Vec2 press, Naui_Vec2 mouse)
{
	const float dx = mouse.x - press.x;
	const float dy = mouse.y - press.y;
	const int32_t threshold = NAUI_DPI(UPH_SONG_TIMELINE_TRACK_DRAG_THRESHOLD);
	return dx * dx + dy * dy >= threshold * threshold;
}

static void uph__song_timeline_queue_move(Naui_List(Uph_TrackPtr) tracks, Uph_TrackDropTarget target)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_TrackPendingMove *move = &state->pending_move;
	naui_list_free(move->tracks);
	*move = (Uph_TrackPendingMove){
		.tracks = tracks,
		.dest_group = target.owner.depth == 0 ? NULL : uph_action_track_resolve(target.owner),
		.dest_index = target.index
	};
}

static void uph__song_timeline_render_drop_line(Uph_TrackDropTarget target)
{
	const int32_t indent = NAUI_DPI(target.owner.depth * UPH_SONG_TIMELINE_DEPTH_INDENT);
	const int32_t line_height = NAUI_DPI(3);
	const float line_y = target.after ? target.row.y + target.row.height : target.row.y;

	leaf({
		.size = { .width = LEAF_SIZE_FIXED(target.row.width - indent), .height = LEAF_SIZE_FIXED(line_height) },
		.color = uph_state.colors.ui_slider_fill,
		.floating = { .offset = { .x = target.row.x + indent, .y = line_y - line_height * 0.5f } },
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	});
}

static void uph__song_timeline_render_drag_label(const Uph_Track *source, uint32_t carried, Naui_Vec2 mouse)
{
	char label[256];
	if (carried > 1)
		snprintf(label, sizeof(label), "%s +%u", source->name.data, carried - 1);
	else
		snprintf(label, sizeof(label), "%s", source->name.data);

	leaf({
		.padding = LEAF_PADDING_AXES(NAUI_DPI(8), NAUI_DPI(4)),
		.color = uph_state.colors.ui_dropdown_bg,
		.floating = { .offset = { .x = mouse.x + NAUI_DPI(12), .y = mouse.y + NAUI_DPI(12) } },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), LEAF_CORNER_ALL),
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT
	})
	leaf_text(label, {
		.color = uph_state.colors.ui_text,
		.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
	});
}

void uph_song_timeline_update_track_drag(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_TrackDragState *drag = &state->track_drag;

	if (naui_any_mouse_pressed() && !state->track_header_hovered && !uph_ui_any_menu_open())
		uph__song_timeline_select_track(NULL);

	if (!drag->pending && !drag->active)
		return;

	Uph_Track *source = uph_action_track_resolve(drag->source);
	if (!source || naui_key_pressed(NAUI_KEY_ESCAPE))
	{
		*drag = (Uph_TrackDragState){ 0 };
		return;
	}

	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
	const bool mouse_down = naui_mouse_down(NAUI_MOUSE_LEFT);

	if (drag->pending)
	{
		if (!mouse_down)
		{
			if (drag->select_on_release)
				uph__song_timeline_select_track(source);

			if (drag->deselect_record_on_release && uph_state.shared.record_track == source)
				uph_state.shared.record_track = NULL;

			*drag = (Uph_TrackDragState){ 0 };
			return;
		}

		if (!uph__song_timeline_drag_passed_threshold(drag->press_position, mouse))
			return;

		drag->deselect_record_on_release = false;
		if (drag->locked)
			return;

		drag->pending = false;
		drag->active = true;
	}

	const bool automation = source->type == UPH_RESOURCE_AUTOMATION;
	Naui_List(Uph_TrackPtr) dragged = uph__song_timeline_drag_tracks(source);

	Uph_TrackDropTarget target = { 0 };
	if (state->panel_hovered)
		target = automation ? uph__song_timeline_find_sibling_drop_target(mouse.y, source) : uph__song_timeline_find_drop_target(mouse.y, dragged);

	if (!mouse_down)
	{
		if (target.valid)
			uph__song_timeline_queue_move(dragged, target);
		else
			naui_list_free(dragged);

		*drag = (Uph_TrackDragState){ 0 };
		return;
	}

	const uint32_t carried = automation ? (uint32_t)naui_list_len(dragged) : uph_resources_count_movable_tracks(dragged);
	naui_list_free(dragged);

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (target.valid)
		uph__song_timeline_render_drop_line(target);

	uph__song_timeline_render_drag_label(source, carried, mouse);
}

void uph_song_timeline_apply_pending_move(void)
{
	Uph_SongTimelineState *state = &uph__song_timeline_state;
	Uph_TrackPendingMove *move = &state->pending_move;
	if (!move->tracks)
		return;

	Naui_List(Uph_TrackPtr) moved = NULL;
	if (uph_resources_move_tracks(move->tracks, move->dest_group, move->dest_index, &moved))
		uph__song_timeline_set_selection(moved);

	naui_list_free(moved);
	naui_list_free(move->tracks);
	*move = (Uph_TrackPendingMove){ 0 };
}

#pragma endregion
