uint32_t uph_midi_editor_find_scale(const char *name)
{
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.scales); i++)
	{
		if (naui_string_equals_cstr(uph_state.scales[i].name, name, true))
			return i;
	}

	return 0;
}

void uph_midi_editor_set_scale(Uph_MidiEditorData *editor, uint32_t scale_index, uint8_t root)
{
	if (scale_index >= (uint32_t)naui_list_len(uph_state.scales))
		return;

	editor->scale_index = scale_index;
	editor->scale_root = root % UPH_NOTES_PER_OCTAVE;
	uph_note_map_apply_scale(&editor->note_map, uph_state.scales[scale_index].mask, editor->scale_root);
}

void uph_midi_editor_quantize(Uph_MidiEditorData *editor)
{
	if (uph_state.shared.selected_resource.type != UPH_RESOURCE_PATTERN)
		return;

	uph_note_quantize(uph_state.shared.selected_resource.index, editor->snap_resolution, editor->view.zoom.x);
}

static void _uph_midi_menu_bar_edit(Uph_MidiEditorData *editor, Uph_MidiPattern *pattern)
{
	Uph_UIMenuID menu = uph_ui_menu("Edit", leaf_id("uph_midi_menu_edit"));
	const Uph_ResourceIndex pattern_index = uph_state.shared.selected_resource.index;
	const Uph_KeybindSettings *keybinds = &uph_state.settings.keybinds;

	if (uph_ui_menu_shortcut_item(menu, "Select All", leaf_id("uph_midi_menu_select_all"), keybinds->select_all) && pattern)
		uph_note_select_all(pattern, true);

	if (uph_ui_menu_shortcut_item(menu, "Deselect", leaf_id("uph_midi_menu_deselect"), keybinds->deselect) && pattern)
		uph_note_select_all(pattern, false);

	if (uph_ui_menu_shortcut_item(menu, "Duplicate", leaf_id("uph_midi_menu_duplicate"), keybinds->duplicate) && pattern)
		uph_note_duplicate_selected(pattern_index);

	if (uph_ui_menu_shortcut_item(menu, "Delete", leaf_id("uph_midi_menu_delete"), keybinds->delete_selected) && pattern)
		uph_note_delete_selected(pattern_index);
}

static void _uph_midi_menu_bar_tools(Uph_MidiEditorData *editor)
{
	Uph_UIMenuID menu = uph_ui_menu("Tools", leaf_id("uph_midi_menu_tools"));

	if (uph_ui_menu_shortcut_item(menu, "Quantize", leaf_id("uph_midi_menu_quantize"), uph_state.settings.keybinds.quantize))
		uph_midi_editor_quantize(editor);
}

static void _uph_midi_menu_bar_scale(Uph_MidiEditorData *editor)
{
	Uph_UIMenuID menu = uph_ui_menu("Scale", leaf_id("uph_midi_menu_scale"));

	Uph_UIMenuID root_menu = uph_ui_submenu(menu, "Root", leaf_id("uph_midi_menu_scale_root"));
	for (uint8_t i = 0; i < UPH_NOTES_PER_OCTAVE; i++)
	{
		if (uph_ui_menu_check_item(root_menu, uph_note_pitch_class_name(i), leaf_id_indexed("uph_midi_menu_scale_root_item", i), editor->scale_root == i))
			uph_midi_editor_set_scale(editor, editor->scale_index, i);
	}

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_state.scales); i++)
	{
		if (uph_ui_menu_check_item(menu, uph_state.scales[i].name.data, leaf_id_indexed("uph_midi_menu_scale_item", i), editor->scale_index == i))
			uph_midi_editor_set_scale(editor, i, editor->scale_root);
	}
}

static void _uph_midi_menu_bar_view(Uph_MidiEditorData *editor)
{
	Uph_UIMenuID menu = uph_ui_menu("View", leaf_id("uph_midi_menu_view"));

	if (uph_ui_menu_check_item(menu, "Velocity Lane", leaf_id("uph_midi_menu_velocity_lane"), editor->velocity_lane.visible))
		editor->velocity_lane.visible = !editor->velocity_lane.visible;

	if (uph_ui_menu_item(menu, "Reset Scroll", leaf_id("uph_midi_menu_reset_scroll")))
		uph_midi_editor_reset_scroll();
}

static void _uph_midi_menu_bar_render_scale_label(const Uph_MidiEditorData *editor)
{
	if (editor->scale_index >= (uint32_t)naui_list_len(uph_state.scales))
		return;

	const Uph_Scale *scale = &uph_state.scales[editor->scale_index];
	const Naui_String label = scale->mask == UPH_SCALE_CHROMATIC_MASK ?
		scale->name :
		naui_string_format("%s %s", uph_note_pitch_class_name(editor->scale_root), scale->name.data);

	leaf({
		.size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL}
	});
	leaf_text(label.data, {
		.font_size = NAUI_DPI(UPH_UI_FONT_SIZE),
		.color = naui_theme_color("uph_ui_text_color")
	});
}

void uph_midi_menu_bar_render(Uph_MidiEditorData *editor)
{
	Uph_MidiPattern *pattern = uph_state.shared.selected_resource.type == UPH_RESOURCE_PATTERN ? uph_note_pattern(uph_state.shared.selected_resource.index) : NULL;

	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_MIDI_MENU_BAR_HEIGHT))},
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_TOOLBOX_PADDING_X) * 0.5f, 0.0f),
		.child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
		.color = {naui_theme_color("uph_toolbox_bg_color")},
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		_uph_midi_menu_bar_edit(editor, pattern);
		_uph_midi_menu_bar_tools(editor);
		_uph_midi_menu_bar_scale(editor);
		_uph_midi_menu_bar_view(editor);
		_uph_midi_menu_bar_render_scale_label(editor);
	}
}
