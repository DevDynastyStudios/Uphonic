NAUI_PANEL(uph_automation_list)

#define UPH_AUTOMATION_LIST_ROW_GAP 2
#define UPH_AUTOMATION_LIST_TARGET_DOT 8

typedef struct
{
    Naui_String filter;
}
Uph_AutomationListData;

static Uph_AutomationListData uph_automation_list_data = { 0 };

void uph_automation_list_on_attach(void)
{
    const Naui_PanelID panel_id = naui_panel_current();
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_UNDOCK);
}

void uph_automation_list_on_detach(void)
{
    
}

void uph_automation_list_on_open(void)
{

}

void uph_automation_list_on_close(void)
{
    uph_automation_list_data.filter = (Naui_String){0};
}

static bool uph_automation_list_matches_filter(const Uph_PluginParam *param, const Uph_Plugin *plugin)
{
    if (uph_automation_list_data.filter.length == 0)
        return true;

    return naui_string_contains(param->name, uph_automation_list_data.filter, false)
        || naui_string_contains(plugin->name, uph_automation_list_data.filter, false);
}

static bool uph_automation_list_param_visible(const Uph_PluginParam *param, const Uph_Plugin *plugin)
{
    return !param->used && uph_automation_list_matches_filter(param, plugin);
}

static bool uph_automation_list_plugin_has_visible_params(const Uph_Plugin *plugin)
{
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(plugin->params); i++)
    {
        if (uph_automation_list_param_visible(&plugin->params[i], plugin))
            return true;
    }
    return false;
}

static Naui_Color uph_automation_list_muted_color(void)
{
    return naui_theme_color("naui_panel_text_color");
}

static void uph_automation_list_target_bar(const Uph_Track *track)
{
    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color accent = uph_resources_track_color(track->color_index);

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2.0f), NAUI_DPI(padding.y * 1.5f)),
        .color = uph_color_mix(naui_theme_color("uph_ui_frame_bg_color"), accent, 0.12f),
        .border = {
            .width = 1,
            .color = naui_theme_color("uph_ui_frame_border"),
            .sides = LEAF_SIDE_BOTTOM
        },
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .child_gap = NAUI_DPI(8)
    })
    {
        leaf_text(NAUI_TR("automation.menu.target"), { .font_size = font_size, .color = uph_automation_list_muted_color() });
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_TARGET_DOT)), LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_TARGET_DOT))},
            .color = accent,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
        leaf_text(track->name.data, { .font_size = font_size, .color = naui_theme_color("uph_ui_text_color") });
    }
}

static bool uph_automation_list_param_row(const char *name, const Leaf_ID id)
{
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const bool hovered = uph_ui_widget_hovered(id);

    Naui_Color color = { 0 };
    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        color = naui_mouse_down(NAUI_MOUSE_LEFT) ?
            naui_theme_color("uph_ui_frame_pressed_bg_color") :
            naui_theme_color("uph_ui_frame_secondary_bg_color");
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2.0f), NAUI_DPI(padding.y)),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
        .clip_children = true
    })
    {
        leaf_text(name, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = naui_theme_color("uph_ui_text_color")
        });
    }

    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static bool uph_automation_list_render_plugin_params(Uph_Plugin *plugin, Uph_Track *parent_track, int32_t effect_index, uint32_t *row_index)
{
    if (!uph_automation_list_plugin_has_visible_params(plugin))
        return false;

    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .child_gap = NAUI_DPI(UPH_AUTOMATION_LIST_ROW_GAP)
    })
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2.0f), NAUI_DPI(padding.y)),
            .color = naui_theme_color("uph_resource_list_section_bg_color"),
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .child_gap = NAUI_DPI(padding.x)
        })
        {
            leaf({
                .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
                .clip_children = true
            }) leaf_text(plugin->name.data, { .font_size = font_size, .color = naui_theme_color("uph_resource_list_section_text_color") });
            leaf_text(effect_index < 0 ? NAUI_TR("plugin.menu.instrument") : NAUI_TR("plugin.menu.effect"), {
                .font_size = font_size,
                .color = uph_automation_list_muted_color()
            });
        }

        for (uint32_t i = 0; i < (uint32_t)naui_list_len(plugin->params); i++)
        {
            Uph_PluginParam *param = &plugin->params[i];
            if (!uph_automation_list_param_visible(param, plugin))
                continue;

            if (uph_automation_list_param_row(param->name.data, leaf_id_indexed("uph_automation_list_param", (*row_index)++)))
            {
                Uph_ActionTrackAutomationCreate lane = {
                    .parent = uph_action_track_ref(parent_track),
                    .name = param->name,
                    .effect_index = effect_index,
                    .param_id = param->id
                };
                naui_action_execute_stack(UPH_ACTION_TRACK_AUTOMATION_CREATE, lane);
                naui_panel_close(naui_panel_current());
            }
        }
    }
    return true;
}

void uph_automation_list_on_update(void)
{
	naui_panel_set_title(naui_panel_current(), NAUI_TR("automation.menu.title"));
    Uph_Track *track = uph_state.shared.current_automation_list_track;
    if (!track)
        return;

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    uph_automation_list_target_bar(track);

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2.0f), NAUI_DPI(padding.y * 1.5f)),
        .border = {
            .width = 1,
            .color = naui_theme_color("uph_ui_frame_border"),
            .sides = LEAF_SIDE_BOTTOM
        }
    })
    {
        uph_ui_textfield(&uph_automation_list_data.filter, leaf_id("uph_automation_list_filter"), UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE, "Search");
    }

    static float scroll = 0.0f;
    Uph_UIScrollContainer container = uph_ui_begin_scroll_container(
        UPH_UI_SCROLL_DIRECTION_VERTICAL,
        &scroll,
        leaf_id("uph_automation_list_scrollbar")
    );
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x)),
        .child_gap = NAUI_DPI(padding.y * 2.0f)
    })
    {
        uint32_t row_index = 0;
        bool any_visible = uph_automation_list_render_plugin_params(&track->instrument, track, -1, &row_index);
        for (uint32_t i = 0; i < naui_list_len(track->effects); i++)
            any_visible |= uph_automation_list_render_plugin_params(&track->effects[i].plugin, track, i, &row_index);

        if (!any_visible)
        {
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.y * 2.0f)),
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                leaf_text(NAUI_TR("automation.menu.none"), {
                    .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
                    .color = uph_automation_list_muted_color()
                });
            }
        }
    }
    uph_ui_end_scroll_container(&container);
}