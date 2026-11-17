NAUI_PANEL(uph_plugin_list)

#define UPH_PLUGIN_LIST_LOAD_VISIBLE_FRAMES 3
#define UPH_PLUGIN_LIST_TYPE_COLUMN 64
#define UPH_PLUGIN_LIST_FORMAT_COLUMN 64
#define UPH_PLUGIN_LIST_ROW_GAP 2
#define UPH_PLUGIN_LIST_TARGET_DOT 8

typedef struct
{
    Naui_Path path;
    Naui_String name;
    Naui_JobHandle prefetch;
    atomic_bool prefetch_cancel;
    uint32_t frames;
    bool active;
}
Uph_PluginListLoad;

typedef struct
{
    Naui_List(Uph_PluginInfo) plugin_infos;

    Naui_String filter;

    int32_t current_plugin_index;

    Uph_PluginListLoad load;
}
Uph_PluginListState;

static Uph_PluginListState uph__plugin_list_state = { 0 };

static bool uph_plugin_list_ensure_selection(void);

static void uph_plugin_list_prefetch_job(void *data, char *err_buf, size_t err_size)
{
    (void)err_buf;
    (void)err_size;
    Uph_PluginListLoad *load = data;
    uph_plugin_prefetch(load->path, &load->prefetch_cancel);
}

static void uph_plugin_list_cancel_load(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    Uph_PluginListLoad *load = &state->load;
    if (!load->active)
        return;

    atomic_store(&load->prefetch_cancel, true);
    if (naui_job_is_valid(load->prefetch))
        naui_job_wait(load->prefetch);

    load->prefetch = NAUI_JOB_INVALID;
    load->active = false;
}

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
	if (naui_list_len(uph_state.settings.plugin.plugin_paths) == 0)
		return;
}

void uph_plugin_list_on_close(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    uph_plugin_list_cancel_load();
    state->filter = (Naui_String){0};
}

static void uph_plugin_list_apply_load(const Naui_Path plugin_path)
{
    Uph_Track *track = uph_state.shared.current_plugin_list_track;
    if (uph_state.shared.plugin_list_target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT)
    {
        if (track->instrument.loaded)
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

// Load plugins on the main thread only plz
static void uph_plugin_list_load(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    Uph_PluginListLoad *load = &state->load;
    if (load->active || !uph_plugin_list_ensure_selection())
        return;

    const Uph_PluginInfo *info = &state->plugin_infos[state->current_plugin_index];
    load->path = info->path;
    load->name = info->name;
    load->frames = 0;
    load->active = true;
    atomic_store(&load->prefetch_cancel, false);

    if (naui_job_submit(&load->prefetch, uph_plugin_list_prefetch_job, load) != NAUI_JOB_SUBMIT_OK)
        load->prefetch = NAUI_JOB_INVALID;
}

static bool uph_plugin_list_update_load(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    Uph_PluginListLoad *load = &state->load;
    if (!load->active)
        return false;

    load->frames++;
    const bool cached = !naui_job_is_valid(load->prefetch) || naui_job_is_done(load->prefetch);
    if (load->frames < UPH_PLUGIN_LIST_LOAD_VISIBLE_FRAMES || !cached)
        return false;

    if (naui_job_is_valid(load->prefetch))
        naui_job_release(load->prefetch);

    const Naui_Path path = load->path;
    load->prefetch = NAUI_JOB_INVALID;
    load->active = false;
    uph_plugin_list_apply_load(path);
    return true;
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

static void uph_plugin_list_target_bar(void)
{
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;

    const Uph_PluginListTarget target = uph_state.shared.plugin_list_target;
    const Uph_Track *track = uph_state.shared.current_plugin_list_track;
    const Naui_Color accent = uph_resources_track_color(track ? track->color_index : 0);

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
        leaf_text(target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT ? NAUI_TR("plugin.menu.target.instrument") : NAUI_TR("plugin.menu.target.effect"), {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(font_size)
        });
        leaf({
            .size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TARGET_DOT)), .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TARGET_DOT)) },
            .color = accent,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
        leaf_text(!track || track->type == UPH_RESOURCE_MASTER ? NAUI_TR("master.track.title") : track->name.data, {
            .color = uph_state.colors.ui_text,
            .font_size = LEAF_SIZE_FIXED(font_size)
        });
    }
}

static void uph_plugin_list_status(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    static char text[256];
    text[0] = '\0';

    const Uph_PluginListLoad *load = &state->load;
    const Uph_PluginScanProgress scan = uph_plugin_scan_progress();
    if (load->active)
        snprintf(text, sizeof(text), "%s  %s", NAUI_TR("plugin.menu.loading"), load->name.data);
    else if (scan.active)
        snprintf(text, sizeof(text), "%s  %u / %u", NAUI_TR("plugin.menu.scanning"), scan.scanned, scan.total);

    if (!text[0])
        return;

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2), NAUI_DPI(padding.y))
    })
    {
        leaf_text(text, {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
        });
    }
}

static void uph_plugin_list_text_cell(const char *text, const Leaf_SizeAxis width, const Naui_Color color)
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
            .color = color,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
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
        .padding = LEAF_PADDING_AXES(NAUI_DPI(6), NAUI_DPI(2)),
        .color = colors[format],
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL)
    })
    {
        leaf_text(labels[format], {
            .color = uph_state.colors.plugin_list_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
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
        .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        uph_plugin_list_text_cell(info->name.data, LEAF_SIZE_GROW, uph_state.colors.ui_text);
        uph_plugin_list_text_cell(info->vendor.data, LEAF_SIZE_GROW, uph_state.colors.panel_text);
        uph_plugin_list_text_cell(info->type == UPH_PLUGIN_INSTRUMENT ? NAUI_TR("plugin.menu.instrument.short") : NAUI_TR("plugin.menu.effect.short"),
            LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TYPE_COLUMN)), uph_state.colors.panel_text);
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

static bool uph_plugin_list_matches_target(const Uph_PluginInfo *info)
{
    const bool want_instrument = uph_state.shared.plugin_list_target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT;
    return (info->type == UPH_PLUGIN_INSTRUMENT) == want_instrument;
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

static bool uph_plugin_list_matches_filter(const Uph_PluginInfo *info)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    if (!uph_plugin_list_matches_target(info))
        return false;

    if (state->filter.length == 0)
        return true;

    return naui_string_contains(info->name, state->filter, false) || naui_string_contains(info->vendor, state->filter, false);
}

static void uph_plugin_list_main_menu(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const Naui_Color muted_color = uph_state.colors.panel_text;

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
        uph_plugin_list_text_cell(NAUI_TR("plugin.menu.name"), LEAF_SIZE_GROW, muted_color);
        uph_plugin_list_text_cell(NAUI_TR("plugin.menu.vendor"), LEAF_SIZE_GROW, muted_color);
        uph_plugin_list_text_cell(NAUI_TR("plugin.menu.type"), LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TYPE_COLUMN)), muted_color);
        uph_plugin_list_text_cell(NAUI_TR("plugin.menu.format"), LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_FORMAT_COLUMN)), muted_color);
    }

    uint32_t visible_count = 0;
    static float scroll = 0.0;
    Uph_UIScrollContainer scroll_container = uph_ui_begin_scroll_container(
        UPH_UI_SCROLL_DIRECTION_VERTICAL,
        &scroll,
        leaf_id("uph_plugin_list_scrollbar")
    );
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x)),
        .child_gap = NAUI_DPI(UPH_PLUGIN_LIST_ROW_GAP)
    })
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(state->plugin_infos); i++)
        {
            Uph_PluginInfo info = state->plugin_infos[i];
            if (!uph_plugin_list_matches_filter(&info))
                continue;
            uph_plugin_list_item(&info, i);
            visible_count++;
        }

        if (visible_count == 0)
        {
            leaf({
                .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
                .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.y * 2)),
                .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
            })
            {
                leaf_text(NAUI_TR("plugin.menu.none.matching"), {
                    .color = muted_color,
                    .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
                });
            }
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
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
            .clip_children = true
        }) leaf_text(label, { .color = uph_state.colors.panel_text, .font_size = LEAF_SIZE_FIXED(font_size) });
        if (value)
            leaf_text(value, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(font_size) });
    }
}

static bool uph_plugin_list_load_button(const char *label, const Leaf_ID id)
{
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const bool hovered = uph_ui_widget_hovered(id);

    Naui_Color color = uph_state.colors.ui_selection;
    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        color = naui_mouse_down(NAUI_MOUSE_LEFT) ?
            uph_color_mix(color, uph_state.colors.ui_frame_bg, 0.25f) :
            uph_state.colors.ui_slider_fill;
    }

    leaf({
        .id = id,
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 1.5f)),
        .color = color,
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL)
    })
    {
        leaf_text(label, {
            .color = uph_state.colors.plugin_list_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
        });
    }

    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph_plugin_list_current_menu(void)
{
    Uph_PluginListState *state = &uph__plugin_list_state;
    const int32_t font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;
    const Uph_PluginInfo *info = &state->plugin_infos[state->current_plugin_index];

    leaf({
        .size = { .width = LEAF_SIZE_PERCENT(0.28f), .height = LEAF_SIZE_FULL },
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
            .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
            .child_gap = NAUI_DPI(4),
            .clip_children = true
        })
        {
            leaf_text(info->name.data, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED((int32_t)(font_size * 1.6f)) });
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
            .child_gap = NAUI_DPI(padding.y)
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
        }
        leaf({ .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW } });
        if (uph_plugin_list_load_button(NAUI_TR("plugin.menu.load"), leaf_id("uph_plugin_list_load")))
            uph_plugin_list_load();
    }
}

void uph_plugin_list_on_update(void)
{
	Uph_PluginListState *state = &uph__plugin_list_state;
    uph_plugin_list_poll_scan();

    if (uph_plugin_list_update_load())
        return;

    const Naui_Vec2 padding = UPH_UI_FRAME_PADDING;

    uph_plugin_list_target_bar();

    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2), NAUI_DPI(padding.y * 1.5f)),
        .child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
        .child_gap = NAUI_DPI(padding.x),
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        uph_ui_textfield(&state->filter, leaf_id("uph_plugin_list_filter"), UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE | UPH_UI_TEXTFIELD_FLAG_SEARCH, "Search");
        if (uph_ui_text_button("Refresh", leaf_id("uph_plugin_list_refresh")))
            uph_plugin_scan_start(state->plugin_infos);
    }

    uph_plugin_list_status();

    if (naui_list_len(state->plugin_infos) == 0)
    {
        if (uph_plugin_scan_progress().active)
            return;

        leaf({
            .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
            .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER }
        }) leaf_text(NAUI_TR("plugin.menu.none.detected"), {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE)),
            .wrap_mode = LEAF_TEXT_WRAP_MODE_WORD
        });
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
}