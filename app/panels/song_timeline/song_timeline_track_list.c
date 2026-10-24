#define UPH_TRACK_STRIP_WIDTH		5.0f
#define UPH_TRACK_STRIP_HOVER_WIDTH	6.0f
#define UPH_TRACK_STRIP_HIT_WIDTH	10.0f
#define UPH_TRACK_HEADER_CHILD_GAP	10.0f

static void _uph_song_timeline_finish_rename(Uph_Track *track)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	if (!track->name.length)
		track->name = naui_string_from_cstr(NAUI_TR("song_timeline.track.title"));

	if (!naui_strings_equal(track->name, timeline->rename_old_name, true))
	{
		Uph_ActionTrackRename data = {
			.track = uph_action_track_ref(track),
			.old_name = timeline->rename_old_name,
			.new_name = track->name
		};
		naui_action_execute_stack(UPH_ACTION_TRACK_RENAME, data);
	}

	timeline->disable_space_to_play = false;
	timeline->rename_track = NULL;
}

static void _uph_song_timeline_begin_rename(Uph_Track *track)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	timeline->disable_space_to_play = true;
	timeline->rename_track = track;
	timeline->rename_old_name = track->name;
}

static bool _uph_song_timeline_track_selectable(const Uph_Track *track)
{
	return track && track->type != UPH_RESOURCE_AUTOMATION;
}

static bool _uph_song_timeline_track_selected(const Uph_Track *track)
{
	Naui_List(Uph_TrackPtr) selected = uph_song_timeline_data.selected_tracks;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(selected); i++)
	{
		if (selected[i] == track)
			return true;
	}
	return false;
}

static void _uph_song_timeline_set_selection(Naui_List(Uph_TrackPtr) tracks)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	naui_list_clear(timeline->selected_tracks);
	timeline->selection_anchor = NULL;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks); i++)
	{
		if (!_uph_song_timeline_track_selectable(tracks[i]))
			continue;

		naui_list_push(timeline->selected_tracks, tracks[i]);
		timeline->selection_anchor = tracks[i];
	}
}

static void _uph_song_timeline_select_track(Uph_Track *track)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	naui_list_clear(timeline->selected_tracks);
	timeline->selection_anchor = track;

	if (track)
		naui_list_push(timeline->selected_tracks, track);
}

static void _uph_song_timeline_toggle_track(Uph_Track *track)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(timeline->selected_tracks); i++)
	{
		if (timeline->selected_tracks[i] != track)
			continue;

		naui_list_remove(timeline->selected_tracks, i);

		const uint32_t remaining = (uint32_t)naui_list_len(timeline->selected_tracks);
		if (timeline->selection_anchor == track)
			timeline->selection_anchor = remaining > 0 ? timeline->selected_tracks[remaining - 1] : NULL;
		return;
	}

	naui_list_push(timeline->selected_tracks, track);
	timeline->selection_anchor = track;
}

static void _uph_song_timeline_select_track_range(Uph_Track *to)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	Uph_Track *from = timeline->selection_anchor;
	if (!from || from->parent != to->parent)
	{
		_uph_song_timeline_select_track(to);
		return;
	}

	Naui_List(Uph_Track) list = to->parent ? to->parent->subtracks : uph_state.project.tracks;
	const uint32_t low = from->index < to->index ? from->index : to->index;
	const uint32_t high = from->index < to->index ? to->index : from->index;

	naui_list_clear(timeline->selected_tracks);
	for (uint32_t i = low; i <= high && i < (uint32_t)naui_list_len(list); i++)
	{
		if (_uph_song_timeline_track_selectable(&list[i]))
			naui_list_push(timeline->selected_tracks, &list[i]);
	}
}

static void _uph_song_timeline_click_select(Uph_Track *track, const bool allow_multi)
{
	if (allow_multi && uph_ui_ctrl_down())
		_uph_song_timeline_toggle_track(track);
	else if (allow_multi && uph_ui_shift_down())
		_uph_song_timeline_select_track_range(track);
	else
		_uph_song_timeline_select_track(track);
}

static bool _uph_song_timeline_press_keeps_selection(const Uph_Track *track)
{
	return
		_uph_song_timeline_track_selected(track) &&
		naui_list_len(uph_song_timeline_data.selected_tracks) >= 2 &&
		!uph_ui_ctrl_down() &&
		!uph_ui_shift_down();
}

static void _uph_song_timeline_open_options(Uph_Track *track, Uph_UIMenuID options_menu)
{
	if (_uph_song_timeline_track_selectable(track) && !_uph_song_timeline_track_selected(track))
		_uph_song_timeline_click_select(track, false);

	uph_song_timeline_data.current_options_track = track;
	uph_ui_open_context_menu(options_menu);
}

static void _uph_song_timeline_open_instrument_list(Uph_Track *track)
{
	uph_state.shared.plugin_list_target = UPH_PLUGIN_LIST_TRACK_INSTRUMENT;
	uph_state.shared.current_plugin_list_track = track;
	naui_panel_open(uph_state.panels.plugin_list);
}

static void _uph_song_timeline_update_header_input(Uph_Track *track, const Leaf_ID header_id, Uph_UIMenuID options_menu)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	if (timeline->rename_track == track || uph_ui_any_widget_hovered())
		return;

	if (!timeline->panel_hovered || !leaf_hovered_clipped(header_id))
		return;

	const bool selectable = _uph_song_timeline_track_selectable(track);
	if (selectable && naui_mouse_pressed(NAUI_MOUSE_LEFT))
	{
		const bool keep_selection = _uph_song_timeline_press_keeps_selection(track);
		if (!keep_selection)
			_uph_song_timeline_click_select(track, true);

		timeline->track_drag = (Uph_TrackDragState){
			.source = uph_action_track_ref(track),
			.press_position = { (float)naui_mouse_x(), (float)naui_mouse_y() },
			.pending = true,
			.select_on_release = keep_selection
		};
	}
	else if (naui_mouse_pressed(NAUI_MOUSE_RIGHT))
		_uph_song_timeline_open_options(track, options_menu);
}

static void _uph_song_timeline_render_track_header(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu, Uph_UIMenuID color_menu)
{
	const float depth_offset = depth * UPH_SONG_TIMELINE_DEPTH_INDENT;
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const Leaf_Color base_bg_color = naui_theme_color("uph_song_timeline_header_color");
	const Leaf_Color border_color = naui_theme_color("uph_song_timeline_header_border_color");

	const Naui_Vec2 padding = naui_theme_vec2("uph_song_timeline_header_padding");

	const int32_t strip_idle_width = (int32_t)NAUI_DPI(UPH_TRACK_STRIP_WIDTH);
	const int32_t strip_hover_width = (int32_t)NAUI_DPI(UPH_TRACK_STRIP_HOVER_WIDTH);
	const int32_t strip_hit_width = (int32_t)NAUI_DPI(UPH_TRACK_STRIP_HIT_WIDTH);
	const int32_t header_child_gap = (int32_t)NAUI_DPI(UPH_TRACK_HEADER_CHILD_GAP) - (strip_hit_width - strip_idle_width);

	uint64_t track_id = (uint64_t)track;
	Naui_Color color = uph_resources_track_color(track->color_index);

	const Uph_Track *tint_track = track->type == UPH_RESOURCE_GROUP ? track : track->parent;
	if (track->type == UPH_RESOURCE_AUTOMATION && tint_track && tint_track->type != UPH_RESOURCE_GROUP)
		tint_track = tint_track->parent;
	const bool selected = _uph_song_timeline_track_selected(track);
	Leaf_Color bg_color = base_bg_color;
	if (tint_track)
		bg_color = uph_color_mix(base_bg_color, uph_resources_track_color(tint_track->color_index), selected ? 0.22f : 0.09f);
	else if (selected)
		bg_color = naui_theme_color("uph_song_timeline_header_selected_color");

	const Leaf_ID header_id = leaf_id_indexed("uph_song_timeline_header", track_id);
	if (uph_song_timeline_data.panel_hovered && leaf_hovered_clipped(header_id))
		uph_song_timeline_data.track_header_hovered = true;

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_header_column_width())), LEAF_SIZE_FULL}
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(depth_offset)), LEAF_SIZE_FULL}
		});
		leaf({
			.id = header_id,
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
			.color = bg_color,
			.child_gap = header_child_gap,
			.border = {
				.width = 1,
				.color = {border_color},
				.sides = LEAF_SIDE_ALL
			}
		})
		{

			const Leaf_ID color_strip_id = leaf_id_indexed("uph_track_head_strip", track_id);
			const bool strip_hovered = uph_ui_widget_hovered(color_strip_id);
			if (strip_hovered)
			{
				naui_set_cursor(NAUI_CURSOR_HAND);

				if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
				{
					uph_song_timeline_data.current_color_options_track = track;
					uph_ui_open_context_menu(color_menu);
				}
			}

			const int32_t strip_width = strip_hovered ? strip_hover_width : strip_idle_width;
			leaf({
				.id = color_strip_id,
				.size = {LEAF_SIZE_FIXED(strip_hit_width), LEAF_SIZE_FULL}
			})
			{
				leaf({
					.size = {LEAF_SIZE_FIXED(strip_width), LEAF_SIZE_FULL},
					.color = color,
					.rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
				});
			}

			leaf({
				.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
				.child_gap = NAUI_DPI(4)
			})
			{
				const int32_t button_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
				const int32_t title_height = button_size + NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").y) * 2;

				leaf({
					.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(title_height)},
					.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
					.direction = LEAF_DIRECTION_HORIZONTAL
				})
				{
					leaf({
						.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
						.direction = LEAF_DIRECTION_HORIZONTAL,
						.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
						.child_gap = NAUI_DPI(4)
					})
					{
						if (track->type != UPH_RESOURCE_NONE)
						{
							Naui_Image *icon;
							switch (track->type)
							{
								case UPH_RESOURCE_SAMPLE: icon = naui_asset_image("uph_icon_wave"); break;
								case UPH_RESOURCE_PATTERN: icon = naui_asset_image("uph_icon_piano"); break;
								case UPH_RESOURCE_AUTOMATION: icon = naui_asset_image("uph_icon_automation"); break;
								case UPH_RESOURCE_GROUP: icon = naui_asset_image("uph_icon_folder"); break;
							}
							const float icon_size = NAUI_DPI(naui_theme_float("uph_song_timeline_icon_size"));
							leaf({
								.size = {LEAF_SIZE_FIXED(icon_size), LEAF_SIZE_FIXED(icon_size)},
								.image = icon,
								.color = {text_color}
							});
						}

						Leaf_ID name_id = leaf_id_indexed("uph_song_timeline_name", track_id);

						if (uph_song_timeline_data.rename_track == track)
						{
							if (uph_ui_textfield(&track->name, name_id, UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE, NAUI_TR("song_timeline.track.title")))
								_uph_song_timeline_finish_rename(track);
						}
						else
						{
							if (uph_song_timeline_data.panel_hovered && leaf_hovered_clipped(name_id) && naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
								_uph_song_timeline_begin_rename(track);

							leaf({
								.id = name_id
							})
							{
								leaf_text(track->name.data, {
									.font_size = {NAUI_DPI(14)},
									.color = {text_color}
								});
							}
						}
					}

					if (uph_ui_image_button_ex(naui_asset_image("uph_icon_dots"), leaf_id_indexed("uph_song_timeline_options", track_id), (Naui_Vec2) { button_size, button_size }, text_color, LEAF_COLOR_TRANSPARENT, NAUI_CORNER_ALL))
						_uph_song_timeline_open_options(track, options_menu);

					if (track->type == UPH_RESOURCE_GROUP && uph_ui_text_button(track->state & UPH_TRACK_TIMELINE_COLLAPSED ? "+" : "-", leaf_id_indexed("uph_song_timeline_collapse", track_id)))
						track->state ^= UPH_TRACK_TIMELINE_COLLAPSED;

					if ((track->type == UPH_RESOURCE_NONE || track->type == UPH_RESOURCE_PATTERN) && uph_ui_image_button_ex(naui_asset_image("uph_icon_add_instrument"), leaf_id_indexed("uph_song_timeline_add_instrument", track_id), (Naui_Vec2) { button_size, button_size }, text_color, LEAF_COLOR_TRANSPARENT, NAUI_CORNER_ALL))
						_uph_song_timeline_open_instrument_list(track);

				}

				if (track->type != UPH_RESOURCE_GROUP)
				{
					leaf({
						.direction = LEAF_DIRECTION_HORIZONTAL,
						.child_gap = NAUI_DPI(2)
					})
					{
						if (uph_ui_text_toggle_button("M", leaf_id_indexed("uph_song_timeline_mute_toggle", track_id), track->state & UPH_TRACK_MUTED))
							uph_resources_toggle_track_state(track, UPH_TRACK_MUTED, UPH_ACTION_TRACK_MUTE);
						if (track->type != UPH_RESOURCE_AUTOMATION && uph_ui_text_toggle_button("S", leaf_id_indexed("uph_song_timeline_solo_toggle", track_id), uph_state.project.soloed_track == track))
						{
							Uph_ActionTrackSolo data = { .track = uph_action_track_ref(track) };
							naui_action_execute_stack(UPH_ACTION_TRACK_SOLO, data);
						}

						if (track->type != UPH_RESOURCE_AUTOMATION &&
							uph_ui_image_toggle_button(
								naui_asset_image("uph_icon_mic"),
								leaf_id_indexed("uph_song_timeline_arm_toggle", track_id),
								(Naui_Vec2) { button_size, button_size },
								text_color,
								track->state & UPH_TRACK_ARMED
						)) uph_resources_toggle_track_state(track, UPH_TRACK_ARMED, UPH_ACTION_TRACK_ARM);
					}
				}
			}

			_uph_song_timeline_update_header_input(track, header_id, options_menu);
		}
	}
	
}

void uph_song_timeline_render_track(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu, Uph_UIMenuID color_menu)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	leaf({
		.id = leaf_id_indexed("uph_song_timeline_row", (uint64_t)track),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(track->type == UPH_RESOURCE_GROUP ? NAUI_DPI(48) : NAUI_DPI(uph_song_timeline_data.zoom.y))}
	})
	{
		_uph_song_timeline_render_track_header(track, depth, options_menu, color_menu);
		if (track->type != UPH_RESOURCE_GROUP)
			uph_song_timeline_render_track_timeline(track);
	}

	if (track->type == UPH_RESOURCE_GROUP && (track->state & UPH_TRACK_TIMELINE_COLLAPSED))
		return;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		data->visual_row_counter++;
		uph_song_timeline_render_track(&track->subtracks[i], depth + 1, options_menu, color_menu);
	}
}

void uph_song_timeline_render_track_color_menu(Uph_UIMenuID track_color_options_context_menu)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	if (naui_list_len(uph_state.project.tracks) == 0 || !data->current_color_options_track)
		return;

	Uph_Track *track = data->current_color_options_track;
	const Naui_List(Naui_Color) palette = naui_theme_color_list("uph_track_palette");
	const uint32_t color_count = (uint32_t)naui_list_len(palette);
	if (color_count == 0)
		return;

	const uint32_t rows = 2;
	const uint32_t columns = (uint32_t)ceilf((color_count + rows - 1) / rows);
	const int32_t current = track->color_index % (int32_t)color_count;

	const uint32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")) * 1.5f;
	int32_t picked = -1;

	leaf({
		.child_gap = NAUI_DPI(4)
	})
	{
		for (uint32_t i = 0; i < rows; i++)
		{
			leaf({
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.child_gap = NAUI_DPI(4)
			})
			{
				for (uint32_t j = 0; j < columns; j++)
				{
					const int32_t index = j + i * columns;
					const Leaf_ID id = leaf_id_indexed("uph_track_color_picker", index);
					if (leaf_hovered(id) && naui_mouse_pressed(NAUI_MOUSE_LEFT))
					{
						picked = index;
						uph_ui_close_menu();
					}
					leaf({
						.id = id,
						.size = {LEAF_SIZE_FIXED(font_size), LEAF_SIZE_FIXED(font_size)},
						.color = palette[index],
						.border = {
							.width = NAUI_DPI(2),
							.color = track->color_index == index ?
								LEAF_COLOR_WHITE : LEAF_COLOR_TRANSPARENT,
							.sides = LEAF_SIDE_ALL
						}
					});
				}
			}
		}
	}

	if (picked >= 0 && track->color_index != picked)
	{
		Uph_ActionTrackColor color_data = {
			.track = uph_action_track_ref(track),
			.old_color = track->color_index,
			.new_color = picked
		};

		naui_action_execute_stack(UPH_ACTION_TRACK_COLOR, color_data);
	}
}

void uph_song_timeline_render_track_options_menu(Uph_UIMenuID track_options_context_menu)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	if (naui_list_len(uph_state.project.tracks) == 0 || !data->current_options_track)
		return;

	Uph_Track *track = data->current_options_track;

	Naui_List(Uph_TrackPtr) targets = NULL;
	if (_uph_song_timeline_track_selected(track))
	{
		for (uint32_t i = 0; i < (uint32_t)naui_list_len(data->selected_tracks); i++)
			naui_list_push(targets, data->selected_tracks[i]);
	}
	else
	{
		naui_list_push(targets, track);
	}

	bool restructured = false;

	if (uph_resources_count_groupable_tracks(targets) >= 1 &&
		uph_ui_menu_item(track_options_context_menu, "Group Tracks", leaf_id("uph_song_timeline_options_group")))
	{
		Uph_Track *group = uph_resources_group_tracks(targets, "Untitled Group", track->color_index);
		if (group)
		{
			_uph_song_timeline_select_track(group);
			restructured = true;
		}
	}

	if (!restructured && track->type == UPH_RESOURCE_GROUP &&
		uph_ui_menu_item(track_options_context_menu, "Ungroup", leaf_id("uph_song_timeline_options_ungroup")))
	{
		Naui_List(Uph_TrackPtr) children = NULL;
		if (uph_resources_ungroup_track(track, &children))
		{
			_uph_song_timeline_set_selection(children);
			restructured = true;
		}
		naui_list_free(children);
	}

	if (restructured)
	{
		naui_list_free(targets);
		return;
	}

	if (uph_ui_menu_item(track_options_context_menu, "Rename", leaf_id("uph_song_timeline_options_rename")))
		_uph_song_timeline_begin_rename(track);

	if (track->type == UPH_RESOURCE_NONE || track->type == UPH_RESOURCE_PATTERN)
	{
		if (track->instrument.loaded)
		{
			const bool visible = uph_plugin_window_visible(&track->instrument);
			if (uph_ui_menu_item(track_options_context_menu, visible ? "Hide Instrument" : "Show Instrument", leaf_id("uph_song_timeline_options_show_instrument"))) 
			{
				if (visible)
					uph_plugin_hide_window(&track->instrument);
				else uph_plugin_show_window(&track->instrument);
			}

			if (uph_ui_menu_item(track_options_context_menu, "Remove Instrument", leaf_id("uph_song_timeline_options_remove_instrument"))) 
			{
				uph_action_track_unload_plugin(track, &track->instrument);
				if (naui_list_len(track->blocks) == 0)
					track->type = UPH_RESOURCE_NONE;
			}
		}
		else
		{
			if (uph_ui_menu_item(track_options_context_menu, "Load Instrument", leaf_id("uph_song_timeline_options_load_instrument")))
				_uph_song_timeline_open_instrument_list(track);
		}

		if (track->instrument.loaded || naui_list_len(track->effects) != 0)
		{
			if (uph_ui_menu_item(track_options_context_menu, "Automate", leaf_id("uph_song_timeline_options_automate")))
			{
				uph_state.shared.current_automation_list_track = track;
				naui_panel_open(uph_state.panels.automation_list);
			}
		}
	}

	if (uph_ui_menu_item(track_options_context_menu, "Remove", leaf_id("uph_song_timeline_options_remove"))) 
		uph_resources_delete_tracks(targets);

	naui_list_free(targets);
}

void uph_song_timeline_render_track_plus(void)
{
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const Leaf_Color bg_color = naui_theme_color("uph_song_timeline_header_color");
	const Leaf_Color border_color = naui_theme_color("uph_song_timeline_header_border_color");
	const Leaf_Color hovered_bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");

	const Naui_Vec2 padding = naui_theme_vec2("uph_song_timeline_header_padding");
	const float header_width = NAUI_DPI(naui_theme_float("uph_song_timeline_header_width") + uph_song_timeline_header_depth_width());

	Leaf_ID id = leaf_id("uph_song_timeline_plus");
	Leaf_Color bg_color_result = bg_color;
	if (uph_ui_widget_hovered(id))
	{
		bg_color_result = hovered_bg_color;
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			naui_action_execute_stack(UPH_ACTION_TRACK_CREATE, (Uph_ActionTrackCreate){0});

		naui_set_cursor(NAUI_CURSOR_HAND);
	}

	leaf({
		.id = id,
		.size = {LEAF_SIZE_FIXED(header_width), LEAF_SIZE_FIT},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(6), LEAF_CORNER_BL | LEAF_CORNER_BR),
		.border = {
			.width = 1,
			.sides = LEAF_SIDE_ALL,
			.color = border_color
		},
		.color = bg_color_result
	})
	{
		Naui_Image *icon = naui_asset_image("uph_icon_plus");

		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(13)), LEAF_SIZE_DERIVED},
			.color = text_color,
			.image = icon,
			.aspect_ratio = 1.0f
		});
	}
}

#define UPH_SONG_TIMELINE_TRACK_DRAG_THRESHOLD 6.0f

typedef struct
{
	Uph_ActionTrackRef owner;
	uint32_t index;
	Leaf_BoundingBox row;
	bool after;
	bool valid;
}
Uph_TrackDropTarget;

static bool _uph_song_timeline_row_box(const Uph_Track *track, Leaf_BoundingBox *out)
{
	*out = leaf_get_bounding_box(leaf_id_indexed("uph_song_timeline_row", (uint64_t)track));
	return out->height > 0.0f;
}

static Leaf_BoundingBox _uph_song_timeline_subtree_box(const Uph_Track *track, Leaf_BoundingBox box)
{
	const uint32_t count = (uint32_t)naui_list_len(track->subtracks);
	if (count == 0 || (track->type == UPH_RESOURCE_GROUP && (track->state & UPH_TRACK_TIMELINE_COLLAPSED)))
		return box;

	Leaf_BoundingBox last;
	if (!_uph_song_timeline_row_box(&track->subtracks[count - 1], &last))
		return box;

	last = _uph_song_timeline_subtree_box(&track->subtracks[count - 1], last);
	box.height = last.y + last.height - box.y;
	return box;
}

static bool _uph_song_timeline_track_dragged(const Uph_Track *track, Naui_List(Uph_TrackPtr) dragged)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(dragged); i++)
	{
		if (dragged[i] == track)
			return true;
	}
	return false;
}

static bool _uph_song_timeline_drop_on_row(Uph_TrackDropTarget *target, float mouse_y, Leaf_BoundingBox box, Uph_ActionTrackRef owner, uint32_t index)
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

static bool _uph_song_timeline_find_drop_in_list(Uph_TrackDropTarget *target, float mouse_y, Naui_List(Uph_Track) tracks, Uph_ActionTrackRef owner, Naui_List(Uph_TrackPtr) dragged, Leaf_BoundingBox *last_box)
{
	const uint32_t count = (uint32_t)naui_list_len(tracks);
	for (uint32_t i = 0; i < count; i++)
	{
		Uph_Track *track = &tracks[i];

		Leaf_BoundingBox box;
		if (!_uph_song_timeline_row_box(track, &box))
			continue;

		if (track->type != UPH_RESOURCE_GROUP || (track->state & UPH_TRACK_TIMELINE_COLLAPSED) || _uph_song_timeline_track_dragged(track, dragged))
		{
			box = _uph_song_timeline_subtree_box(track, box);
			*last_box = box;
			if (_uph_song_timeline_drop_on_row(target, mouse_y, box, owner, i))
				return true;
			continue;
		}

		*last_box = box;

		const Uph_ActionTrackRef group_ref = uph_action_track_ref_child(owner, i);
		if (_uph_song_timeline_drop_on_row(target, mouse_y, box, owner, i))
		{
			if (target->index > i)
			{
				target->owner = group_ref;
				target->index = 0;
			}
			return true;
		}

		if (_uph_song_timeline_find_drop_in_list(target, mouse_y, track->subtracks, group_ref, dragged, last_box))
			return true;
	}

	return false;
}

static Uph_TrackDropTarget _uph_song_timeline_find_drop_target(float mouse_y, Naui_List(Uph_TrackPtr) dragged)
{
	Uph_TrackDropTarget target = { 0 };
	Leaf_BoundingBox last_box = { 0 };

	if (_uph_song_timeline_find_drop_in_list(&target, mouse_y, uph_state.project.tracks, uph_action_track_ref_root(), dragged, &last_box))
		return target;

	if (last_box.height > 0.0f)
	{
		target = (Uph_TrackDropTarget){
			.owner = uph_action_track_ref_root(),
			.index = (uint32_t)naui_list_len(uph_state.project.tracks),
			.row = last_box,
			.after = true,
			.valid = true
		};
	}
	return target;
}

static Naui_List(Uph_TrackPtr) _uph_song_timeline_drag_tracks(Uph_Track *source)
{
	Naui_List(Uph_TrackPtr) tracks = NULL;

	if (_uph_song_timeline_track_selected(source))
	{
		for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_song_timeline_data.selected_tracks); i++)
			naui_list_push(tracks, uph_song_timeline_data.selected_tracks[i]);
	}
	else
	{
		naui_list_push(tracks, source);
	}

	return tracks;
}

void uph_song_timeline_update_track_drag(void)
{
	Uph_SongTimelineData *timeline = &uph_song_timeline_data;
	Uph_TrackDragState *drag = &timeline->track_drag;

	if (naui_any_mouse_pressed() && !timeline->track_header_hovered && !uph_ui_any_menu_open())
		_uph_song_timeline_select_track(NULL);

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
				_uph_song_timeline_select_track(source);

			*drag = (Uph_TrackDragState){ 0 };
			return;
		}

		const float dx = mouse.x - drag->press_position.x;
		const float dy = mouse.y - drag->press_position.y;
		const float threshold = NAUI_DPI(UPH_SONG_TIMELINE_TRACK_DRAG_THRESHOLD);
		if (dx * dx + dy * dy < threshold * threshold)
			return;

		drag->pending = false;
		drag->active = true;
	}

	Naui_List(Uph_TrackPtr) dragged = _uph_song_timeline_drag_tracks(source);

	const Uph_TrackDropTarget target = timeline->panel_hovered ?
		_uph_song_timeline_find_drop_target(mouse.y, dragged) : (Uph_TrackDropTarget){ 0 };

	if (!mouse_down)
	{
		if (target.valid)
		{
			naui_list_free(timeline->pending_move.tracks);
			timeline->pending_move = (Uph_TrackPendingMove){
				.tracks = dragged,
				.dest_group = target.owner.depth == 0 ? NULL : uph_action_track_resolve(target.owner),
				.dest_index = target.index
			};
		}
		else naui_list_free(dragged);
		*drag = (Uph_TrackDragState){ 0 };
		return;
	}

	const uint32_t carried = uph_resources_count_movable_tracks(dragged);
	naui_list_free(dragged);

	char label[256];
	if (carried > 1)
		snprintf(label, sizeof(label), "%s +%u", source->name.data, carried - 1);
	else
		snprintf(label, sizeof(label), "%s", source->name.data);

	naui_set_cursor(NAUI_CURSOR_HAND);

	if (target.valid)
	{
		const float indent = NAUI_DPI(target.owner.depth * UPH_SONG_TIMELINE_DEPTH_INDENT);
		const float line_height = NAUI_DPI(3.0f);
		const float line_y = target.after ? target.row.y + target.row.height : target.row.y;
		leaf({
			.size = {LEAF_SIZE_FIXED(target.row.width - indent), LEAF_SIZE_FIXED(line_height)},
			.color = {naui_theme_color("uph_ui_slider_fill_color")},
			.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
			.floating = { .offset = { target.row.x + indent, line_y - line_height * 0.5f } }
		});
	}

	leaf({
		.padding = LEAF_PADDING_AXES(NAUI_DPI(8), NAUI_DPI(4)),
		.color = {naui_theme_color("uph_ui_dropdown_bg_color")},
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), LEAF_CORNER_ALL),
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
		.floating = { .offset = { mouse.x + NAUI_DPI(12), mouse.y + NAUI_DPI(12) } }
	})
	{
		leaf_text(label, {
			.font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
			.color = {naui_theme_color("uph_ui_text_color")}
		});
	}
}

void uph_song_timeline_apply_pending_move(void)
{
	Uph_TrackPendingMove *move = &uph_song_timeline_data.pending_move;
	if (!move->tracks)
		return;

	Naui_List(Uph_TrackPtr) moved = NULL;
	if (uph_resources_move_tracks(move->tracks, move->dest_group, move->dest_index, &moved))
		_uph_song_timeline_set_selection(moved);
	naui_list_free(moved);
	naui_list_free(move->tracks);
	*move = (Uph_TrackPendingMove){ 0 };
}
