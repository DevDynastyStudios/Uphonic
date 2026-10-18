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
	const Leaf_Color bg_color = tint_track ?
		uph_color_mix(base_bg_color, uph_resources_track_color(tint_track->color_index), 0.09f) :
		base_bg_color;

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_header_column_width())), LEAF_SIZE_FULL}
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(depth_offset)), LEAF_SIZE_FULL}
		});
		leaf({
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

				leaf({
					.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
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
							if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && uph_ui_widget_hovered(name_id))
							{
								uph_song_timeline_data.disable_space_to_play = true;
								uph_song_timeline_data.rename_track = track;
								uph_song_timeline_data.rename_old_name = track->name;
							}

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

					if (uph_ui_image_button_ex(
						naui_asset_image("uph_icon_gear"),
						leaf_id_indexed("uph_song_timeline_options", track_id),
						(Naui_Vec2){button_size,button_size},
						text_color,
						LEAF_COLOR_TRANSPARENT,
						NAUI_CORNER_ALL
					))
					{
						uph_song_timeline_data.current_options_track = track;
						uph_ui_open_context_menu(options_menu);
					}
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
		}
	}
	
}

void uph_song_timeline_render_track(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu, Uph_UIMenuID color_menu)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(track->type == UPH_RESOURCE_GROUP ? NAUI_DPI(48) : NAUI_DPI(uph_song_timeline_data.zoom.y))}
	})
	{
		_uph_song_timeline_render_track_header(track, depth, options_menu, color_menu);
		if (track->type != UPH_RESOURCE_GROUP)
			uph_song_timeline_render_track_timeline(track);
	}

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
			{
				uph_state.shared.plugin_list_target = UPH_PLUGIN_LIST_TRACK_INSTRUMENT;
				uph_state.shared.current_plugin_list_track = track;
				naui_panel_open(uph_state.panels.plugin_list);
			}
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
	{
		Uph_ActionTrackDelete delete_data = { .track = uph_action_track_ref(track) };
		naui_action_execute_stack(UPH_ACTION_TRACK_DELETE, delete_data);
	}
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
