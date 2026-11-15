#define UPH_MIDI_EDITOR_PREVIEW_CHANNEL 1
#define UPH_MIDI_EDITOR_PREVIEW_VELOCITY 127
#define UPH_MIDI_EDITOR_DEFAULT_NOTE_LENGTH 1.0
#define UPH_MIDI_EDITOR_ZOOM_X_MIN 8.0f
#define UPH_MIDI_EDITOR_ZOOM_X_MAX 256.0f
#define UPH_MIDI_EDITOR_ZOOM_Y_MIN 8.0f
#define UPH_MIDI_EDITOR_ZOOM_Y_MAX 60.0f
#define UPH_MIDI_EDITOR_PAN_SPEED 1.0f
#define UPH_MIDI_EDITOR_SCROLL_Y_SPEED 40.0f
#define UPH_MIDI_EDITOR_ZOOM_SPEED 0.1f
#define UPH_MIDI_EDITOR_TOP_RULER_HEIGHT 28

#define UPH_MIDI_EDITOR_INITIAL_TOP_KEY 70
#define UPH_MIDI_EDITOR_PEDAL_DEFAULT_VALUE 127

#define UPH_MIDI_EDITOR_TAB_BAR_HEIGHT 26
#define UPH_MIDI_EDITOR_TAB_PADDING_X 10
#define UPH_MIDI_EDITOR_TAB_ICON_SIZE 9
#define UPH_MIDI_EDITOR_RESIZE_GRAB_HEIGHT 4

static Uph_MidiEditorState _uph_midi_editor_state;

void uph_midi_editor_init(void)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;

	uph_midi_note_input_cancel_drag(state);
	uph_midi_velocity_cancel_drag(&state->velocity_lane);
	uph_midi_pedal_cancel_drag(&state->pedal_lane);
	memset(state, 0, sizeof(*state));
	uph_note_map_init_default(&state->note_map);
	uph_midi_editor_set_scale(state, uph_midi_editor_find_scale("Major"), 0);
	uph_piano_init(&state->piano);
	uph_midi_velocity_init(&state->velocity_lane);
	uph_midi_pedal_init(&state->pedal_lane);
	state->audition_key = -1;

	state->view.zoom = (Naui_Vec2) { NAUI_DPI(100), NAUI_DPI(30) };
	state->current_action_mode = UPH_ACTION_DRAW;
	state->snap_resolution = UPH_SNAP_LINE;
	state->note_input.last_note_length = UPH_MIDI_EDITOR_DEFAULT_NOTE_LENGTH;
	state->note_input.last_note_velocity = UPH_MIDI_EDITOR_DEFAULT_NOTE_VELOCITY;
	uph_midi_editor_reset_scroll();
}

void uph_midi_editor_reset_scroll(void)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	const int32_t top_row = uph_note_map_row(&state->note_map, UPH_MIDI_EDITOR_INITIAL_TOP_KEY);
	state->view.scroll = (Naui_Vec2) { 0.0f, fmaxf(0.0f, (float)top_row * uph_midi_view_lane_height(&state->view)) };
}

void uph_midi_editor_emit_note_shockwave(Uph_ResourceIndex pattern_index, const Uph_MidiNote *note)
{
	if (naui_panel_closed(uph_state.panels.resource_editor))
		return;

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN || uph_state.shared.selected_resource.index != pattern_index)
		return;

	const Uph_MidiEditorState *state = &_uph_midi_editor_state;
	const Leaf_BoundingBox box = state->lanes_bounding_box;
	const Naui_Vec4 note_box = uph_midi_view_note_box(&state->view, &state->note_map, box, note);
	const Leaf_Color color = uph_midi_view_velocity_color(uph_midi_view_note_color(), note->velocity);

	if (naui_app_animations_enabled())
		uph_particle_emit_shockwave(
			(Naui_Vec2){ note_box.x, note_box.y },
			(Naui_Vec2){ note_box.z, note_box.w },
			uph_color_mix(color, leaf_rgba(255, 255, 255, 255), 0.35f),
			NAUI_DPI(3),
			naui_panel_bounding_box(uph_state.panels.resource_editor)
		);
}

#pragma region Piano preview

static void _uph_midi_editor_send_preview(Uph_PianoEvents events)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	if (events.released != -1)
		uph_midi_preview_note_off((uint8_t)events.released);

	if (events.pressed != -1)
		uph_midi_preview_note_on((uint8_t)events.pressed, (uint8_t)state->note_input.last_note_velocity);
}

static void _uph_midi_editor_set_audition(Uph_MidiEditorState *editor, int16_t key, uint8_t velocity)
{
	if (key == editor->audition_key)
		return;

	if (editor->audition_key != -1)
		uph_midi_preview_note_off((uint8_t)editor->audition_key);

	if (key != -1)
		uph_midi_preview_note_on((uint8_t)key, velocity);

	editor->audition_key = key;
}

static void _uph_midi_editor_update_audition(Uph_MidiEditorState *editor, const Uph_MidiPattern *pattern)
{
	const Uph_DraggingNoteState *drag = &editor->note_input.drag;
	const bool playing = uph_midi_preview_playing() || uph_state.shared.song_timeline_playing;
	if (playing || !drag->active || drag->note_index >= (uint32_t)naui_list_len(pattern->notes))
	{
		_uph_midi_editor_set_audition(editor, -1, 0);
		return;
	}

	const Uph_MidiNote *note = &pattern->notes[drag->note_index];
	_uph_midi_editor_set_audition(editor, (int16_t)note->key_number, note->velocity);
}

void uph_midi_editor_reset_interaction(void)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	uph_midi_note_input_cancel_drag(state);
	uph_midi_note_input_cancel_erase(state);
	uph_midi_velocity_cancel_drag(&state->velocity_lane);
	uph_midi_pedal_cancel_drag(&state->pedal_lane);
	state->velocity_lane.resizing = false;
	state->note_input.marquee.active = false;
	_uph_midi_editor_send_preview(uph_piano_release_mouse(&state->piano));
	_uph_midi_editor_set_audition(state, -1, 0);
	state->playhead_dragging = false;
	state->loop_drag.mode = UPH_LOOP_DRAG_NONE;
	uph_midi_preview_stop();
}

#pragma endregion

#pragma region Pattern playback

void uph_midi_editor_toggle_playback(void)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	if (uph_midi_preview_playing())
	{
		uph_midi_preview_pause();
		return;
	}

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN || !uph_note_pattern(uph_state.shared.selected_resource.index))
		return;

	uph_state.shared.song_timeline_playing = false;
	uph_sample_preview_pause();
	state->playing_pattern = uph_state.shared.selected_resource.index;
	uph_midi_preview_play(state->playing_pattern);
}

void uph_midi_editor_stop_playback(void)
{
	uph_midi_preview_stop();
}

static void _uph_midi_editor_update_playback(Uph_MidiEditorState *editor)
{
	uph_midi_preview_refresh_voice();

	if (naui_panel_focused() == uph_state.panels.resource_editor && naui_keybind_pressed(uph_state.settings.keybinds.play_pause) && !uph_ui_any_menu_open())
		uph_midi_editor_toggle_playback();

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN || editor->playing_pattern == uph_state.shared.selected_resource.index)
		return;

	editor->playing_pattern = uph_state.shared.selected_resource.index;
	uph_midi_preview_stop();
}

static void _uph_midi_editor_update_playhead_drag(Uph_MidiEditorState *editor, Leaf_BoundingBox box)
{
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
	const bool over_ruler = uph_song_timeline_vec4_contains_vec2((Naui_Vec4) { box.x, box.y, box.width, box.height }, mouse);
	Uph_MidiPattern *pattern = uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN ? uph_note_pattern(uph_state.shared.selected_resource.index) : NULL;

	bool loop_busy = false;
	if (pattern)
	{
		loop_busy = uph_loop_region_update(
			&pattern->loop_region, &editor->loop_drag, box,
			editor->view.zoom.x, editor->view.scroll.x, uph_snap_division(editor->snap_resolution, editor->view.zoom.x),
			!editor->playhead_dragging && editor->panel_hovered
		);
		if (editor->loop_drag.created)
		{
			editor->loop_drag.created = false;
			uph_midi_preview_set_loop(true);
		}
	}
	else
		editor->loop_drag.mode = UPH_LOOP_DRAG_NONE;

	if (!editor->playhead_dragging && !loop_busy && over_ruler && editor->panel_hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
	{
		editor->playhead_dragging = true;
		editor->playhead_drag_beat = -1.0;
	}

	if (!editor->playhead_dragging)
		return;

	double beat = fmax(0.0, uph_snap_beat_floor(uph_midi_view_x_to_beat(&editor->view, box, mouse.x), editor->snap_resolution, editor->view.zoom.x));
	if (pattern)
	{
		double length = uph_midi_preview_loop_length(pattern);
		if (uph_midi_preview_looping() && uph_loop_region_valid(&pattern->loop_region))
			length = fmax(length, pattern->loop_region.end);
		if (beat >= length)
			beat = fmax(0.0, length - uph_snap_division(editor->snap_resolution, editor->view.zoom.x));
	}

	if (beat != editor->playhead_drag_beat)
	{
		editor->playhead_drag_beat = beat;
		uph_midi_preview_seek(beat);
	}

	naui_set_cursor(NAUI_CURSOR_HAND);
	if (!naui_mouse_down(NAUI_MOUSE_LEFT))
		editor->playhead_dragging = false;
}

static void _uph_midi_editor_playhead_overlay_draw(Leaf_BoundingBox box, void *user_data)
{
	const Uph_MidiEditorState *state = &_uph_midi_editor_state;
	const Leaf_BoundingBox lanes = state->lanes_bounding_box;
	const float x = uph_midi_view_beat_to_x(&state->view, lanes, uph_midi_preview_position());
	const int32_t size = NAUI_DPI(16);
	const float half_size = size * 0.5f;
	const float ruler_bottom = box.y + (float)(NAUI_DPI(UPH_MIDI_EDITOR_TOP_RULER_HEIGHT) - 2);
	const Naui_Color color = naui_theme_color("uph_midi_editor_playhead_color");

	naui_push_clip_rect(lanes.x - NAUI_DPI(1), box.y, lanes.width + NAUI_DPI(1), lanes.y + lanes.height - box.y);
	naui_draw_line((Naui_Vec2) { x, ruler_bottom - half_size }, (Naui_Vec2) { x, lanes.y + lanes.height }, color, NAUI_DPI(1));
	naui_draw_image(
		naui_asset_image("uph_icon_playhead"),
		(Naui_Vec2) { x - half_size, ruler_bottom - size },
		(Naui_Vec2) { size, size },
		color,
		0.0f,
		NAUI_CORNER_NONE
	);
	naui_pop_clip_rect();
}

static void _uph_midi_editor_render_loop_end(const Uph_MidiEditorState *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const float x = uph_midi_view_beat_to_x(&editor->view, box, uph_midi_preview_loop_length(pattern));
	if (x > box.x + box.width)
		return;

	const float start = fmaxf(x, box.x);
	naui_fill_rect((Naui_Vec2) { start, box.y }, (Naui_Vec2) { box.x + box.width - start, box.height }, naui_theme_color("uph_midi_editor_loop_end_overlay_color"), 0.0f, NAUI_CORNER_NONE);
}

#pragma endregion

#pragma region View input

static void _uph_midi_editor_update_view_input(Uph_MidiEditorState *editor, Leaf_BoundingBox box)
{
	Uph_MidiView *view = &editor->view;
	const float wheel_y = (float)naui_mouse_scroll_delta();
	const bool ctrl_held = naui_key_down(NAUI_KEY_LCONTROL);
	const float max_scroll_y = uph_midi_view_max_scroll_y(view, &editor->note_map, box.height);

	if (editor->panel_hovered)
	{
		if (ctrl_held && wheel_y != 0.0f)
		{
			const float old_zoom_x = view->zoom.x;
			const double mouse_beat_before = uph_midi_view_x_to_beat(view, box, (float)naui_mouse_x());

			float new_zoom_x = old_zoom_x * (1.0f + wheel_y * UPH_MIDI_EDITOR_ZOOM_SPEED);
			new_zoom_x = NAUI_CLAMP(new_zoom_x, UPH_MIDI_EDITOR_ZOOM_X_MIN, UPH_MIDI_EDITOR_ZOOM_X_MAX);
			view->zoom.x = new_zoom_x;
			view->scroll.x = (float)(mouse_beat_before * new_zoom_x) - ((float)naui_mouse_x() - box.x);
			view->scroll.x = fmaxf(0.0f, view->scroll.x);
		}
		else if (wheel_y != 0.0f)
		{
			view->scroll.y -= wheel_y * UPH_MIDI_EDITOR_SCROLL_Y_SPEED;
			view->scroll.y = NAUI_CLAMP(view->scroll.y, 0.0f, max_scroll_y);
		}
	}

	if (editor->panel_hovered && naui_mouse_pressed(NAUI_MOUSE_MIDDLE))
	{
		editor->panning = true;
		editor->pan_last_mouse = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
	}

	if (editor->panning)
	{
		const Naui_Vec2 current = (Naui_Vec2) { (float)naui_mouse_x(), (float)naui_mouse_y() };
		const Naui_Vec2 delta = (Naui_Vec2) { current.x - editor->pan_last_mouse.x, current.y - editor->pan_last_mouse.y };

		view->scroll.x -= delta.x * UPH_MIDI_EDITOR_PAN_SPEED;
		view->scroll.x = fmaxf(0.0f, view->scroll.x);

		const float current_max_scroll_y = uph_midi_view_max_scroll_y(view, &editor->note_map, box.height);
		view->scroll.y -= delta.y * UPH_MIDI_EDITOR_PAN_SPEED;
		view->scroll.y = NAUI_CLAMP(view->scroll.y, 0.0f, current_max_scroll_y);

		editor->pan_last_mouse = current;

		if (naui_mouse_released(NAUI_MOUSE_MIDDLE))
			editor->panning = false;
	}
}

#pragma endregion

#pragma region Draw callbacks

static void _uph_midi_editor_piano_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	const Uph_PianoCanvas canvas = uph_midi_view_piano_canvas(&state->view, box);

	_uph_midi_editor_send_preview(uph_piano_update(&state->piano, &state->note_map, &canvas, state->piano_hovered));
	uph_piano_render(&state->piano, &state->note_map, &canvas);
}

static void _uph_midi_editor_lanes_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	uph_midi_grid_render_lanes(box, &state->view, &state->note_map, &state->piano);
	state->note_input.hovered_note.active = false;

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	if (state->current_action_mode == UPH_ACTION_CUT && state->lanes_hovered)
		uph_midi_notes_render_cut_line(box, &state->view, state->snap_resolution);

	uph_midi_grid_render_beats(box, &state->view, state->snap_resolution);
	naui_pop_clip_rect();

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
		return;

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[uph_state.shared.selected_resource.index];
	uph_midi_note_input_update(state, box, pattern);
	_uph_midi_editor_update_audition(state, pattern);
	_uph_midi_editor_render_loop_end(state, box, pattern);
	uph_loop_region_render_bounds(&pattern->loop_region, (float)box.x, (float)box.y, (float)(box.y + box.height), state->view.zoom.x, state->view.scroll.x, uph_midi_preview_looping(), naui_theme_color("uph_midi_editor_selection_outline_color"));
	uph_midi_notes_render(box, &state->view, &state->note_map, pattern);
	uph_midi_notes_render_marquee(&state->view, box, &state->note_input.marquee);
	naui_pop_clip_rect();
}

static void _uph_midi_editor_ruler_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	_uph_midi_editor_update_playhead_drag(state, box);
	const Uph_MidiPattern *pattern = uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN ? uph_note_pattern(uph_state.shared.selected_resource.index) : NULL;
	if (pattern)
		uph_loop_region_render_ruler(&pattern->loop_region, box, state->view.zoom.x, state->view.scroll.x, uph_midi_preview_looping(), naui_theme_color("uph_midi_editor_selection_outline_color"));
	uph_midi_grid_render_ruler(box, &state->view, state->snap_resolution);
}

static void _uph_midi_editor_velocity_header_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	if (state->lane_tab == UPH_MIDI_LANE_TAB_PEDAL)
		uph_midi_pedal_render_header(state, box);
	else
		uph_midi_velocity_render_header(box);
}

static void _uph_midi_editor_velocity_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;
	Uph_MidiPattern *pattern = uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN ? uph_note_pattern(uph_state.shared.selected_resource.index) : NULL;

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	if (state->lane_tab == UPH_MIDI_LANE_TAB_PEDAL)
	{
		if (pattern)
			uph_midi_pedal_update(state, box, pattern);

		uph_midi_pedal_render(state, box, pattern);
	}
	else
	{
		if (pattern)
			uph_midi_velocity_update(state, box, pattern);

		uph_midi_velocity_render(state, box, pattern);
	}

	if (pattern)
		_uph_midi_editor_render_loop_end(state, box, pattern);

	naui_pop_clip_rect();
}

#pragma endregion

static bool _uph_midi_editor_tab(const char *label, const Leaf_ID id, bool active)
{
	const bool hovered = uph_ui_widget_hovered(id);
	if (hovered)
		naui_set_cursor(NAUI_CURSOR_HAND);

	Leaf_Color bg_color = { 0 };
	if (active)
		bg_color = naui_theme_color("uph_ui_frame_pressed_bg_color");
	else if (hovered)
		bg_color = naui_theme_color("uph_ui_frame_hovered_bg_color");

	const Leaf_Color text_color = naui_theme_color(active || hovered ? "uph_ui_text_color" : "uph_ui_text_disabled_color");

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_MIDI_EDITOR_TAB_PADDING_X), 0),
		.color = bg_color,
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_TL | NAUI_CORNER_TR),
		.child_gap = NAUI_DPI(6),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		const int32_t icon_size = NAUI_DPI(UPH_MIDI_EDITOR_TAB_ICON_SIZE);
		leaf({
			.image = (void*)naui_asset_image(active ? "uph_icon_arrow_down" : "uph_icon_arrow_right"),
			.size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
			.color = text_color
		});
		leaf_text(label, {
			.color = text_color,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
		});
	}

	return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void _uph_midi_editor_render_tab_bar(Uph_MidiEditorState *editor, Leaf_ID bar_id)
{
	Uph_VelocityLane *lane = &editor->velocity_lane;
	const Leaf_ID tab_id = leaf_id("uph_midi_editor_velocity_tab");
	const Leaf_ID pedal_tab_id = leaf_id("uph_midi_editor_pedal_tab");

	if (lane->visible)
	{
		const Leaf_BoundingBox bar = leaf_get_bounding_box(bar_id);
		const int32_t grab = NAUI_DPI(UPH_MIDI_EDITOR_RESIZE_GRAB_HEIGHT);
		const float mouse_x = (float)naui_mouse_x();
		const float mouse_y = (float)naui_mouse_y();
		const bool over_border = mouse_x >= bar.x && mouse_x < bar.x + bar.width && mouse_y >= bar.y && mouse_y <= bar.y + grab;
		uph_midi_velocity_update_splitter(editor, over_border);
	}
	else
	{
		lane->splitter_hovered = false;
		lane->resizing = false;
	}

	const bool handle_active = lane->splitter_hovered || lane->resizing;
	const Leaf_Color border_color = naui_theme_color(handle_active ? "uph_ui_frame_hovered_bg_color" : "uph_midi_editor_toolbox_border_color");

	leaf({
		.id = bar_id,
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIDI_EDITOR_TAB_BAR_HEIGHT)) },
		.padding = { .left = NAUI_DPI(6), .top = NAUI_DPI(4) },
		.border = {
			.width = handle_active ? 2 : 1,
			.sides = lane->visible ? (LEAF_SIDE_TOP | LEAF_SIDE_BOTTOM) : LEAF_SIDE_TOP,
			.color = border_color
		},
		.color = naui_theme_color("uph_midi_editor_toolbox_bg_color"),
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_BOTTOM },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		static const char *const labels[] = { "Velocity", "Pedal" };
		const Leaf_ID ids[] = { tab_id, pedal_tab_id };
		for (Uph_MidiLaneTab tab = UPH_MIDI_LANE_TAB_VELOCITY; tab <= UPH_MIDI_LANE_TAB_PEDAL; tab++)
		{
			const bool active = lane->visible && editor->lane_tab == tab;
			if (!_uph_midi_editor_tab(labels[tab], ids[tab], active))
				continue;

			uph_midi_velocity_cancel_drag(lane);
			uph_midi_pedal_cancel_drag(&editor->pedal_lane);
			lane->visible = !active;
			editor->lane_tab = tab;
		}
	}
}

static void _uph_midi_editor_render_top_ruler(const Uph_MidiEditorState *editor)
{
	const int32_t height = NAUI_DPI(UPH_MIDI_EDITOR_TOP_RULER_HEIGHT);
	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(height) },
		.child_gap = 1,
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FIXED(uph_piano_width(&editor->piano)), .height = LEAF_SIZE_FULL }
		});
		leaf({
			.custom_draw = (Leaf_CustomDrawFn)_uph_midi_editor_ruler_draw,
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIXED(height - 2) },
			.border = {
				.width = 1,
				.sides = LEAF_SIDE_ALL,
				.color = naui_theme_color("uph_midi_editor_ruler_border_color")
			},
			.color = naui_theme_color("uph_midi_editor_ruler_bg_color")
		});
	}
}

void uph_midi_editor_update(void)
{
	Uph_MidiEditorState *state = &_uph_midi_editor_state;

	const Leaf_ID lanes_id = leaf_id("uph_midi_editor_lanes");
	const Leaf_ID piano_id = leaf_id("uph_midi_editor_piano");
	const Leaf_ID velocity_id = leaf_id("uph_midi_editor_velocity");
	const Leaf_ID splitter_id = leaf_id("uph_midi_editor_velocity_tab_bar");

	state->panel_hovered = naui_panel_is_hovered(naui_panel_current());
	state->panel_focused = naui_panel_focused() == naui_panel_current();
	state->lanes_hovered = leaf_hovered(lanes_id) && state->panel_hovered;
	if (state->lanes_hovered)
		naui_set_cursor(uph_action_mode_cursor(_uph_note_input_tool(state)));
	state->piano_hovered = leaf_hovered(piano_id) && state->panel_hovered;
	const bool bottom_lane_hovered = state->velocity_lane.visible && leaf_hovered(velocity_id) && state->panel_hovered;
	state->velocity_lane.hovered = bottom_lane_hovered && state->lane_tab == UPH_MIDI_LANE_TAB_VELOCITY;
	state->pedal_lane.hovered = bottom_lane_hovered && state->lane_tab == UPH_MIDI_LANE_TAB_PEDAL;
	state->lanes_bounding_box = leaf_get_bounding_box(lanes_id);

	if (state->note_input.marquee.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
		state->note_input.marquee.active = false;

	if (!state->velocity_lane.visible)
	{
		uph_midi_velocity_cancel_drag(&state->velocity_lane);
		uph_midi_pedal_cancel_drag(&state->pedal_lane);
		state->velocity_lane.resizing = false;
	}

	_uph_midi_editor_update_playback(state);
	_uph_midi_editor_update_view_input(state, state->lanes_bounding_box);
	uph_midi_note_input_update_shortcuts(state);
	uph_midi_menu_bar_render(state);
	uph_midi_toolbar_render(state);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW }
	})
	{
		_uph_midi_editor_render_top_ruler(state);

		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
			.child_gap = 1.0f,
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.clip_children = true
		})
		{
			leaf({
				.id = piano_id,
				.custom_draw = _uph_midi_editor_piano_draw,
				.size = { .width = LEAF_SIZE_FIXED(uph_piano_width(&state->piano)), .height = LEAF_SIZE_FULL }
			});
			leaf({
				.id = lanes_id,
				.custom_draw = _uph_midi_editor_lanes_draw,
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL }
			});
		}

		_uph_midi_editor_render_tab_bar(state, splitter_id);

		if (state->velocity_lane.splitter_hovered || state->velocity_lane.resizing)
			state->lanes_hovered = false;

		if (state->velocity_lane.visible)
		{
			leaf({
				.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(state->velocity_lane.height)) },
				.child_gap = 1.0f,
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.clip_children = true
			})
			{
				leaf({
					.custom_draw = _uph_midi_editor_velocity_header_draw,
					.size = { .width = LEAF_SIZE_FIXED(uph_piano_width(&state->piano)), .height = LEAF_SIZE_FULL }
				});
				leaf({
					.id = velocity_id,
					.custom_draw = _uph_midi_editor_velocity_draw,
					.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL }
				});
			}
		}

		leaf({
			.custom_draw = _uph_midi_editor_playhead_overlay_draw,
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
		});
	}
}
