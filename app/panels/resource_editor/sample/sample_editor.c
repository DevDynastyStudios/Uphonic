void uph_sample_editor_init(void)
{

}

void uph_sample_editor_reset_interaction(void)
{

}

void uph_sample_editor_update(void)
{
	Uph_Sample *sample = &uph_state.project.samples[uph_state.shared.selected_resource.index];
	uph_ui_drag_int(&sample->pitch, leaf_id("uph_sample_editor_pitch"), 1, -1200, 1200, "%i", UPH_UI_DRAG_CLAMPED);
	uph_ui_drag_float(&sample->time_scale, leaf_id("uph_sample_editor_time_scale"), 0.01f, 0.0001f, 10000.0f, "%.4f", UPH_UI_DRAG_CLAMPED);
	
	static const char *sample_modes[] = {
		"Resample",
		"Stretch"
	};
	uph_ui_dropdown(sample_modes, 2, (uint32_t*)&sample->mode, leaf_id("uph_sample_editor_mode"));
}
