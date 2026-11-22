NAUI_PANEL(uph_automation_list)

#define UPH_AUTOMATION_LIST_ROW_GAP 2
#define UPH_AUTOMATION_LIST_SECTION_GAP 6
#define UPH_AUTOMATION_LIST_TARGET_DOT 8
#define UPH_AUTOMATION_LIST_ICON_SIZE 14
#define UPH_AUTOMATION_LIST_CHEVRON_SIZE 10
#define UPH_AUTOMATION_LIST_ROW_HEIGHT 26
#define UPH_AUTOMATION_LIST_HEADER_HEIGHT 30
#define UPH_AUTOMATION_LIST_METER_WIDTH 44
#define UPH_AUTOMATION_LIST_METER_HEIGHT 4
#define UPH_AUTOMATION_LIST_WIDTH 420
#define UPH_AUTOMATION_LIST_HEIGHT 720

typedef struct
{
    Naui_String filter;
    Naui_String last_filter;
    uint64_t collapsed;
    int32_t highlighted;
    uint32_t row_count;
    int32_t last_mouse_x, last_mouse_y;
    float scroll;
    bool scroll_to_highlight;
    bool activate;
}
Uph_AutomationListState;

typedef struct
{
    Uph_AutomationTarget target;
    const char *name;
}
Uph_AutomationListTarget;

static Uph_AutomationListState uph__automation_list_state = { 0 };

void uph_automation_list_on_attach(void)
{
    const Naui_PanelID panel_id = naui_panel_current();
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_UNDOCK);
    naui_panel_set_size(panel_id, (Naui_Vec2){ NAUI_DPI(UPH_AUTOMATION_LIST_WIDTH), UPH_AUTOMATION_LIST_HEIGHT });
}

void uph_automation_list_on_detach(void)
{

}

void uph_automation_list_on_open(void)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    state->collapsed = 0;
    state->highlighted = 0;
    state->scroll = 0.0f;
    state->last_mouse_x = naui_mouse_x();
    state->last_mouse_y = naui_mouse_y();
}

void uph_automation_list_on_close(void)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    uph_song_timeline_cancel_automation_drop();
    state->filter = (Naui_String){0};
    state->last_filter = (Naui_String){0};
}

static bool uph_automation_list_searching(void)
{
    return uph__automation_list_state.filter.length > 0;
}

static bool uph_automation_list_section_collapsed(uint32_t section)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    return !uph_automation_list_searching() && section < 64 && (state->collapsed & (1ull << section));
}

static bool uph_automation_list_param_visible(const Uph_PluginParam *param, const Uph_Plugin *plugin)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    if (param->used)
        return false;

    return state->filter.length == 0
        || naui_string_contains(param->name, state->filter, false)
        || naui_string_contains(plugin->name, state->filter, false);
}

static uint32_t uph_automation_list_plugin_visible_count(const Uph_Plugin *plugin)
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(plugin->params); i++)
        count += uph_automation_list_param_visible(&plugin->params[i], plugin);
    return count;
}

static bool uph_automation_list_track_target_visible(const Uph_Track *track, Uph_AutomationTarget target, const char *name)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    if (track && uph_resources_track_has_automation_target(track, target))
        return false;

    return state->filter.length == 0
        || naui_string_contains(naui_string_from_cstr(name), state->filter, false);
}

static uint32_t uph_automation_list_track_visible_count(const Uph_Track *track, const Uph_AutomationListTarget *targets, uint32_t target_count)
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < target_count; i++)
        count += uph_automation_list_track_target_visible(track, targets[i].target, targets[i].name);
    return count;
}

static void uph_automation_list_target_bar(const char *name, Naui_Color accent)
{
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2), NAUI_DPI(padding.y * 1.5f)),
        .border = {
            .width = 1,
            .sides = LEAF_SIDE_BOTTOM,
            .color = uph_state.colors.ui_frame_border
        },
        .color = uph_color_mix(uph_state.colors.ui_frame_bg, accent, 0.12f),
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(8),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .image = (void*)uph_state.icons.automation,
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_ICON_SIZE)), .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_ICON_SIZE)) },
            .color = uph_state.colors.panel_text
        });
        leaf_text(NAUI_TR("automation.menu.target"), { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        leaf({
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_TARGET_DOT)), .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_TARGET_DOT)) },
            .color = accent,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .clip_children = true
        }) leaf_text(name, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(font_size) });
    }
}

static void uph_automation_list_section_header(uint32_t section, const Naui_Image *icon, Naui_Color accent, const char *name, const char *kind, uint32_t count)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const Leaf_ID id = leaf_id_indexed("uph_automation_list_section", section);
    const bool searching = uph_automation_list_searching();
    const bool collapsed = uph_automation_list_section_collapsed(section);
    const bool hovered = !searching && uph_ui_widget_hovered(id);

    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && section < 64)
            state->collapsed ^= 1ull << section;
    }

    Naui_Color color = uph_state.colors.automation_list_section_bg;
    if (hovered)
        color = uph_color_mix(color, uph_state.colors.ui_frame_secondary_bg, 0.5f);

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_HEADER_HEIGHT)) },
        .padding = { .left = (int32_t)NAUI_DPI(padding.x), .right = (int32_t)NAUI_DPI(padding.x) },
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .child_gap = NAUI_DPI(padding.x),
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .clip_children = true
    })
    {
        leaf({
            .image = (void*)(collapsed ? uph_state.icons.arrow_right : uph_state.icons.arrow_down),
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_CHEVRON_SIZE)), .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_CHEVRON_SIZE)) },
            .color = searching ? LEAF_COLOR_TRANSPARENT : uph_state.colors.panel_text
        });
        leaf({
            .image = (void*)icon,
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_ICON_SIZE)), .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_ICON_SIZE)) },
            .color = accent
        });
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .clip_children = true
        }) leaf_text(name, { .color = uph_state.colors.automation_list_section_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        if (kind)
            leaf_text(kind, { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        uph_ui_count_badge(count);
    }
}

static void uph_automation_list_value_meter(float value, Naui_Color accent)
{
    leaf({
        .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_METER_WIDTH)), .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_METER_HEIGHT)) },
        .color = uph_state.colors.ui_frame_bg,
        .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL),
        .clip_children = true
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_PERCENT(NAUI_CLAMP(value, 0.0f, 1.0f)), .height = LEAF_SIZE_FULL },
            .color = accent,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
    }
}

static bool uph_automation_list_param_row(const char *name, float value, Naui_Color accent)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const uint32_t row = state->row_count++;
    const Leaf_ID id = leaf_id_indexed("uph_automation_list_param", row);
    const bool hovered = uph_ui_widget_hovered(id);
    const bool mouse_moved = naui_mouse_x() != state->last_mouse_x || naui_mouse_y() != state->last_mouse_y;

    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        if (mouse_moved)
            state->highlighted = (int32_t)row;
    }

    const bool highlighted = state->highlighted == (int32_t)row;

    Naui_Color color = { 0 };
    if (hovered && naui_mouse_down(NAUI_MOUSE_LEFT))
        color = uph_state.colors.ui_frame_pressed_bg;
    else if (highlighted)
        color = uph_color_mix(uph_state.colors.ui_frame_bg, uph_state.colors.ui_selection, 0.3f);

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_AUTOMATION_LIST_ROW_HEIGHT)) },
        .padding = {
            .left = (int32_t)NAUI_DPI(padding.x * 2 + UPH_AUTOMATION_LIST_CHEVRON_SIZE),
            .right = (int32_t)NAUI_DPI(padding.x * 1.5f)
        },
        .border = {
            .width = highlighted ? (int32_t)NAUI_DPI(2) : 0,
            .sides = LEAF_SIDE_LEFT,
            .color = accent
        },
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .child_gap = NAUI_DPI(padding.x),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .clip_children = true
        }) uph_ui_highlighted_text(name, state->filter, uph_state.colors.ui_text, uph_state.colors.panel_tab_accent);

        if (value >= 0.0f)
            uph_automation_list_value_meter(value, highlighted ? accent : uph_state.colors.panel_text);
    }

    const bool activated = highlighted && state->activate;
    if (activated)
        state->activate = false;
    return (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT)) || activated;
}

static float uph_automation_list_param_value(const Uph_PluginParam *param)
{
    const double range = param->max_value - param->min_value;
    if (range <= 0.0)
        return -1.0f;
    return (float)((param->current_value - param->min_value) / range);
}

static bool uph_automation_list_render_plugin_params(Uph_Plugin *plugin, Uph_ActionTrackRef parent, int32_t effect_index, uint32_t section, Naui_Color accent)
{
    const uint32_t count = uph_automation_list_plugin_visible_count(plugin);
    if (count == 0)
        return false;

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .child_gap = NAUI_DPI(UPH_AUTOMATION_LIST_ROW_GAP)
    })
    {
        uph_automation_list_section_header(
            section,
            effect_index < 0 ? uph_state.icons.piano : uph_state.icons.wave,
            accent,
            plugin->name.data,
            effect_index < 0 ? NAUI_TR("plugin.menu.instrument") : NAUI_TR("plugin.menu.effect"),
            count
        );

        if (!uph_automation_list_section_collapsed(section))
        {
            for (uint32_t i = 0; i < (uint32_t)naui_list_len(plugin->params); i++)
            {
                Uph_PluginParam *param = &plugin->params[i];
                if (!uph_automation_list_param_visible(param, plugin))
                    continue;

                if (uph_automation_list_param_row(param->name.data, uph_automation_list_param_value(param), accent))
                {
                    Uph_ActionTrackAutomationCreate lane = {
                        .parent = parent,
                        .name = param->name,
                        .effect_index = effect_index,
                        .param_id = param->id
                    };
                    uph_song_timeline_create_automation_lane(lane);
                    naui_panel_close(naui_panel_current());
                }
            }
        }
    }
    return true;
}

static bool uph_automation_list_render_track_params(const Uph_Track *track, Uph_ActionTrackRef parent, const Uph_AutomationListTarget *targets, uint32_t target_count, const char *section_name, Naui_Color accent)
{
    const uint32_t count = uph_automation_list_track_visible_count(track, targets, target_count);
    if (count == 0)
        return false;

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .child_gap = NAUI_DPI(UPH_AUTOMATION_LIST_ROW_GAP)
    })
    {
        uph_automation_list_section_header(0, uph_state.icons.automation, accent, section_name, NULL, count);

        if (!uph_automation_list_section_collapsed(0))
        {
            for (uint32_t i = 0; i < target_count; i++)
            {
                if (!uph_automation_list_track_target_visible(track, targets[i].target, targets[i].name))
                    continue;

                const float value = targets[i].target == UPH_AUTOMATION_TARGET_TRACK_PAN ? (track->pan + 1.0f) * 0.5f : -1.0f;
                if (uph_automation_list_param_row(targets[i].name, value, accent))
                {
                    Uph_ActionTrackAutomationCreate lane = {
                        .parent = parent,
                        .name = naui_string_from_cstr(targets[i].name),
                        .target = targets[i].target
                    };
                    uph_song_timeline_create_automation_lane(lane);
                    naui_panel_close(naui_panel_current());
                }
            }
        }
    }
    return true;
}

static void uph_automation_list_footer(void)
{
    const Uph_UIKeyHint hints[] = {
        { NAUI_TR("list.hint.navigate_keys"), NAUI_TR("list.hint.navigate") },
        { "Enter", NAUI_TR("automation.menu.automate") },
        { "Esc", NAUI_TR("list.hint.close") }
    };
    uph_ui_key_hint_bar(hints, sizeof(hints) / sizeof(hints[0]));
}

static void uph_automation_list_handle_keys(void)
{
    Uph_AutomationListState *state = &uph__automation_list_state;

    if (!naui_strings_equal(state->filter, state->last_filter, true))
    {
        state->last_filter = state->filter;
        state->highlighted = 0;
        state->scroll = 0.0f;
    }

    if (naui_key_pressed(NAUI_KEY_ESCAPE))
    {
        naui_panel_close(naui_panel_current());
        return;
    }

    if (state->row_count == 0)
        return;

    if (naui_key_pressed_repeat(NAUI_KEY_DOWN))
    {
        state->highlighted = (state->highlighted + 1) % (int32_t)state->row_count;
        state->scroll_to_highlight = true;
    }
    if (naui_key_pressed_repeat(NAUI_KEY_UP))
    {
        state->highlighted = (state->highlighted + (int32_t)state->row_count - 1) % (int32_t)state->row_count;
        state->scroll_to_highlight = true;
    }
    state->highlighted = NAUI_CLAMP(state->highlighted, 0, (int32_t)state->row_count - 1);
    state->activate = naui_key_pressed(NAUI_KEY_ENTER);
}

static void uph_automation_list_scroll_to_highlight(Leaf_ID scroll_id)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    if (!state->scroll_to_highlight)
        return;
    state->scroll_to_highlight = false;
    uph_ui_scroll_to_element(scroll_id, leaf_id_indexed("uph_automation_list_param", (uint32_t)state->highlighted), &state->scroll, NAUI_DPI(UPH_AUTOMATION_LIST_HEADER_HEIGHT));
}

void uph_automation_list_on_update(void)
{
    Uph_AutomationListState *state = &uph__automation_list_state;
    Uph_Track *track = uph_state.shared.current_automation_list_track;
    if (!track)
        return;

    const bool master = track->type == UPH_RESOURCE_MASTER;
    const Naui_Color accent = master ? uph_state.colors.ui_text_disabled : uph_resources_track_color(track->color_index);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;

    uph_automation_list_handle_keys();
    if (naui_panel_closed(naui_panel_current()))
        return;

    uph_automation_list_target_bar(master ? NAUI_TR("master.track.title") : track->name.data, accent);

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2), NAUI_DPI(padding.y * 1.5f)),
        .border = {
            .width = 1,
            .sides = LEAF_SIDE_BOTTOM,
            .color = uph_state.colors.ui_frame_border
        }
    })
    {
        uph_ui_textfield(&state->filter, leaf_id("uph_automation_list_filter"), UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE | UPH_UI_TEXTFIELD_FLAG_SEARCH, NAUI_TR("automation.menu.search"));
    }

    const Uph_AutomationListTarget targets[] = {
        { UPH_AUTOMATION_TARGET_TRACK_VOLUME, NAUI_TR("automation.target.volume") },
        { UPH_AUTOMATION_TARGET_TRACK_PAN, NAUI_TR("automation.target.pan") },
        { UPH_AUTOMATION_TARGET_TEMPO, NAUI_TR("automation.target.tempo") }
    };
    const uint32_t target_count = master ? 3 : 2;

    bool any_visible = uph_automation_list_track_visible_count(track, targets, target_count) > 0
        || uph_automation_list_plugin_visible_count(&track->instrument) > 0;
    for (uint32_t i = 0; !any_visible && i < naui_list_len(track->effects); i++)
        any_visible = uph_automation_list_plugin_visible_count(&track->effects[i].plugin) > 0;

    state->row_count = 0;
    if (!any_visible)
    {
        uph_ui_empty_state(uph_state.icons.automation, NAUI_TR(uph_automation_list_searching() ? "automation.menu.none.matching" : "automation.menu.none"));
        uph_automation_list_footer();
        state->last_mouse_x = naui_mouse_x();
        state->last_mouse_y = naui_mouse_y();
        return;
    }

    const Leaf_ID scroll_id = leaf_id("uph_automation_list_scrollbar");
    uph_automation_list_scroll_to_highlight(scroll_id);
    Uph_UIScrollContainer container = uph_ui_begin_scroll_container(UPH_UI_SCROLL_DIRECTION_VERTICAL, &state->scroll, scroll_id);
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x)),
        .child_gap = NAUI_DPI(UPH_AUTOMATION_LIST_SECTION_GAP)
    })
    {
        const Uph_ActionTrackRef parent = uph_action_track_ref(track);
        uph_automation_list_render_track_params(track, parent, targets, target_count, master ? NAUI_TR("master.track.title") : NAUI_TR("automation.menu.track"), accent);
        uph_automation_list_render_plugin_params(&track->instrument, parent, -1, 1, accent);
        for (uint32_t i = 0; i < naui_list_len(track->effects); i++)
            uph_automation_list_render_plugin_params(&track->effects[i].plugin, parent, (int32_t)i, i + 2, accent);
    }
    uph_ui_end_scroll_container(&container);

    uph_automation_list_footer();

    state->activate = false;
    state->last_mouse_x = naui_mouse_x();
    state->last_mouse_y = naui_mouse_y();
}
