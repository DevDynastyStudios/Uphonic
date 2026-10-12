NAUI_PANEL(uph_resource_editor)

static void uph_resource_editor_on_attach(void)
{
	uph_midi_editor_init(&uph_midi_editor_data);
	uph_sample_editor_init();
}

static void uph_resource_editor_on_detach(void)
{
	uph_midi_editor_reset_interaction(&uph_midi_editor_data);
	uph_sample_editor_reset_interaction();
}

static void uph_resource_editor_on_open(void)
{

}

static void uph_resource_editor_on_close(void)
{

}

static void uph_midi_editor_render_empty(const char *message)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
	})
	{
		leaf_text(message, {
			.color = {naui_theme_color("uph_ui_text_color")},
			.font_size = {NAUI_DPI(naui_theme_float("uph_ui_font_size"))}
		});
	}
}

static void uph_resource_editor_on_update(void)
{
	naui_panel_set_title(naui_current_panel(), NAUI_TR("resource_editor.title"));
	switch (uph_state.shared.selected_resource.type)
	{
		case UPH_RESOURCE_PATTERN:
			uph_sample_editor_reset_interaction();
			uph_midi_editor_update(&uph_midi_editor_data);
			break;
		case UPH_RESOURCE_SAMPLE:
			uph_sample_editor_update();
			break;
		default:
			uph_sample_editor_reset_interaction();
			uph_midi_editor_reset_interaction(&uph_midi_editor_data);
			uph_midi_editor_render_empty("No midi pattern is selected.");
			break;
	}
}