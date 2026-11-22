NAUI_PANEL(uph_plugin_list)

#define UPH_PLUGIN_LIST_FORMAT_COLUMN 64
#define UPH_PLUGIN_LIST_ROW_GAP 2
#define UPH_PLUGIN_LIST_ROW_HEIGHT 26
#define UPH_PLUGIN_LIST_TARGET_DOT 8
#define UPH_PLUGIN_LIST_ICON_SIZE 14
#define UPH_PLUGIN_LIST_TILE_SIZE 48
#define UPH_PLUGIN_LIST_TILE_ICON_SIZE 26
#define UPH_PLUGIN_LIST_PROGRESS_HEIGHT 3

typedef struct
{
    Naui_List(Uph_PluginInfo) plugin_infos;

    Naui_String filter;
    Naui_String last_filter;

    int32_t current_plugin_index;
    float scroll;
    bool scroll_to_selection;
}
Uph_PluginListState;

static Uph_PluginListState uph__plugin_list_state = { 0 };

static bool uph_plugin_list_ensure_selection(void);

void uph_plugin_list_on_attach(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const Naui_PanelID panel_id = naui_panel_current();
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_UNDOCK);
    if (!uph_plugin_cache_read(&state->plugin_infos))
        uph_plugin_scan_start(state->plugin_infos);
}

void uph_plugin_list_on_detach(void)
{

}

void uph_plugin_list_on_open(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    state->scroll_to_selection = true;
}

void uph_plugin_list_on_close(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    state->filter = (Naui_String){0};
    state->last_filter = (Naui_String){0};
}

static void uph_plugin_list_load(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    if (!uph_plugin_list_ensure_selection())
        return;

    const Naui_Path plugin_path = state->plugin_infos[state->current_plugin_index].path;
    Uph_Track *track = uph_state.shared.current_plugin_list_track;
    if (uph_state.shared.plugin_list_target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT)
    {
        uph_action_track_unload_plugin(track, &track->instrument);
        track->instrument = uph_plugin_load(plugin_path, true);
        track->type = UPH_RESOURCE_PATTERN;
    }
    else
    {
        Uph_EffectPlugin effect = { .plugin = uph_plugin_load(plugin_path, true), .enabled = true };
        naui_list_push(track->effects, effect);
    }
    naui_panel_close(naui_panel_current());
}

static void uph_plugin_list_poll_scan(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const bool had_selection = state->current_plugin_index < (int32_t)naui_list_len(state->plugin_infos);
    const Naui_Path selected = had_selection ? naui_path_normalize(state->plugin_infos[state->current_plugin_index].path) : (Naui_Path){ 0 };

    if (!uph_plugin_scan_poll(&state->plugin_infos))
        return;

    state->current_plugin_index = 0;
    for (uint32_t i = 0; had_selection && i < (uint32_t)naui_list_len(state->plugin_infos); i++)
    {
        if (strcmp(naui_path_normalize(state->plugin_infos[i].path).data, selected.data) == 0)
        {
            state->current_plugin_index = (int32_t)i;
            break;
        }
    }
}

static bool uph_plugin_list_matches_target(const Uph_PluginInfo *info)
{
    const bool want_instrument = uph_state.shared.plugin_list_target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT;
    return (info->type == UPH_PLUGIN_INSTRUMENT) == want_instrument;
}

static bool uph_plugin_list_matches_filter(const Uph_PluginInfo *info)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    if (!uph_plugin_list_matches_target(info))
        return false;

    if (state->filter.length == 0)
        return true;

    return naui_string_contains(info->name, state->filter, false) || naui_string_contains(info->vendor, state->filter, false);
}

static bool uph_plugin_list_ensure_selection(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const int32_t count = (int32_t)naui_list_len(state->plugin_infos);
    if (state->current_plugin_index >= 0 && state->current_plugin_index < count && uph_plugin_list_matches_target(&state->plugin_infos[state->current_plugin_index]))
        return true;

    for (int32_t i = 0; i < count; i++)
    {
        if (uph_plugin_list_matches_target(&state->plugin_infos[i]))
        {
            state->current_plugin_index = i;
            return true;
        }
    }
    return false;
}

static bool uph_plugin_list_selection_visible(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const int32_t count = (int32_t)naui_list_len(state->plugin_infos);
    return state->current_plugin_index >= 0 && state->current_plugin_index < count
        && uph_plugin_list_matches_filter(&state->plugin_infos[state->current_plugin_index]);
}

static void uph_plugin_list_step_selection(int32_t direction)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const int32_t count = (int32_t)naui_list_len(state->plugin_infos);
    if (count == 0)
        return;

    int32_t index = uph_plugin_list_selection_visible() ? state->current_plugin_index : (direction > 0 ? -1 : 0);
    for (int32_t step = 0; step < count; step++)
    {
        index = (index + direction + count) % count;
        if (uph_plugin_list_matches_filter(&state->plugin_infos[index]))
        {
            state->current_plugin_index = index;
            state->scroll_to_selection = true;
            return;
        }
    }
}

static uint32_t uph_plugin_list_target_count(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    uint32_t count = 0;
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->plugin_infos); i++)
        count += uph_plugin_list_matches_target(&state->plugin_infos[i]);
    return count;
}

static Naui_Color uph_plugin_list_accent(void)
{
    const Uph_Track *track = uph_state.shared.current_plugin_list_track;
    return uph_resources_track_color(track ? track->color_index : 0);
}

static const Naui_Image *uph_plugin_list_type_icon(Uph_PluginType type)
{
    return type == UPH_PLUGIN_INSTRUMENT ? uph_state.icons.piano : uph_state.icons.wave;
}

static void uph_plugin_list_handle_keys(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;

    if (!naui_strings_equal(state->filter, state->last_filter, true))
    {
        state->last_filter = state->filter;
        state->scroll = 0.0f;
        if (!uph_plugin_list_selection_visible())
            uph_plugin_list_step_selection(1);
        state->scroll_to_selection = true;
    }

    if (naui_key_pressed(NAUI_KEY_ESCAPE))
    {
        naui_panel_close(naui_panel_current());
        return;
    }

    if (naui_key_pressed_repeat(NAUI_KEY_DOWN))
        uph_plugin_list_step_selection(1);
    if (naui_key_pressed_repeat(NAUI_KEY_UP))
        uph_plugin_list_step_selection(-1);
    if (naui_key_pressed(NAUI_KEY_ENTER) && uph_plugin_list_selection_visible())
        uph_plugin_list_load();
}

static void uph_plugin_list_target_bar(void)
{
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;

    const Uph_PluginListTarget target = uph_state.shared.plugin_list_target;
    const Uph_Track *track = uph_state.shared.current_plugin_list_track;
    const Naui_Color accent = uph_plugin_list_accent();
    const int32_t icon_size = NAUI_DPI(UPH_PLUGIN_LIST_ICON_SIZE);

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
            .image = (void*)uph_plugin_list_type_icon(target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT ? UPH_PLUGIN_INSTRUMENT : UPH_PLUGIN_EFFECT),
            .size = { .width = LEAF_SIZE_FIXED(icon_size), .height = LEAF_SIZE_FIXED(icon_size) },
            .color = uph_state.colors.panel_text
        });
        leaf_text(target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT ? NAUI_TR("plugin.menu.target.instrument") : NAUI_TR("plugin.menu.target.effect"), {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(font_size)
        });
        leaf({
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TARGET_DOT)), .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TARGET_DOT)) },
            .color = accent,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .clip_children = true
        }) leaf_text(!track || track->type == UPH_RESOURCE_MASTER ? NAUI_TR("master.track.title") : track->name.data, {
            .color = uph_state.colors.ui_text,
            .font_size = LEAF_SIZE_FIXED(font_size)
        });
        uph_ui_count_badge(uph_plugin_list_target_count());
    }
}

static void uph_plugin_list_status(void)
{
    const Uph_PluginScanProgress scan = uph_plugin_scan_progress();
    if (!scan.active)
        return;

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const float progress = scan.total > 0 ? (float)scan.scanned / (float)scan.total : 0.0f;

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = { .left = (int32_t)NAUI_DPI(padding.x * 2), .right = (int32_t)NAUI_DPI(padding.x * 2), .bottom = (int32_t)NAUI_DPI(padding.y) },
        .child_gap = NAUI_DPI(4)
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
            .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
            .direction = LEAF_DIRECTION_HORIZONTAL
        })
        {
            leaf({
                .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
                .clip_children = true
            }) leaf_text(NAUI_TR("plugin.menu.scanning"), { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
            leaf_text(naui_string_format("%u / %u", scan.scanned, scan.total).data, { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        }
        leaf({
            .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_PROGRESS_HEIGHT)) },
            .color = uph_state.colors.ui_frame_bg,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL),
            .clip_children = true
        })
        {
            leaf({
                .size = { .width = LEAF_SIZE_PERCENT(NAUI_CLAMP(progress, 0.0f, 1.0f)), .height = LEAF_SIZE_FULL },
                .color = uph_state.colors.ui_selection,
                .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
            });
        }
    }
}

static void uph_plugin_list_text_cell(const char *text, const Leaf_SizeAxis width, const Naui_Color color)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    leaf({
        .size = { .width = width, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .clip_children = true
    })
    {
        uph_ui_highlighted_text(text, state->filter, color, uph_state.colors.panel_tab_accent);
    }
}

static void uph_plugin_list_header_cell(const char *text, const Leaf_SizeAxis width)
{
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    leaf({
        .size = { .width = width, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .clip_children = true
    })
    {
        leaf_text(text, {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE - 2))
        });
    }
}

static void uph_plugin_list_format_badge(const Uph_PluginFormat format)
{
    static const char *const labels[UPH_PLUGIN_FORMAT_COUNT] = {
        [UPH_PLUGIN_CLAP] = "CLAP",
        [UPH_PLUGIN_VST3] = "VST3",
        [UPH_PLUGIN_VST2] = "VST2"
    };
    const Naui_Color colors[UPH_PLUGIN_FORMAT_COUNT] = {
        [UPH_PLUGIN_CLAP] = uph_state.colors.plugin_list_clap,
        [UPH_PLUGIN_VST3] = uph_state.colors.plugin_list_vst3,
        [UPH_PLUGIN_VST2] = uph_state.colors.plugin_list_vst2
    };

    leaf({
        .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(6), NAUI_DPI(1)),
        .color = colors[format],
        .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
    })
    {
        leaf_text(labels[format], {
            .color = uph_state.colors.plugin_list_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE - 2))
        });
    }
}

static void uph_plugin_list_item(const Uph_PluginInfo *info, uint32_t item_index)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const Leaf_ID id = leaf_id_indexed("uph_plugin_list_item", item_index);
    const bool hovered = uph_ui_widget_hovered(id);
    const bool selected = state->current_plugin_index == (int32_t)item_index;
    if (hovered)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
            state->current_plugin_index = item_index;
        if (naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
            uph_plugin_list_load();
        naui_set_cursor(NAUI_CURSOR_HAND);
    }

    Naui_Color color = { 0 };
    if (selected)
        color = uph_color_mix(uph_state.colors.ui_frame_bg, uph_state.colors.ui_selection, 0.3f);
    else if (hovered)
        color = uph_state.colors.ui_frame_secondary_bg;

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_ROW_HEIGHT)) },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .border = {
            .width = selected ? (int32_t)NAUI_DPI(2) : 0,
            .sides = LEAF_SIDE_LEFT,
            .color = uph_plugin_list_accent()
        },
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        uph_plugin_list_text_cell(info->name.data, LEAF_SIZE_GROW, uph_state.colors.ui_text);
        uph_plugin_list_text_cell(info->vendor.data, LEAF_SIZE_GROW, uph_state.colors.panel_text);
        leaf({
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_FORMAT_COLUMN)), .height = LEAF_SIZE_FIT },
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
            .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
            .clip_children = true
        })
        {
            uph_plugin_list_format_badge(info->format);
        }
    }
}

static void uph_plugin_list_main_menu(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2), NAUI_DPI(padding.y)),
        .border = {
            .width = 1,
            .sides = LEAF_SIDE_BOTTOM,
            .color = uph_state.colors.ui_frame_border
        },
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        uph_plugin_list_header_cell(NAUI_TR("plugin.menu.name"), LEAF_SIZE_GROW);
        uph_plugin_list_header_cell(NAUI_TR("plugin.menu.vendor"), LEAF_SIZE_GROW);
        uph_plugin_list_header_cell(NAUI_TR("plugin.menu.format"), LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_FORMAT_COLUMN)));
    }

    uint32_t visible_count = 0;
    for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->plugin_infos); i++)
        visible_count += uph_plugin_list_matches_filter(&state->plugin_infos[i]);

    if (visible_count == 0)
    {
        uph_ui_empty_state(uph_state.icons.zoom, NAUI_TR("plugin.menu.none.matching"));
        return;
    }

    const Leaf_ID scroll_id = leaf_id("uph_plugin_list_scrollbar");
    if (state->scroll_to_selection)
    {
        state->scroll_to_selection = false;
        uph_ui_scroll_to_element(scroll_id, leaf_id_indexed("uph_plugin_list_item", (uint32_t)state->current_plugin_index), &state->scroll, 0);
    }

    Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(UPH_UI_SCROLL_DIRECTION_VERTICAL, &state->scroll, scroll_id);
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x)),
        .child_gap = NAUI_DPI(UPH_PLUGIN_LIST_ROW_GAP)
    })
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->plugin_infos); i++)
        {
            Uph_PluginInfo info = state->plugin_infos[i];
            if (uph_plugin_list_matches_filter(&info))
                uph_plugin_list_item(&info, i);
        }
    }
    uph_ui_end_scroll_container(&scroll_container);
}

static void uph_plugin_list_property(const char *label, const char *value)
{
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(UPH_UI_FRAME_PADDING.x),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf_text(label, { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT } });
        if (value)
        {
            leaf({
                .size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
                .clip_children = true
            }) leaf_text(value, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        }
    }
}

static const char *uph_plugin_list_file_name(const Naui_Path *path)
{
    const char *name = path->data;
    for (const char *c = path->data; *c; c++)
    {
        if (*c == '/' || *c == '\\')
            name = c + 1;
    }
    return name;
}

static void uph_plugin_list_current_menu(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const Uph_PluginInfo *info = &state->plugin_infos[state->current_plugin_index];
    const Naui_Color accent = uph_plugin_list_accent();
    const int32_t tile_size = NAUI_DPI(UPH_PLUGIN_LIST_TILE_SIZE);
    const int32_t tile_icon_size = NAUI_DPI(UPH_PLUGIN_LIST_TILE_ICON_SIZE);

    leaf({
        .size = { .width = LEAF_SIZE_PERCENT(0.3f), .height = LEAF_SIZE_FULL },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x * 2)),
        .border = {
            .width = 1,
            .sides = LEAF_SIDE_LEFT,
            .color = uph_state.colors.ui_frame_border
        },
        .color = uph_state.colors.plugin_list_bg,
        .child_gap = NAUI_DPI(padding.y * 2),
        .clip_children = true
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_FIXED(tile_size), .height = LEAF_SIZE_FIXED(tile_size) },
            .color = uph_color_mix(uph_state.colors.ui_frame_bg, accent, 0.25f),
            .border = {
                .width = 1,
                .sides = LEAF_SIDE_ALL,
                .color = uph_color_mix(uph_state.colors.ui_frame_border, accent, 0.4f)
            },
            .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING * 2), NAUI_CORNER_ALL)
        })
        {
            leaf({
                .image = (void*)uph_plugin_list_type_icon(info->type),
                .size = { .width = LEAF_SIZE_FIXED(tile_icon_size), .height = LEAF_SIZE_FIXED(tile_icon_size) },
                .color = accent
            });
        }
        leaf({
            .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
            .child_gap = NAUI_DPI(4),
            .clip_children = true
        })
        {
            leaf_text(info->name.data, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED((int32_t)(font_size * 1.4f)), .wrap_mode = LEAF_TEXT_WRAP_MODE_WORD });
            leaf_text(info->vendor.data, { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        }
        leaf({
            .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
            .padding = { .top = (int32_t)NAUI_DPI(padding.y * 2) },
            .border = {
                .width = 1,
                .sides = LEAF_SIDE_TOP,
                .color = uph_state.colors.ui_frame_border
            },
            .child_gap = NAUI_DPI(padding.y * 1.5f)
        })
        {
            uph_plugin_list_property(NAUI_TR("plugin.menu.type"), info->type == UPH_PLUGIN_INSTRUMENT ? NAUI_TR("plugin.menu.instrument") : NAUI_TR("plugin.menu.effect"));
            leaf({
                .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
                .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
                .direction = LEAF_DIRECTION_HORIZONTAL
            })
            {
                leaf({
                    .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
                    .clip_children = true
                }) leaf_text(NAUI_TR("plugin.menu.format"), { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
                uph_plugin_list_format_badge(info->format);
            }
            uph_plugin_list_property(NAUI_TR("plugin.menu.file"), uph_plugin_list_file_name(&info->path));
        }
        leaf({ .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW } });
        if (uph_ui_large_primary_button_full(NAUI_TR("plugin.menu.load"), leaf_id("uph_plugin_list_load")))
            uph_plugin_list_load();
    }
}

static void uph_plugin_list_footer(void)
{
    const Uph_UIKeyHint hints[] = {
        { NAUI_TR("list.hint.navigate_keys"), NAUI_TR("list.hint.navigate") },
        { "Enter", NAUI_TR("plugin.menu.load") },
        { "Esc", NAUI_TR("list.hint.close") }
    };
    uph_ui_key_hint_bar(hints, sizeof(hints) / sizeof(hints[0]));
}

void uph_plugin_list_on_update(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    uph_plugin_list_poll_scan();

    uph_plugin_list_handle_keys();
    if (naui_panel_closed(naui_panel_current()))
        return;

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const int32_t icon_size = NAUI_DPI(UPH_PLUGIN_LIST_ICON_SIZE);

    uph_plugin_list_target_bar();

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2), NAUI_DPI(padding.y * 1.5f)),
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(padding.x),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        uph_ui_textfield(&state->filter, leaf_id("uph_plugin_list_filter"), UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE | UPH_UI_TEXTFIELD_FLAG_SEARCH, NAUI_TR("plugin.menu.search"));
        if (uph_ui_image_button(uph_state.icons.reload, leaf_id("uph_plugin_list_refresh"), (Naui_Vec2){ icon_size, icon_size }, uph_state.colors.ui_text))
            uph_plugin_scan_start(state->plugin_infos);
    }

    uph_plugin_list_status();

    if (naui_list_len(state->plugin_infos) == 0)
    {
        if (!uph_plugin_scan_progress().active)
            uph_ui_empty_state(uph_state.icons.piano, NAUI_TR("plugin.menu.none.detected"));
        return;
    }

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
        .border = {
            .width = 1,
            .sides = LEAF_SIDE_TOP,
            .color = uph_state.colors.ui_frame_border
        },
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW }
        })
        {
            uph_plugin_list_main_menu();
        }

        if (uph_plugin_list_ensure_selection())
            uph_plugin_list_current_menu();
    }

    uph_plugin_list_footer();
}
