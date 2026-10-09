#define UPH_MIDI_EDITOR_PREVIEW_CHANNEL 1
#define UPH_MIDI_EDITOR_PREVIEW_VELOCITY 127

static Uph_MidiEditorData uph_midi_editor_data;

void uph_midi_editor_init(Uph_MidiEditorData *editor)
{
	uph_midi_note_input_cancel_drag(editor);
	memset(editor, 0, sizeof(*editor));
	uph_note_map_init_default(&editor->note_map);
	uph_piano_init(&editor->piano);

	editor->view.zoom = (Naui_Vec2) { NAUI_DPI(100.0f), NAUI_DPI(30.0f) };
	editor->current_action_mode = UPH_ACTION_DRAW;
	editor->snap_resolution = UPH_SNAP_QUARTER;
	editor->note_input.last_note_length = UPH_MIDI_EDITOR_DEFAULT_NOTE_LENGTH;
	editor->note_input.last_note_velocity = UPH_MIDI_EDITOR_DEFAULT_NOTE_VELOCITY;
	const int32_t top_row = uph_note_map_row(&editor->note_map, UPH_MIDI_EDITOR_INITIAL_TOP_KEY);
	editor->view.scroll = (Naui_Vec2) { 0.0f, fmaxf(0.0f, (float)top_row * uph_midi_view_lane_height(&editor->view)) };
}

#pragma region Piano preview

static void _uph_midi_editor_send_preview(Uph_PianoEvents events)
{
	if (events.released != -1)
		cmidi_stop_note(uph_state.settings.midi.output, UPH_MIDI_EDITOR_PREVIEW_CHANNEL, (uint8_t)events.released);

	if (events.pressed != -1)
		cmidi_play_note(uph_state.settings.midi.output, UPH_MIDI_EDITOR_PREVIEW_CHANNEL, (uint8_t)events.pressed, UPH_MIDI_EDITOR_PREVIEW_VELOCITY);
}

void uph_midi_editor_reset_interaction(Uph_MidiEditorData *editor)
{
	uph_midi_note_input_cancel_drag(editor);
	editor->note_input.marquee.active = false;
	_uph_midi_editor_send_preview(uph_piano_release_mouse(&editor->piano));
}

#pragma endregion

#pragma region View input

static void _uph_midi_editor_update_view_input(Uph_MidiEditorData *editor, Leaf_BoundingBox box)
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
	Uph_MidiEditorData *editor = &uph_midi_editor_data;
	const Uph_PianoCanvas canvas = uph_midi_view_piano_canvas(&editor->view, box);

	_uph_midi_editor_send_preview(uph_piano_update(&editor->piano, &editor->note_map, &canvas, editor->piano_hovered));
	uph_piano_render(&editor->piano, &editor->note_map, &canvas);
}

static void _uph_midi_editor_lanes_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorData *editor = &uph_midi_editor_data;
	uph_midi_grid_render_lanes(box, &editor->view, &editor->note_map, &editor->piano);
	editor->note_input.hovered_note.active = false;

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	if (editor->current_action_mode == UPH_ACTION_CUT && editor->lanes_hovered)
		uph_midi_notes_render_cut_line(box, &editor->view, editor->snap_resolution);

	uph_midi_grid_render_beats(box, &editor->view, editor->snap_resolution);
	naui_pop_clip_rect();

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
		return;

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	Uph_MidiPattern *pattern = &uph_state.project.midi_patterns[uph_state.shared.selected_resource.index];
	uph_midi_note_input_update(editor, box, pattern);
	uph_midi_notes_render(box, &editor->view, &editor->note_map, pattern);
	uph_midi_notes_render_marquee(&editor->view, box, &editor->note_input.marquee);
	naui_pop_clip_rect();
}

static void _uph_midi_editor_ruler_draw(Leaf_BoundingBox box, void *user_data)
{
	uph_midi_grid_render_ruler(box, &uph_midi_editor_data.view, uph_midi_editor_data.snap_resolution);
}

#pragma endregion

static void _uph_midi_editor_render_top_ruler(const Uph_MidiEditorData *editor)
{
	const int32_t height = NAUI_DPI(UPH_MIDI_EDITOR_TOP_RULER_HEIGHT);
	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(height)},
		.child_gap = 1
	})
	{
		leaf({
			.size = {LEAF_SIZE_FIXED(uph_piano_width(&editor->piano)), LEAF_SIZE_FULL}
		});
		leaf({
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FIXED(height - 2)},
			.custom_draw = (Leaf_CustomDrawFn)_uph_midi_editor_ruler_draw,
			.border = {
				.width = 1,
				.sides = LEAF_SIDE_ALL,
				.color = naui_theme_color("uph_timeline_top_ruler_border_color")
			},
			.color = naui_theme_color("uph_timeline_top_ruler_bg_color")
		});
	}
}

void uph_midi_editor_update(Uph_MidiEditorData *editor)
{
	const Leaf_ID lanes_id = leaf_id("uph_midi_editor_lanes");
	const Leaf_ID piano_id = leaf_id("uph_midi_editor_piano");

	editor->panel_hovered = naui_panel_hovered(naui_current_panel());
	editor->lanes_hovered = leaf_hovered(lanes_id) && editor->panel_hovered;
	editor->piano_hovered = leaf_hovered(piano_id) && editor->panel_hovered;
	editor->lanes_bounding_box = leaf_get_bounding_box(lanes_id);

	if (editor->note_input.marquee.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
		editor->note_input.marquee.active = false;

	_uph_midi_editor_update_view_input(editor, editor->lanes_bounding_box);
	uph_midi_note_input_update_shortcuts(editor);
	uph_midi_toolbar_render(&editor->current_action_mode, &editor->snap_resolution);
	_uph_midi_editor_render_top_ruler(editor);

	leaf({
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
		.child_gap = 1.0f,
		.clip_children = true
	})
	{
		leaf({
			.id = piano_id,
			.size = {LEAF_SIZE_FIXED(uph_piano_width(&editor->piano)), LEAF_SIZE_FULL},
			.custom_draw = _uph_midi_editor_piano_draw
		});
		leaf({
			.id = lanes_id,
			.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
			.custom_draw = _uph_midi_editor_lanes_draw
		});
	}
}
