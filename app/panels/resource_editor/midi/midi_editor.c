#define UPH_MIDI_EDITOR_PREVIEW_CHANNEL 1
#define UPH_MIDI_EDITOR_PREVIEW_VELOCITY 127

static Uph_MidiEditorData uph_midi_editor_data;

void uph_midi_editor_init(void)
{
	Uph_MidiEditorData *editor = &uph_midi_editor_data;

	uph_midi_note_input_cancel_drag(editor);
	uph_midi_velocity_cancel_drag(&editor->velocity_lane);
	memset(editor, 0, sizeof(*editor));
	uph_note_map_init_default(&editor->note_map);
	uph_piano_init(&editor->piano);
	uph_midi_velocity_init(&editor->velocity_lane);
	editor->audition_key = -1;

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
		uph_midi_preview_note_off((uint8_t)events.released);

	if (events.pressed != -1)
		uph_midi_preview_note_on((uint8_t)events.pressed, (uint8_t)uph_midi_editor_data.note_input.last_note_velocity);
}

static void _uph_midi_editor_set_audition(Uph_MidiEditorData *editor, int16_t key, uint8_t velocity)
{
	if (key == editor->audition_key)
		return;

	if (editor->audition_key != -1)
		uph_midi_preview_note_off((uint8_t)editor->audition_key);

	if (key != -1)
		uph_midi_preview_note_on((uint8_t)key, velocity);

	editor->audition_key = key;
}

static void _uph_midi_editor_update_audition(Uph_MidiEditorData *editor, const Uph_MidiPattern *pattern)
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
	Uph_MidiEditorData *editor = &uph_midi_editor_data;
	uph_midi_note_input_cancel_drag(editor);
	uph_midi_note_input_cancel_erase(editor);
	uph_midi_velocity_cancel_drag(&editor->velocity_lane);
	editor->velocity_lane.resizing = false;
	editor->note_input.marquee.active = false;
	_uph_midi_editor_send_preview(uph_piano_release_mouse(&editor->piano));
	_uph_midi_editor_set_audition(editor, -1, 0);
	editor->playhead_dragging = false;
	uph_midi_preview_pause();
}

#pragma endregion

#pragma region Pattern playback

void uph_midi_editor_toggle_playback(void)
{
	if (uph_midi_preview_playing())
	{
		uph_midi_preview_pause();
		return;
	}

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN || !uph_note_pattern(uph_state.shared.selected_resource.index))
		return;

	uph_state.shared.song_timeline_playing = false;
	uph_midi_editor_data.playing_pattern = uph_state.shared.selected_resource.index;
	uph_midi_preview_play(uph_midi_editor_data.playing_pattern);
}

void uph_midi_editor_stop_playback(void)
{
	uph_midi_preview_stop();
}

static void _uph_midi_editor_update_playback(Uph_MidiEditorData *editor)
{
	uph_midi_preview_refresh_voice();

	if (editor->panel_focused && naui_key_pressed(NAUI_KEY_SPACE) && !uph_ui_any_menu_open())
		uph_midi_editor_toggle_playback();

	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN || editor->playing_pattern == uph_state.shared.selected_resource.index)
		return;

	editor->playing_pattern = uph_state.shared.selected_resource.index;
	uph_midi_preview_seek(0.0);
	if (uph_midi_preview_playing())
		uph_midi_preview_play(editor->playing_pattern);
}

static void _uph_midi_editor_update_playhead_drag(Uph_MidiEditorData *editor, Leaf_BoundingBox box)
{
	const Naui_Vec2 mouse = { (float)naui_mouse_x(), (float)naui_mouse_y() };
	const bool over_ruler = uph_song_timeline_vec4_contains_vec2((Naui_Vec4) { box.x, box.y, box.width, box.height }, mouse);

	if (!editor->playhead_dragging && over_ruler && editor->panel_hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
	{
		editor->playhead_dragging = true;
		editor->playhead_drag_beat = -1.0;
	}

	if (!editor->playhead_dragging)
		return;

	double beat = fmax(0.0, uph_snap_beat_floor(uph_midi_view_x_to_beat(&editor->view, box, mouse.x), editor->snap_resolution));
	const Uph_MidiPattern *pattern = uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN ? uph_note_pattern(uph_state.shared.selected_resource.index) : NULL;
	if (pattern)
	{
		const double length = uph_midi_preview_loop_length(pattern);
		if (beat >= length)
			beat = fmax(0.0, length - uph_snap_division(editor->snap_resolution));
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
	const Uph_MidiEditorData *editor = &uph_midi_editor_data;
	const Leaf_BoundingBox lanes = editor->lanes_bounding_box;
	const float x = uph_midi_view_beat_to_x(&editor->view, lanes, uph_midi_preview_position());
	const float size = NAUI_DPI(16.0f);
	const float half_size = size * 0.5f;
	const float ruler_bottom = box.y + (float)(NAUI_DPI(UPH_MIDI_EDITOR_TOP_RULER_HEIGHT) - 2);
	const Naui_Color color = naui_theme_color("uph_playhead_color");

	naui_push_clip_rect(lanes.x - NAUI_DPI(1), box.y, lanes.width + NAUI_DPI(1), box.height);
	naui_draw_line((Naui_Vec2) { x, ruler_bottom - half_size }, (Naui_Vec2) { x, box.y + box.height }, color, NAUI_DPI(1.0f));
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

static void _uph_midi_editor_render_loop_end(const Uph_MidiEditorData *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern)
{
	const float x = uph_midi_view_beat_to_x(&editor->view, box, uph_midi_preview_loop_length(pattern));
	if (x > box.x + box.width)
		return;

	const float start = fmaxf(x, box.x);
	naui_fill_rect((Naui_Vec2) { start, box.y }, (Naui_Vec2) { box.x + box.width - start, box.height }, leaf_rgba(0, 0, 0, 40), 0.0f, NAUI_CORNER_NONE);
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
	_uph_midi_editor_update_audition(editor, pattern);
	_uph_midi_editor_render_loop_end(editor, box, pattern);
	uph_midi_notes_render(box, &editor->view, &editor->note_map, pattern);
	uph_midi_notes_render_marquee(&editor->view, box, &editor->note_input.marquee);
	naui_pop_clip_rect();
}

static void _uph_midi_editor_ruler_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorData *editor = &uph_midi_editor_data;
	_uph_midi_editor_update_playhead_drag(editor, box);
	uph_midi_grid_render_ruler(box, &editor->view, editor->snap_resolution);
}

static void _uph_midi_editor_velocity_header_draw(Leaf_BoundingBox box, void *user_data)
{
	uph_midi_velocity_render_header(box);
}

static void _uph_midi_editor_velocity_draw(Leaf_BoundingBox box, void *user_data)
{
	Uph_MidiEditorData *editor = &uph_midi_editor_data;
	Uph_MidiPattern *pattern = uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN ? uph_note_pattern(uph_state.shared.selected_resource.index) : NULL;

	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	if (pattern)
		uph_midi_velocity_update(editor, box, pattern);

	uph_midi_velocity_render(editor, box, pattern);
	if (pattern)
		_uph_midi_editor_render_loop_end(editor, box, pattern);

	naui_pop_clip_rect();
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

void uph_midi_editor_update(void)
{
	Uph_MidiEditorData *editor = &uph_midi_editor_data;

	const Leaf_ID lanes_id = leaf_id("uph_midi_editor_lanes");
	const Leaf_ID piano_id = leaf_id("uph_midi_editor_piano");
	const Leaf_ID velocity_id = leaf_id("uph_midi_editor_velocity");
	const Leaf_ID splitter_id = leaf_id("uph_midi_editor_velocity_splitter");

	editor->panel_hovered = naui_panel_is_hovered(naui_panel_current());
	editor->panel_focused = naui_panel_focused() == naui_panel_current();
	editor->lanes_hovered = leaf_hovered(lanes_id) && editor->panel_hovered;
	editor->piano_hovered = leaf_hovered(piano_id) && editor->panel_hovered;
	editor->velocity_lane.hovered = editor->velocity_lane.visible && leaf_hovered(velocity_id) && editor->panel_hovered;
	editor->lanes_bounding_box = leaf_get_bounding_box(lanes_id);

	if (editor->note_input.marquee.active && !naui_mouse_down(NAUI_MOUSE_LEFT))
		editor->note_input.marquee.active = false;

	if (!editor->velocity_lane.visible)
	{
		uph_midi_velocity_cancel_drag(&editor->velocity_lane);
		editor->velocity_lane.resizing = false;
	}

	_uph_midi_editor_update_playback(editor);
	_uph_midi_editor_update_view_input(editor, editor->lanes_bounding_box);
	uph_midi_note_input_update_shortcuts(editor);
	uph_midi_toolbar_render(editor);

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW}
	})
	{
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

		if (editor->velocity_lane.visible)
		{
			uph_midi_velocity_update_splitter(editor, splitter_id);
			leaf({
				.id = splitter_id,
				.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIDI_EDITOR_VELOCITY_SPLITTER_HEIGHT))},
				.color = naui_theme_color(editor->velocity_lane.splitter_hovered || editor->velocity_lane.resizing ? "uph_ui_frame_hovered_bg_color" : "uph_toolbox_border_color")
			});

			leaf({
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(editor->velocity_lane.height))},
				.child_gap = 1.0f,
				.clip_children = true
			})
			{
				leaf({
					.size = {LEAF_SIZE_FIXED(uph_piano_width(&editor->piano)), LEAF_SIZE_FULL},
					.custom_draw = _uph_midi_editor_velocity_header_draw
				});
				leaf({
					.id = velocity_id,
					.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
					.custom_draw = _uph_midi_editor_velocity_draw
				});
			}
		}

		leaf({
			.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
			.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
			.custom_draw = _uph_midi_editor_playhead_overlay_draw
		});
	}
}
