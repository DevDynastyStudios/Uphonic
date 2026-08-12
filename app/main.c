NAUI_APP("Uphonic")

Uph_State state = { 0 };

void naui_app_start(void)
{
	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	naui_set_main_viewport(NAUI_ATTACH_PANEL(welcome));
}

void naui_app_end(void)
{

}

void naui_app_update(void)
{
	naui_widgets_reset();

	naui_render_main_titlebar("Uphonic");
	naui_render_panels_and_viewport();
}
