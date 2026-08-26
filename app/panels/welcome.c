NAUI_PANEL(uph_welcome)

static void uph_welcome_sidebar()
{
	leaf({
		.size = { LEAF_SIZE_PERCENT(0.12f), LEAF_SIZE_GROW },
		.direction = LEAF_DIRECTION_VERTICAL,
		.color = naui_theme_color("naui_widget_toggle_on_color"),
	})
	{

	}
}

static void uph_welcome_top_bar()
{
	leaf({
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_PERCENT(0.1f) },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.color = naui_theme_color("naui_widget_accent_color"),
	})
	{

	}
}

static void uph_welcome_content()
{
	leaf({
		.size = { LEAF_SIZE_GROW, LEAF_SIZE_GROW },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.color = naui_theme_color("naui_panel_close_hovered_bg_color"),
		.rounding = { 32, LEAF_CORNER_ALL }
	})
	{

	}
}

static void uph_welcome_on_attach(void)
{
    Naui_PanelID self = naui_current_panel();
    naui_panel_set_title(self, "Welcome");
    naui_panel_enable_flags(self, NAUI_PANEL_FLAG_NO_UNDOCK | NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_TITLE);
}

static void uph_welcome_on_detach(void)
{

}

static void uph_welcome_on_open(void)
{
    
}

static void uph_welcome_on_close(void)
{
    
}

static void uph_welcome_on_update(void)
{
	leaf({
		.size = { LEAF_SIZE_FULL, LEAF_SIZE_FULL },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.color = naui_theme_color("uphonic_accent_color"),
	})
	{
		welcome_sidebar();

		leaf({
			.size = { LEAF_SIZE_GROW, LEAF_SIZE_GROW },
			.direction = LEAF_DIRECTION_VERTICAL,
		})
		{
			welcome_top_bar();

			leaf({
				.size = { LEAF_SIZE_GROW, LEAF_SIZE_GROW },
				.padding = LEAF_PADDING_ALL(6)
			})
			{
				welcome_content();
			}
		}
	}
}