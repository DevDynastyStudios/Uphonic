NAUI_PANEL(uph_plugin_list)

typedef struct
{
    Naui_List(Uph_PluginInfo) plugin_infos;

    Naui_String filter;

    int32_t current_plugin_index;
}
Uph_PluginListData;

static Uph_PluginListData uph_plugin_list_data = { 0 };

void uph_plugin_list_on_attach(void)
{
    const Naui_PanelID panel_id = naui_current_panel();
    naui_panel_set_title(panel_id, NAUI_TR("plugin.menu.title"));
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_UNDOCK);
    uph_plugin_cache_get(&uph_plugin_list_data.plugin_infos, false);
}

void uph_plugin_list_on_detach(void)
{
    
}

void uph_plugin_list_on_open(void)
{
	if (naui_list_len(uph_state.settings.plugin.plugin_paths) == 0)
		return;
}

void uph_plugin_list_on_close(void)
{
    uph_plugin_list_data.filter = (Naui_String){0};
}

static void uph_plugin_list_load(void)
{
    if (uph_state.shared.plugin_list_for_track_instrument)
    {
        uph_state.shared.current_plugin_list_track->instrument = uph_load_plugin(uph_plugin_list_data.plugin_infos[uph_plugin_list_data.current_plugin_index].path);
        uph_state.shared.current_plugin_list_track->type = UPH_RESOURCE_PATTERN;
    }
    else
    {
        Uph_EffectPlugin effect = {
            .plugin = uph_load_plugin(uph_plugin_list_data.plugin_infos[uph_plugin_list_data.current_plugin_index].path),
            .enabled = true
        };
        naui_list_push(
            uph_state.shared.current_plugin_list_track->effects,
            effect
        );
    }
    naui_close_panel(naui_current_panel());
}

static void uph_plugin_list_item(const Uph_PluginInfo *info, uint32_t item_index)
{
    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");

    const Leaf_ID id = leaf_id_indexed("uph_plugin_list_item", item_index);
    bool hovered = uph_ui_widget_hovered(id);
    if (hovered)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
            uph_plugin_list_data.current_plugin_index = item_index;
        if (naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
            uph_plugin_list_load();
        naui_set_cursor(NAUI_CURSOR_HAND);
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = (hovered || uph_plugin_list_data.current_plugin_index == item_index) ?
            naui_theme_color("uph_ui_frame_secondary_bg_color") : naui_theme_color("uph_ui_frame_bg_color"),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .clip_children = true
        })
        {
            leaf_text(info->name.data, {
                .font_size = font_size,
                .color = text_color
            });
        }
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .clip_children = true
        })
        {
            leaf_text(info->vendor.data, {
                .font_size = font_size,
                .color = text_color
            });
        }
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .clip_children = true
        })
        {
            leaf_text(info->type == UPH_PLUGIN_INSTRUMENT ? NAUI_TR("plugin.menu.instrument.short") : NAUI_TR("plugin.menu.effect.short"), {
                .font_size = font_size,
                .color = text_color
            });
        }
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .clip_children = true
        })
        {
            leaf({
                .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
                .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
                .color = info->format == UPH_PLUGIN_VST3 ?
                    naui_theme_color("uph_plugin_list_vst3_color") :
                    naui_theme_color("uph_plugin_list_clap_color")
            })
            {
                leaf_text(info->format == UPH_PLUGIN_VST3 ? "VST3" : "CLAP", {
                    .font_size = font_size,
                    .color = naui_theme_color("uph_plugin_list_text_color")
                });
            }
        }
    }
}

static bool uph_plugin_list_matches_filter(const Uph_PluginInfo *info)
{
    if (uph_plugin_list_data.filter.length == 0)
        return true;

    return naui_string_contains(info->name, uph_plugin_list_data.filter, false)
        || naui_string_contains(info->vendor, uph_plugin_list_data.filter, false);
}

static void uph_plugin_list_main_menu(void)
{
    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .clip_children = true
        }) leaf_text(NAUI_TR("plugin.menu.name"), { .font_size = font_size, .color = text_color });
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .clip_children = true
        }) leaf_text(NAUI_TR("plugin.menu.vendor"), { .font_size = font_size, .color = text_color });
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .clip_children = true
        }) leaf_text(NAUI_TR("plugin.menu.type"), { .font_size = font_size, .color = text_color });
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .clip_children = true
        }) leaf_text(NAUI_TR("plugin.menu.format"), { .font_size = font_size, .color = text_color });
    }

    static float scroll = 0.0;
    Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
        UPH_UI_SCROLL_DIRECTION_VERTICAL,
        &scroll,
        leaf_id("uph_plugin_list_scrollbar")
    );
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_plugin_list_data.plugin_infos); i++)
    {
        Uph_PluginInfo info = uph_plugin_list_data.plugin_infos[i];
        if (!uph_plugin_list_matches_filter(&info))
            continue;
        uph_plugin_list_item(&info, i);
    }
    uph_ui_end_scroll_container(&scroll_container);
}

static void uph_plugin_list_current_menu(void)
{
    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color text_color = naui_theme_color("uph_ui_text_color");

    leaf({
        .size = {LEAF_SIZE_PERCENT(0.25f), LEAF_SIZE_FULL},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .border = {
            .width = 1,
            .color = naui_theme_color("uph_ui_frame_border"),
            .sides = LEAF_SIDE_LEFT
        },
        .clip_children = true
    })
    {

        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
            .child_gap = NAUI_DPI(6)
        })
        {
            const Uph_PluginInfo *info = &uph_plugin_list_data.plugin_infos[uph_plugin_list_data.current_plugin_index];
            leaf_text(info->name.data, { .font_size = font_size * 2, .color = text_color });
            leaf_text(info->vendor.data, { .font_size = font_size, .color = text_color });
            leaf_text(info->type == UPH_PLUGIN_INSTRUMENT ? NAUI_TR("plugin.menu.instrument") : NAUI_TR("plugin.menu.effect"), { .font_size = font_size, .color = text_color });
            leaf_text(info->format == UPH_PLUGIN_VST3 ? "VST3" : "CLAP", { .font_size = font_size, .color = text_color });
        }
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_PERCENT(0.3f)},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        })
        {
            if (uph_ui_text_button(NAUI_TR("plugin.menu.load"), leaf_id("uph_plugin_list_load")))
            {
                uph_plugin_list_load();
            }
        }
    }
}

void uph_plugin_list_on_update(void)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        uph_ui_textfield(&uph_plugin_list_data.filter, leaf_id("uph_plugin_list_filter"), UPH_UI_TEXTFIELD_ALWAYS_ACTIVE, "Search");
        if (uph_ui_text_button("Refresh", leaf_id("uph_plugin_list_refresh")))
            uph_plugin_cache_get(&uph_plugin_list_data.plugin_infos, true);
    }

    if (naui_list_len(uph_plugin_list_data.plugin_infos) == 0)
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        }) leaf_text(NAUI_TR("plugin.menu.none.detected"), {
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(naui_theme_float("uph_ui_font_size"))),
            .color = {naui_theme_color("uph_ui_text_color")},
            .wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
        });
        return;
    }

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .border = {
            .width = 1,
            .color = naui_theme_color("uph_ui_frame_border"),
            .sides = LEAF_SIDE_TOP
        },
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_GROW}
        })
        {
            uph_plugin_list_main_menu();
        }
        uph_plugin_list_current_menu();
    }
}