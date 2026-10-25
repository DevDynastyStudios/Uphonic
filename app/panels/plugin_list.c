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
Uph_PluginListData;

static Uph_PluginListData uph_plugin_list_data = { 0 };

static void uph_plugin_list_prefetch_job(void *data, char *err_buf, size_t err_size)
{
    (void)err_buf;
    (void)err_size;
    Uph_PluginListLoad *load = data;
    uph_plugin_prefetch(load->path, &load->prefetch_cancel);
}

static void uph_plugin_list_cancel_load(void)
{
    Uph_PluginListLoad *load = &uph_plugin_list_data.load;
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
    const Naui_PanelID panel_id = naui_panel_current();
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_DOCK | NAUI_PANEL_FLAG_NO_UNDOCK);
    if (!uph_plugin_cache_read(&uph_plugin_list_data.plugin_infos))
        uph_plugin_scan_start(uph_plugin_list_data.plugin_infos);
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
    uph_plugin_list_cancel_load();
    uph_plugin_list_data.filter = (Naui_String){0};
}

static void uph_plugin_list_apply_load(const Naui_Path plugin_path)
{
    switch (uph_state.shared.plugin_list_target)
    {
        case UPH_PLUGIN_LIST_TRACK_INSTRUMENT:
        {
            Uph_Track *track = uph_state.shared.current_plugin_list_track;
            if (track->instrument.loaded)
                uph_action_track_unload_plugin(track, &track->instrument);

            track->instrument = uph_plugin_load(plugin_path, true);
            track->type = UPH_RESOURCE_PATTERN;
            break;
        }
        case UPH_PLUGIN_LIST_TRACK_EFFECT:
        {
            Uph_EffectPlugin effect = { .plugin = uph_plugin_load(plugin_path, true), .enabled = true };
            naui_list_push(uph_state.shared.current_plugin_list_track->effects, effect);
            break;
        }
        case UPH_PLUGIN_LIST_MASTER_EFFECT:
        {
            Uph_EffectPlugin effect = { .plugin = uph_plugin_load(plugin_path, true), .enabled = true };
            naui_list_push(uph_state.project.master_track.effects, effect);
            break;
        }
    }
    naui_panel_close(naui_panel_current());
}

// The plugin itself has to be created on this thread (CLAP and VST3 both require init and the GUI to be on the main thread),
// so the load only starts once the file is cached and the loading text has actually been drawn
static void uph_plugin_list_load(void)
{
    Uph_PluginListLoad *load = &uph_plugin_list_data.load;
    if (load->active || uph_plugin_list_data.current_plugin_index >= (int32_t)naui_list_len(uph_plugin_list_data.plugin_infos))
        return;

    const Uph_PluginInfo *info = &uph_plugin_list_data.plugin_infos[uph_plugin_list_data.current_plugin_index];
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
    Uph_PluginListLoad *load = &uph_plugin_list_data.load;
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

// Finished scans can drop entries, so the selection is found again by path instead of by index
static void uph_plugin_list_poll_scan(void)
{
    Uph_PluginListData *data = &uph_plugin_list_data;
    const bool had_selection = data->current_plugin_index < (int32_t)naui_list_len(data->plugin_infos);
    const Naui_Path selected = had_selection ? naui_path_normalize(data->plugin_infos[data->current_plugin_index].path) : (Naui_Path){ 0 };

    if (!uph_plugin_scan_poll(&data->plugin_infos))
        return;

    data->current_plugin_index = 0;
    for (uint32_t i = 0; had_selection && i < (uint32_t)naui_list_len(data->plugin_infos); i++)
    {
        if (strcmp(naui_path_normalize(data->plugin_infos[i].path).data, selected.data) == 0)
        {
            data->current_plugin_index = (int32_t)i;
            break;
        }
    }
}

static Naui_Color uph_plugin_list_muted_color(void)
{
    return naui_theme_color("naui_panel_text_color");
}

static void uph_plugin_list_target_bar(void)
{
    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    const Uph_PluginListTarget target = uph_state.shared.plugin_list_target;
    const Uph_Track *track = target == UPH_PLUGIN_LIST_MASTER_EFFECT ? NULL : uph_state.shared.current_plugin_list_track;
    const Naui_Color accent = uph_resources_track_color(track ? track->color_index : 0);

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
        leaf_text(target == UPH_PLUGIN_LIST_TRACK_INSTRUMENT ? NAUI_TR("plugin.menu.target.instrument") : NAUI_TR("plugin.menu.target.effect"), {
            .font_size = font_size,
            .color = uph_plugin_list_muted_color()
        });
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TARGET_DOT)), LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TARGET_DOT))},
            .color = accent,
            .rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
        });
        leaf_text(track ? track->name.data : NAUI_TR("master.track.title"), {
            .font_size = font_size,
            .color = naui_theme_color("uph_ui_text_color")
        });
    }
}

static void uph_plugin_list_status(void)
{
    static char text[256];
    text[0] = '\0';

    const Uph_PluginListLoad *load = &uph_plugin_list_data.load;
    const Uph_PluginScanProgress scan = uph_plugin_scan_progress();
    if (load->active)
        snprintf(text, sizeof(text), "%s  %s", NAUI_TR("plugin.menu.loading"), load->name.data);
    else if (scan.active)
        snprintf(text, sizeof(text), "%s  %u / %u", NAUI_TR("plugin.menu.scanning"), scan.scanned, scan.total);

    if (!text[0])
        return;

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2.0f), NAUI_DPI(padding.y))
    })
    {
        leaf_text(text, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = uph_plugin_list_muted_color()
        });
    }
}

static void uph_plugin_list_text_cell(const char *text, const Leaf_SizeAxis width, const Naui_Color color)
{
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    leaf({
        .size = {width, LEAF_SIZE_FIT},
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
        .clip_children = true
    })
    {
        leaf_text(text, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = color
        });
    }
}

static void uph_plugin_list_format_badge(const Uph_PluginFormat format)
{
    leaf({
        .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(6), NAUI_DPI(2)),
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
        .color = format == UPH_PLUGIN_VST3 ?
            naui_theme_color("uph_plugin_list_vst3_color") :
            naui_theme_color("uph_plugin_list_clap_color")
    })
    {
        leaf_text(format == UPH_PLUGIN_VST3 ? "VST3" : "CLAP", {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = naui_theme_color("uph_plugin_list_text_color")
        });
    }
}

static void uph_plugin_list_item(const Uph_PluginInfo *info, uint32_t item_index)
{
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    const Leaf_ID id = leaf_id_indexed("uph_plugin_list_item", item_index);
    const bool hovered = uph_ui_widget_hovered(id);
    const bool selected = uph_plugin_list_data.current_plugin_index == (int32_t)item_index;
    if (hovered)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
            uph_plugin_list_data.current_plugin_index = item_index;
        if (naui_mouse_double_clicked(NAUI_MOUSE_LEFT))
            uph_plugin_list_load();
        naui_set_cursor(NAUI_CURSOR_HAND);
    }

    Naui_Color color = { 0 };
    if (selected)
        color = uph_color_mix(naui_theme_color("uph_ui_frame_bg_color"), naui_theme_color("uph_ui_selection_color"), 0.3f);
    else if (hovered)
        color = naui_theme_color("uph_ui_frame_secondary_bg_color");

    leaf({
        .id = id,
        .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y)),
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        uph_plugin_list_text_cell(info->name.data, LEAF_SIZE_GROW, naui_theme_color("uph_ui_text_color"));
        uph_plugin_list_text_cell(info->vendor.data, LEAF_SIZE_GROW, uph_plugin_list_muted_color());
        uph_plugin_list_text_cell(info->type == UPH_PLUGIN_INSTRUMENT ? NAUI_TR("plugin.menu.instrument.short") : NAUI_TR("plugin.menu.effect.short"),
            LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_TYPE_COLUMN)), uph_plugin_list_muted_color());
        leaf({
            .size = {LEAF_SIZE_FIXED(NAUI_DPI(UPH_PLUGIN_LIST_FORMAT_COLUMN)), LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), 0),
            .clip_children = true
        })
        {
            uph_plugin_list_format_badge(info->format);
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
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Naui_Color muted_color = uph_plugin_list_muted_color();

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2.0f), NAUI_DPI(padding.y)),
        .border = {
            .width = 1,
            .color = naui_theme_color("uph_ui_frame_border"),
            .sides = LEAF_SIDE_BOTTOM
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
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x)),
        .child_gap = NAUI_DPI(UPH_PLUGIN_LIST_ROW_GAP)
    })
    {
        for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_plugin_list_data.plugin_infos); i++)
        {
            Uph_PluginInfo info = uph_plugin_list_data.plugin_infos[i];
            if (!uph_plugin_list_matches_filter(&info))
                continue;
            uph_plugin_list_item(&info, i);
            visible_count++;
        }

        if (visible_count == 0)
        {
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.y * 2.0f)),
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                leaf_text(NAUI_TR("plugin.menu.none.matching"), {
                    .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
                    .color = muted_color
                });
            }
        }
    }
    uph_ui_end_scroll_container(&scroll_container);
}

static void uph_plugin_list_property(const char *label, const char *value)
{
    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .direction = LEAF_DIRECTION_HORIZONTAL
    })
    {
        leaf({
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
            .clip_children = true
        }) leaf_text(label, { .font_size = font_size, .color = uph_plugin_list_muted_color() });
        if (value)
            leaf_text(value, { .font_size = font_size, .color = naui_theme_color("uph_ui_text_color") });
    }
}

static bool uph_plugin_list_load_button(const char *label, const Leaf_ID id)
{
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const bool hovered = uph_ui_widget_hovered(id);

    Naui_Color color = naui_theme_color("uph_ui_selection_color");
    if (hovered)
    {
        naui_set_cursor(NAUI_CURSOR_HAND);
        color = naui_mouse_down(NAUI_MOUSE_LEFT) ?
            uph_color_mix(color, naui_theme_color("uph_ui_frame_bg_color"), 0.25f) :
            naui_theme_color("uph_ui_slider_fill_color");
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x), NAUI_DPI(padding.y * 1.5f)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = color,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL)
    })
    {
        leaf_text(label, {
            .font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size")),
            .color = naui_theme_color("uph_plugin_list_text_color")
        });
    }

    return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph_plugin_list_current_menu(void)
{
    const int32_t font_size = NAUI_DPI(naui_theme_float("uph_ui_font_size"));
    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");
    const Uph_PluginInfo *info = &uph_plugin_list_data.plugin_infos[uph_plugin_list_data.current_plugin_index];

    leaf({
        .size = {LEAF_SIZE_PERCENT(0.28f), LEAF_SIZE_FULL},
        .padding = LEAF_PADDING_ALL(NAUI_DPI(padding.x * 2.0f)),
        .color = naui_theme_color("uph_effect_list_bg_color"),
        .border = {
            .width = 1,
            .color = naui_theme_color("uph_ui_frame_border"),
            .sides = LEAF_SIDE_LEFT
        },
        .child_gap = NAUI_DPI(padding.y * 2.0f),
        .clip_children = true
    })
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
            .child_gap = NAUI_DPI(4),
            .clip_children = true
        })
        {
            leaf_text(info->name.data, { .font_size = (int32_t)(font_size * 1.6f), .color = naui_theme_color("uph_ui_text_color") });
            leaf_text(info->vendor.data, { .font_size = font_size, .color = uph_plugin_list_muted_color() });
        }
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
            .padding = {.top = (int32_t)NAUI_DPI(padding.y * 2.0f)},
            .border = {
                .width = 1,
                .color = naui_theme_color("uph_ui_frame_border"),
                .sides = LEAF_SIDE_TOP
            },
            .child_gap = NAUI_DPI(padding.y)
        })
        {
            uph_plugin_list_property(NAUI_TR("plugin.menu.type"), info->type == UPH_PLUGIN_INSTRUMENT ? NAUI_TR("plugin.menu.instrument") : NAUI_TR("plugin.menu.effect"));
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                .direction = LEAF_DIRECTION_HORIZONTAL
            })
            {
                leaf({
                    .size = {LEAF_SIZE_GROW, LEAF_SIZE_FIT},
                    .clip_children = true
                }) leaf_text(NAUI_TR("plugin.menu.format"), { .font_size = font_size, .color = uph_plugin_list_muted_color() });
                uph_plugin_list_format_badge(info->format);
            }
        }
        leaf({ .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW} });
        if (uph_plugin_list_load_button(NAUI_TR("plugin.menu.load"), leaf_id("uph_plugin_list_load")))
            uph_plugin_list_load();
    }
}

void uph_plugin_list_on_update(void)
{
	naui_panel_set_title(naui_panel_current(), NAUI_TR("plugin.menu.title"));

    uph_plugin_list_poll_scan();

    if (uph_plugin_list_update_load())
        return;

    const Naui_Vec2 padding = naui_theme_vec2("uph_ui_frame_padding");

    uph_plugin_list_target_bar();

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(padding.x * 2.0f), NAUI_DPI(padding.y * 1.5f)),
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .child_gap = NAUI_DPI(padding.x)
    })
    {
        uph_ui_textfield(&uph_plugin_list_data.filter, leaf_id("uph_plugin_list_filter"), UPH_UI_TEXTFIELD_FLAG_ALWAYS_ACTIVE, "Search");
        if (uph_ui_text_button("Refresh", leaf_id("uph_plugin_list_refresh")))
            uph_plugin_scan_start(uph_plugin_list_data.plugin_infos);
    }

    uph_plugin_list_status();

    if (naui_list_len(uph_plugin_list_data.plugin_infos) == 0)
    {
        if (uph_plugin_scan_progress().active)
            return;

        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        }) leaf_text(NAUI_TR("plugin.menu.none.detected"), {
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(naui_theme_float("uph_ui_font_size"))),
            .color = {uph_plugin_list_muted_color()},
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