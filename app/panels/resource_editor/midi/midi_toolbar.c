#define UPH_MIDI_TOOLBAR_MAX_VOICES 64

typedef struct
{
	Uph_Track *tracks[UPH_MIDI_TOOLBAR_MAX_VOICES];
	const char *names[UPH_MIDI_TOOLBAR_MAX_VOICES + 1];
	uint32_t count;
}
Uph_MidiToolbarVoices;

static void _uph_midi_toolbar_collect_voices(Naui_List(Uph_Track) tracks, Uph_MidiToolbarVoices *voices)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(tracks) && voices->count < UPH_MIDI_TOOLBAR_MAX_VOICES; i++)
	{
		Uph_Track *track = &tracks[i];
		if (uph_midi_preview_is_voice_candidate(track))
		{
			voices->tracks[voices->count] = track;
			voices->names[voices->count + 1] = track->name.length ? track->name.data : track->instrument.name.data;
			voices->count++;
		}

		_uph_midi_toolbar_collect_voices(track->subtracks, voices);
	}
}

static void _uph_midi_toolbar_render_voice(void)
{
	Uph_MidiToolbarVoices voices = { .names = { "None" } };
	_uph_midi_toolbar_collect_voices(uph_state.project.tracks, &voices);

	const Uph_Track *current = uph_midi_preview_voice();
	uint32_t selected = 0;
	for (uint32_t i = 0; i < voices.count; i++)
	{
		if (voices.tracks[i] == current)
			selected = i + 1;
	}

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
		.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
	})
	{
		leaf_text("Voice: ", {
			.font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
			.color = naui_theme_color("uph_ui_text_color")
		});

		leaf({
			.size = {LEAF_SIZE_FIXED(NAUI_DPI(150)), LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		{
			if (uph_ui_dropdown(voices.names, voices.count + 1, &selected, leaf_id("uph_midi_editor_voice")))
				uph_midi_preview_set_voice(selected == 0 ? NULL : voices.tracks[selected - 1]);
		}
	}
}

void uph_midi_toolbar_render(Uph_MidiEditorData *editor)
{
	Uph_ActionMode *action_mode = &editor->current_action_mode;
	Uph_SnapResolution *snap_resolution = &editor->snap_resolution;

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(38))},
		.padding = LEAF_PADDING_ALL(1),
		.color = {naui_theme_color("uph_toolbox_border_color")}
	})
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(naui_theme_vec2("uph_ui_frame_padding").x * 2.0f), 0.0f),
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.child_gap = NAUI_DPI(12),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		const int32_t button_size = NAUI_DPI(naui_theme_float("uph_song_timeline_icon_size"));
		const Naui_Color bg_color = naui_theme_color("uph_ui_frame_bg_color");
		const Naui_Color line_color = naui_theme_color("uph_toolbox_separator_color");
		const Naui_Color icon_color = naui_theme_color("uph_tool_icon_color");

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		{
			const bool playing = uph_midi_preview_playing();

			if (uph_ui_image_button_ex(
				naui_asset_image(playing ? "uph_icon_pause" : "uph_icon_play"),
				leaf_id("uph_midi_editor_play"),
				(Naui_Vec2){button_size, button_size},
				naui_theme_color(playing ? "uph_pause_icon_color" : "uph_play_icon_color"),
				bg_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL
			)) uph_midi_editor_toggle_playback();

			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_stop"),
				leaf_id("uph_midi_editor_stop"),
				(Naui_Vec2){button_size, button_size},
				naui_theme_color("uph_stop_icon_color"),
				bg_color,
				NAUI_CORNER_NONE
			)) uph_midi_editor_stop_playback();

			if (uph_ui_image_button_ex(
				naui_asset_image("uph_icon_reset"),
				leaf_id("uph_midi_editor_reset"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_NONE
			)) uph_midi_preview_seek(0.0);

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_loop"),
				leaf_id("uph_midi_editor_loop"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				uph_midi_preview_looping()
			)) uph_midi_preview_set_loop(!uph_midi_preview_looping());
		}
		leaf({.size = {LEAF_SIZE_FIXED(NAUI_DPI(1)), LEAF_SIZE_PERCENT(0.5f)}, .color = {line_color}});
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER}
		})
		{
			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_select"),
				leaf_id("uph_midi_editor_select"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TL | NAUI_CORNER_BL,
				*action_mode == UPH_ACTION_SELECT
			)) *action_mode = UPH_ACTION_SELECT;

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_draw"),
				leaf_id("uph_midi_editor_draw"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_NONE,
				*action_mode == UPH_ACTION_DRAW
			)) *action_mode = UPH_ACTION_DRAW;

			if (uph_ui_image_toggle_button_ex(
				naui_asset_image("uph_icon_cut"),
				leaf_id("uph_midi_editor_cut"),
				(Naui_Vec2){button_size, button_size},
				icon_color,
				bg_color,
				NAUI_CORNER_TR | NAUI_CORNER_BR,
				*action_mode == UPH_ACTION_CUT
			)) *action_mode = UPH_ACTION_CUT;
		}
		leaf({.size = {LEAF_SIZE_FIXED(NAUI_DPI(1)), LEAF_SIZE_PERCENT(0.5f)}, .color = {line_color}});
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
			.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
		})
		{
			leaf_text("Snap: ", {
				.font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
				.color = naui_theme_color("uph_ui_text_color")
			});
			static const char *snap_options[] = {
				"Beat",
				"1/2",
				"1/4",
				"1/8",
				"1/16"
			};

			leaf({
				.size = {LEAF_SIZE_FIXED(NAUI_DPI(60)), LEAF_SIZE_FULL},
				.child_alignment = {LEAF_ALIGN_Y_CENTER, LEAF_ALIGN_Y_CENTER}
			}) uph_ui_dropdown(snap_options, 5, (uint32_t*)snap_resolution, leaf_id("uph_midi_editor_snap"));
		}
		leaf({.size = {LEAF_SIZE_FIXED(NAUI_DPI(1)), LEAF_SIZE_PERCENT(0.5f)}, .color = {line_color}});
		_uph_midi_toolbar_render_voice();
		leaf({.size = {LEAF_SIZE_FIXED(NAUI_DPI(1)), LEAF_SIZE_PERCENT(0.5f)}, .color = {line_color}});

		if (uph_ui_text_toggle_button("Velocity", leaf_id("uph_midi_editor_velocity_toggle"), editor->velocity_lane.visible))
			editor->velocity_lane.visible = !editor->velocity_lane.visible;
	}
}
