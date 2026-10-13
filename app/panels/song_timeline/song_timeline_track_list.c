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

static void _uph_song_timeline_render_track_header(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu)
{
	const int32_t depth_offset = depth * 13;
	const Leaf_Color text_color = naui_theme_color("uph_ui_text_color");
	const Leaf_Color bg_color = naui_theme_color("uph_song_timeline_header_color");
	const Leaf_Color border_color = naui_theme_color("uph_song_timeline_header_border_color");

	const Naui_Vec2 padding = naui_theme_vec2("uph_song_timeline_header_padding");
	const float header_width = naui_theme_float("uph_song_timeline_header_width");

	uint64_t track_id = (uint64_t)track;
	Naui_Color color = uph_resources_track_color(track->color_index);

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FIXED(NAUI_DPI(header_width + padding.x * 2.0f)), LEAF_SIZE_FULL}
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
			.child_gap = NAUI_DPI(10),
			.border = {
				.width = 1,
				.color = {border_color},
				.sides = LEAF_SIDE_ALL
			}
		})
		{

			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(5)), LEAF_SIZE_FULL},
				.color = color,
				.rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
			});

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
							if (uph_ui_textfield(&track->name, name_id, UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, NAUI_TR("song_timeline.track.title")))
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

					if (track->type != UPH_RESOURCE_AUTOMATION && uph_ui_image_toggle_button(
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

void uph_song_timeline_render_track(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu)
{
	Uph_SongTimelineData *data = &uph_song_timeline_data;
	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(uph_song_timeline_data.zoom.y))}
	})
	{
		_uph_song_timeline_render_track_header(track, depth, options_menu);
		uph_song_timeline_render_track_timeline(track);
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		data->visual_row_counter++;
		uph_song_timeline_render_track(&track->subtracks[i], depth + 1, options_menu);
	}
}

void uph_song_timeline_render_track_options_menu(Uph_SongTimelineData *data, Uph_UIMenuID track_options_context_menu)
{
	if (naui_list_len(uph_state.project.tracks) == 0 || !data->current_options_track)
		return;

	Uph_Track *track = data->current_options_track;

	{
		Uph_UIMenuID color_menu = uph_ui_submenu(track_options_context_menu, "Color", leaf_id("uph_song_timeline_options_color"));
		const uint32_t color_count = (uint32_t)naui_list_len(naui_theme_color_list("uph_track_palette"));
		for (uint32_t i = 0; i < color_count; i++)
		{
			if (uph_ui_menu_item(color_menu, naui_string_format("Color %i", i).data, leaf_id_indexed("uph_song_timeline_options_color_item", i)) && track->color_index != (int32_t)i)
			{
				Uph_ActionTrackColor color_data = {
					.track = uph_action_track_ref(track),
					.old_color = track->color_index,
					.new_color = (int32_t)i
				};
				naui_action_execute_stack(UPH_ACTION_TRACK_COLOR, color_data);
			}
		}
	   
	}

	if (track->type == UPH_RESOURCE_NONE || track->type == UPH_RESOURCE_PATTERN)
	{
		if (track->instrument.loaded)
		{
			const bool visible = uph_plugin_window_visible(&track->instrument);
			if (uph_ui_menu_item(track_options_context_menu, visible ? "Hide Instrument" : "Show Instrument", leaf_id("uph_song_timeline_options_show_instrument"))) 
			{
				if (visible)
					uph_hide_plugin_window(&track->instrument);
				else uph_show_plugin_window(&track->instrument);
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
				uph_state.shared.plugin_list_for_track_instrument = true;
				uph_state.shared.current_plugin_list_track = track;
				naui_open_panel(uph_state.panels.plugin_list);
			}
		}

		if (track->instrument.loaded || naui_list_len(track->effects) != 0)
		{
			if (uph_ui_menu_item(track_options_context_menu, "Automate", leaf_id("uph_song_timeline_options_automate")))
			{
				uph_state.shared.current_automation_list_track = track;
				naui_open_panel(uph_state.panels.automation_list);
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

	const Naui_Vec2 padding = naui_theme_vec2("uph_song_timeline_header_padding");
	const float header_width = NAUI_DPI(naui_theme_float("uph_song_timeline_header_width"));

	Leaf_ID id = leaf_id("uph_song_timeline_plus");
	if (uph_ui_widget_hovered(id))
	{
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
		.color = bg_color
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
