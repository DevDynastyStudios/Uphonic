NAUI_PANEL(uph_welcome)

static void uph_welcome_sidebar()
{
	leaf({
		.size = { .width = LEAF_SIZE_PERCENT(0.12f), .height = LEAF_SIZE_GROW },
		.color = naui_theme_color("naui_widget_toggle_on_color"),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{

	}
}

static void uph_welcome_top_bar()
{
	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_PERCENT(0.1f) },
		.color = naui_theme_color("naui_widget_accent_color"),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{

	}
}

static void uph_welcome_content()
{
	leaf({
		.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
		.color = naui_theme_color("naui_panel_close_hovered_bg_color"),
		.rounding = { .value = 32, .corners = LEAF_CORNER_ALL },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{

	}
}

static void uph_welcome_on_attach(void)
{
    Naui_PanelID self = naui_panel_current();
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
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.color = naui_theme_color("uphonic_accent_color"),
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		welcome_sidebar();

		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
			.direction = LEAF_DIRECTION_VERTICAL
		})
		{
			welcome_top_bar();

			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
				.padding = LEAF_PADDING_ALL(6)
			})
			{
				welcome_content();
			}
		}
	}
}