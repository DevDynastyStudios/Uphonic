NAUI_PANEL(uph_settings)

#define UPH_SETTINGS_MAX_OPTIONS		64
#define UPH_SETTINGS_MAX_LANGUAGES		32

#define UPH_SETTINGS_THEME_NAME_MAX		48

#define UPH_SETTINGS_SIDEBAR_WIDTH		150.0f
#define UPH_SETTINGS_LABEL_WIDTH		200.0f
#define UPH_SETTINGS_CONTROL_WIDTH		320.0f
#define UPH_SETTINGS_ROW_HEIGHT			28.0f

#define UPH_SETTINGS_UI_SCALE_MIN		0.5f
#define UPH_SETTINGS_UI_SCALE_MAX		2.0f
#define UPH_SETTINGS_AUTOSAVE_MAX		120
#define UPH_SETTINGS_UNDO_MIN			1
#define UPH_SETTINGS_UNDO_MAX			1000
#define UPH_SETTINGS_VU_HOLD_MAX		10.0f
#define UPH_SETTINGS_VU_DECAY_MIN		0.05f
#define UPH_SETTINGS_VU_DECAY_MAX		5.0f
#define UPH_SETTINGS_SENSITIVITY_MIN	0.05f
#define UPH_SETTINGS_SENSITIVITY_MAX	5.0f
#define UPH_SETTINGS_COUNT_IN_MAX		8
#define UPH_SETTINGS_VELOCITY_MIN		1
#define UPH_SETTINGS_VELOCITY_MAX		127
#define UPH_SETTINGS_CHANNEL_MIN		1
#define UPH_SETTINGS_CHANNEL_MAX		16

#define UPH_SETTINGS_NO_ID ((Leaf_ID){ 0 })
#define UPH_SETTINGS_COUNT(array) ((uint32_t)(sizeof(array) / sizeof((array)[0])))

#define UPH_SETTINGS_ROW(label, label_id) \
	for (int uph_settings_row_once = (uph_settings_row_begin((label), (label_id)), 1); \
		 uph_settings_row_once; \
		 uph_settings_row_once = (uph_settings_row_end(), 0))

typedef enum
{
	UPH_SETTINGS_PAGE_GENERAL,
	UPH_SETTINGS_PAGE_AUDIO,
	UPH_SETTINGS_PAGE_INTERFACE,
	UPH_SETTINGS_PAGE_MIDI,
	UPH_SETTINGS_PAGE_PLUGINS,
	UPH_SETTINGS_PAGE_COUNT
}
Uph_SettingsPage;

typedef struct
{
	Naui_String name[UPH_SETTINGS_MAX_OPTIONS];
	uint32_t count;
}
Uph_SettingsNameList;

typedef struct
{
	cmidi_device_t device[CMIDI_MAX_DEVICES];
	uint32_t count;
}
Uph_SettingsMidiList;

typedef struct
{
	Naui_String label;
	Naui_String language;
	Naui_String region;
}
Uph_SettingsLanguage;

typedef struct
{
	Uph_SettingsPage page;
	bool lists_ready;
	bool was_opened;

	Uph_SettingsNameList themes;
	Uph_SettingsNameList audio_outputs;
	Uph_SettingsNameList audio_inputs;
	Uph_SettingsMidiList midi_inputs;
	Uph_SettingsMidiList midi_outputs;

	Uph_SettingsLanguage languages[UPH_SETTINGS_MAX_LANGUAGES];
	uint32_t language_count;
	bool language_changed;

	Naui_String language_unlisted;
	Naui_String midi_unlisted[2];
	Naui_String new_plugin_path;
	Naui_String rejected_plugin_path;
}
Uph_SettingsData;

static Uph_SettingsData uph_settings_data;

static ma_context uph_settings_audio_context;
static bool uph_settings_audio_context_ready;

static const uint32_t uph_settings_sample_rates[] = { 44100, 48000, 88200, 96000, 176400, 192000 };
static const char *const uph_settings_sample_rate_labels[] = { "44.1 kHz", "48 kHz", "88.2 kHz", "96 kHz", "176.4 kHz", "192 kHz" };

static const uint32_t uph_settings_buffer_sizes[] = { 64, 128, 256, 512, 1024, 2048 };
static const char *const uph_settings_buffer_size_labels[] = { "64 samples", "128 samples", "256 samples", "512 samples", "1024 samples", "2048 samples" };

static const uint32_t uph_settings_channel_counts[] = { 1, 2 };
static const char *const uph_settings_channel_labels[] = { "1 (Mono)", "2 (Stereo)" };

static const uint32_t uph_settings_snap_values[] = { UPH_SNAP_BEAT, UPH_SNAP_HALF, UPH_SNAP_QUARTER, UPH_SNAP_EIGHTH, UPH_SNAP_SIXTEENTH };
static const char *const uph_settings_snap_labels[] = { "Beat", "1/2", "1/4", "1/8", "1/16" };

void uph_settings_set_defaults(void)
{
	Uph_Settings *settings = &uph_state.settings;

	settings->general.theme = naui_string_from_cstr("Dark");
	settings->general.language_code = naui_string_from_cstr("en");
	settings->general.region_code = naui_string_from_cstr("US");
	settings->general.ui_scale = 1.0f;
	settings->general.autosave_timer = 5;
	settings->general.undo_history_limit = 100;
	settings->general.confirm_on_exit = true;
	settings->general.confirm_on_delete = true;
	settings->general.copy_resources = false;

	settings->audio.output_device = (Naui_String){ 0 };
	settings->audio.input_device = (Naui_String){ 0 };
	settings->audio.sample_rate = 48000;
	settings->audio.buffer_size = 512;
	settings->audio.channels = 2;
	settings->audio.monitoring_gain = 1.0f;
	settings->audio.vu_meter_hold_time = 1.0f;
	settings->audio.vu_meter_decay_time = 0.3f;
	settings->audio.exclusive_mode = false;
	settings->audio.software_monitoring = false;
	settings->audio.auto_restart_on_device_change = true;
	settings->audio.underrun_protection = true;

	settings->ui.scroll_sensitivity = (Naui_Vec2){ 1.0f, 1.0f };
	settings->ui.zoom_sensitivity = (Naui_Vec2){ 1.0f, 1.0f };
	settings->ui.default_snap = UPH_SNAP_QUARTER;
	settings->ui.playhead_lock_position = 0.5f;
	settings->ui.follow_playhead = true;
	settings->ui.show_tooltips = true;

	settings->midi.record_quantize = false;
	settings->midi.record_quantize_grid = UPH_SNAP_SIXTEENTH;
	settings->midi.count_in_bars = 0;
	settings->midi.grid_division = UPH_SNAP_QUARTER;
	settings->midi.default_velocity = 100;
	settings->midi.default_channel = 1;
	settings->midi.filter_by_channel = false;
	settings->midi.midi_thru_enabled = true;
	settings->midi.record_over_dub = false;

	settings->plugin.sandbox_plugins = false;
}

static uint32_t uph_settings_nearest(const uint32_t value, const uint32_t *values, const uint32_t count)
{
	uint32_t nearest = values[0];
	uint32_t best_distance = UINT32_MAX;

	for (uint32_t i = 0; i < count; i++)
	{
		const uint32_t distance = values[i] > value ? values[i] - value : value - values[i];
		if (distance < best_distance)
		{
			best_distance = distance;
			nearest = values[i];
		}
	}

	return nearest;
}

void uph_settings_sanitize(void)
{
	Uph_Settings *settings = &uph_state.settings;

	if (settings->general.theme.length == 0)
		settings->general.theme = naui_string_from_cstr("Dark");

	if (settings->general.language_code.length == 0 || settings->general.region_code.length == 0)
	{
		settings->general.language_code = naui_string_from_cstr("en");
		settings->general.region_code = naui_string_from_cstr("US");
	}

	settings->general.ui_scale = NAUI_CLAMP(settings->general.ui_scale, UPH_SETTINGS_UI_SCALE_MIN, UPH_SETTINGS_UI_SCALE_MAX);
	settings->general.autosave_timer = NAUI_CLAMP(settings->general.autosave_timer, 0, UPH_SETTINGS_AUTOSAVE_MAX);
	settings->general.undo_history_limit = NAUI_CLAMP(settings->general.undo_history_limit, (uint32_t)UPH_SETTINGS_UNDO_MIN, (uint32_t)UPH_SETTINGS_UNDO_MAX);

	settings->audio.sample_rate = uph_settings_nearest(settings->audio.sample_rate, uph_settings_sample_rates, UPH_SETTINGS_COUNT(uph_settings_sample_rates));
	settings->audio.buffer_size = uph_settings_nearest(settings->audio.buffer_size, uph_settings_buffer_sizes, UPH_SETTINGS_COUNT(uph_settings_buffer_sizes));
	settings->audio.channels = uph_settings_nearest(settings->audio.channels, uph_settings_channel_counts, UPH_SETTINGS_COUNT(uph_settings_channel_counts));
	settings->audio.monitoring_gain = NAUI_CLAMP(settings->audio.monitoring_gain, 0.0f, 1.0f);
	settings->audio.vu_meter_hold_time = NAUI_CLAMP(settings->audio.vu_meter_hold_time, 0.0f, UPH_SETTINGS_VU_HOLD_MAX);
	settings->audio.vu_meter_decay_time = NAUI_CLAMP(settings->audio.vu_meter_decay_time, UPH_SETTINGS_VU_DECAY_MIN, UPH_SETTINGS_VU_DECAY_MAX);

	settings->ui.scroll_sensitivity.x = NAUI_CLAMP(settings->ui.scroll_sensitivity.x, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX);
	settings->ui.scroll_sensitivity.y = NAUI_CLAMP(settings->ui.scroll_sensitivity.y, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX);
	settings->ui.zoom_sensitivity.x = NAUI_CLAMP(settings->ui.zoom_sensitivity.x, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX);
	settings->ui.zoom_sensitivity.y = NAUI_CLAMP(settings->ui.zoom_sensitivity.y, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX);
	settings->ui.default_snap = uph_settings_nearest(settings->ui.default_snap, uph_settings_snap_values, UPH_SETTINGS_COUNT(uph_settings_snap_values));
	settings->ui.playhead_lock_position = NAUI_CLAMP(settings->ui.playhead_lock_position, 0.0f, 1.0f);

	settings->midi.record_quantize_grid = uph_settings_nearest(settings->midi.record_quantize_grid, uph_settings_snap_values, UPH_SETTINGS_COUNT(uph_settings_snap_values));
	settings->midi.grid_division = uph_settings_nearest(settings->midi.grid_division, uph_settings_snap_values, UPH_SETTINGS_COUNT(uph_settings_snap_values));
	settings->midi.count_in_bars = NAUI_MIN(settings->midi.count_in_bars, (uint32_t)UPH_SETTINGS_COUNT_IN_MAX);
	settings->midi.default_velocity = (uint8_t)NAUI_CLAMP((int32_t)settings->midi.default_velocity, UPH_SETTINGS_VELOCITY_MIN, UPH_SETTINGS_VELOCITY_MAX);
	settings->midi.default_channel = (uint8_t)NAUI_CLAMP((int32_t)settings->midi.default_channel, UPH_SETTINGS_CHANNEL_MIN, UPH_SETTINGS_CHANNEL_MAX);
}

bool uph_settings_load(void)
{
	const Naui_Path path = UPHONIC_SETTINGS_FILE;
	if (!naui_path_exists(path))
		return false;

	return uph_io_load_editor_settings(&uph_state, path);
}

bool uph_settings_save(void)
{
	const Naui_Path folder = UPHONIC_FOLDER;
	if (!naui_path_exists(folder) && !naui_directories_create(folder))
	{
		naui_log(NAUI_LOG_ERROR, "Failed to create the settings folder: %s", folder.data);
		return false;
	}

	return uph_io_save_editor_settings(&uph_state, UPHONIC_SETTINGS_FILE);
}

static void uph_settings_refresh_themes(Uph_SettingsData *data)
{
	data->themes.count = 0;

	const Naui_Path themes_dir = naui_path_join(naui_directory_get(NAUI_DIR_ASSETS), NAUI_PATH("Themes"));
	Naui_List(Naui_DirEntry) entries = naui_directory_filter(themes_dir, NULL, NAUI_EXTENSIONS(".json"));

	for (size_t i = 0; i < (size_t)naui_list_len(entries) && data->themes.count < UPH_SETTINGS_MAX_OPTIONS; i++)
	{
		if (entries[i].is_directory)
			continue;

		const Naui_StringView stem = naui_file_stem(&entries[i].path);
		if (stem.length == 0 || stem.length >= UPH_SETTINGS_THEME_NAME_MAX)
			continue;

		data->themes.name[data->themes.count++] = naui_view_to_string(stem);
	}

	naui_directory_filter_free(entries);
}

static void uph_settings_refresh_languages(Uph_SettingsData *data)
{
	data->language_count = 0;

	Naui_List(Naui_LanguageMeta) languages = naui_localization_get_languages();
	for (size_t i = 0; i < (size_t)naui_list_len(languages) && data->language_count < UPH_SETTINGS_MAX_LANGUAGES; i++)
	{
		const Naui_LanguageMeta *meta = &languages[i];
		Uph_SettingsLanguage *language = &data->languages[data->language_count++];
		const Naui_String stem = naui_view_to_string(naui_file_stem(&meta->filename));
		naui_localization_split_locale(stem.data, &language->language, &language->region);
		language->label = meta->display_name.length > 0 ? meta->display_name : stem;
	}
}

static void uph_settings_refresh_audio_devices(Uph_SettingsData *data)
{
	data->audio_outputs.count = 0;
	data->audio_inputs.count = 0;

	if (!uph_settings_audio_context_ready)
	{
		if (ma_context_init(NULL, 0, NULL, &uph_settings_audio_context) != MA_SUCCESS)
			return;

		uph_settings_audio_context_ready = true;
	}

	ma_device_info *playback = NULL;
	ma_device_info *capture = NULL;
	ma_uint32 playback_count = 0;
	ma_uint32 capture_count = 0;

	if (ma_context_get_devices(&uph_settings_audio_context, &playback, &playback_count, &capture, &capture_count) != MA_SUCCESS)
		return;

	for (ma_uint32 i = 0; i < playback_count && data->audio_outputs.count < UPH_SETTINGS_MAX_OPTIONS; i++)
		data->audio_outputs.name[data->audio_outputs.count++] = naui_string_from_cstr(playback[i].name);

	for (ma_uint32 i = 0; i < capture_count && data->audio_inputs.count < UPH_SETTINGS_MAX_OPTIONS; i++)
		data->audio_inputs.name[data->audio_inputs.count++] = naui_string_from_cstr(capture[i].name);
}

static void uph_settings_refresh_midi_devices(Uph_SettingsData *data)
{
	data->midi_inputs.count = 0;
	data->midi_outputs.count = 0;

	cmidi_device_t devices[CMIDI_MAX_DEVICES];
	const int count = cmidi_get_devices(devices, CMIDI_MAX_DEVICES);

	for (int i = 0; i < count; i++)
	{
		if (devices[i].can_input)
			data->midi_inputs.device[data->midi_inputs.count++] = devices[i];

		if (devices[i].can_output)
			data->midi_outputs.device[data->midi_outputs.count++] = devices[i];
	}
}

static void uph_settings_refresh_all(Uph_SettingsData *data)
{
	uph_settings_refresh_themes(data);
	uph_settings_refresh_languages(data);
	uph_settings_refresh_audio_devices(data);
	uph_settings_refresh_midi_devices(data);
	data->lists_ready = true;
}

static void uph_settings_apply_theme_deferred(void *name)
{
	naui_load_theme((const char*)name);
}

static void uph_settings_apply_theme(const char *name)
{
	char buffer[UPH_SETTINGS_THEME_NAME_MAX];
	snprintf(buffer, sizeof(buffer), "%s", name);
	naui_defer(uph_settings_apply_theme_deferred, buffer, sizeof(buffer));
}

static void uph_settings_apply_midi_thru(void)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;

	if (!midi->input)
		return;

	if (midi->midi_thru_enabled && midi->output)
		cmidi_set_thru(midi->input, midi->output);
	else
		cmidi_clear_thru(midi->input);
}

static void uph_settings_select_midi_input(const cmidi_device_t *device)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;

	if (midi->input)
		cmidi_close(midi->input);

	midi->input = device ? cmidi_open_input(device->id, uph_midi_on_input, NULL) : NULL;
	midi->input_device = device ? naui_string_from_cstr(device->name) : (Naui_String){ 0 };
	uph_settings_apply_midi_thru();
}

static void uph_settings_select_midi_output(const cmidi_device_t *device)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;
	if (midi->input)
		cmidi_clear_thru(midi->input);

	if (midi->output)
	{
		cmidi_stop_all_notes(midi->output);
		cmidi_close(midi->output);
	}

	midi->output = device ? cmidi_open_output(device->id) : NULL;
	midi->output_device = device ? naui_string_from_cstr(device->name) : (Naui_String){ 0 };
	uph_settings_apply_midi_thru();
}

static const cmidi_device_t *uph_settings_find_midi_device(const Uph_SettingsMidiList *list, const char *name)
{
	for (uint32_t i = 0; i < list->count; i++)
	{
		if (name[0] && strcmp(list->device[i].name, name) == 0)
			return &list->device[i];
	}

	return NULL;
}

// Opens the ports named in the settings. A device that isn't plugged in is skipped and its name stays saved, so it
// comes back the next time it is there. use_first_device is for a first run (no settings file yet): the first
// available device is used, which is what startup did before settings were saved.
void uph_settings_open_midi_ports(const bool use_first_device)
{
	Uph_SettingsData *data = &uph_settings_data;
	Uph_MIDISettings *midi = &uph_state.settings.midi;

	uph_settings_refresh_midi_devices(data);

	const cmidi_device_t *input = NULL;
	const cmidi_device_t *output = NULL;

	if (use_first_device)
	{
		input = data->midi_inputs.count > 0 ? &data->midi_inputs.device[0] : NULL;
		output = data->midi_outputs.count > 0 ? &data->midi_outputs.device[0] : NULL;
	}
	else
	{
		input = uph_settings_find_midi_device(&data->midi_inputs, midi->input_device.data);
		output = uph_settings_find_midi_device(&data->midi_outputs, midi->output_device.data);
	}

	if (input)
		uph_settings_select_midi_input(input);

	if (output)
		uph_settings_select_midi_output(output);
}

static inline float uph_settings_font_size(void)
{
	return NAUI_DPI(naui_theme_float("uph_ui_font_size"));
}

static void uph_settings_row_begin(const char *label, const Leaf_ID label_id)
{
	const float font_size = uph_settings_font_size();

	leaf_begin_element((Leaf_ElementConfig){
		.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_ROW_HEIGHT)) },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(12.0f)
	});

	leaf_begin_element((Leaf_ElementConfig){
		.id = label_id,
		.size = { LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_LABEL_WIDTH)), LEAF_SIZE_FULL },
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.clip_children = true
	});
	{
		leaf_text(label, { .font_size = { font_size }, .color = { naui_theme_color("uph_ui_text_color") } });
	}
	leaf_end_element();

	leaf_begin_element((Leaf_ElementConfig){
		.size = { LEAF_SIZE_GROW_MAX(NAUI_DPI(UPH_SETTINGS_CONTROL_WIDTH)), LEAF_SIZE_FULL },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8.0f)
	});
}

static void uph_settings_row_end(void)
{
	leaf_end_element();
	leaf_end_element();
}

static void uph_settings_hint(const char *text)
{
	leaf_text(text, { .font_size = { uph_settings_font_size() }, .color = { naui_theme_color("naui_panel_text_color") } });
}

static bool uph_settings_section(const char *title, const bool first, const char *button_text, const Leaf_ID button_id)
{
	bool pressed = false;

	leaf({
		.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIT },
		.direction = LEAF_DIRECTION_VERTICAL,
		.padding = { 0, 0, first ? 0 : (int32_t)NAUI_DPI(14.0f), 0 },
		.child_gap = NAUI_DPI(6.0f)
	})
	{
		leaf({
			.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIT },
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER }
		})
		{
			leaf({ .size = { LEAF_SIZE_GROW, LEAF_SIZE_FIT } })
			{
				leaf_text(title, { .font_size = { uph_settings_font_size() * 1.1f }, .color = { naui_theme_color("uph_ui_text_color") } });
			}

			if (button_text)
				pressed = uph_ui_text_button(button_text, button_id);
		}

		leaf({
			.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(1.0f)) },
			.color = { naui_theme_color("uph_ui_frame_secondary_bg_color") }
		});
	}

	return pressed;
}

static bool uph_settings_value_dropdown(uint32_t *value, const uint32_t *values, const char *const *labels, const uint32_t count, const Leaf_ID id)
{
	uint32_t index = 0;
	uint32_t best_distance = UINT32_MAX;

	for (uint32_t i = 0; i < count; i++)
	{
		const uint32_t distance = values[i] > *value ? values[i] - *value : *value - values[i];
		if (distance < best_distance)
		{
			best_distance = distance;
			index = i;
		}
	}

	uint32_t selected = index;
	if (uph_ui_dropdown(labels, count, &selected, id) && selected != index)
	{
		*value = values[selected];
		return true;
	}

	return false;
}

static bool uph_settings_name_dropdown(Naui_String *value, const Uph_SettingsNameList *list, const char *first_label, const Leaf_ID id)
{
	const char *items[UPH_SETTINGS_MAX_OPTIONS + 2];
	uint32_t count = 0;
	uint32_t index = 0;
	const uint32_t offset = first_label ? 1 : 0;
	bool found = first_label && value->length == 0;

	if (first_label)
		items[count++] = first_label;

	for (uint32_t i = 0; i < list->count; i++)
	{
		if (!found && strcmp(list->name[i].data, value->data) == 0)
		{
			index = count;
			found = true;
		}

		items[count++] = list->name[i].data;
	}

	if (!found && value->length > 0)
	{
		index = count;
		items[count++] = value->data;
	}

	uint32_t selected = index;
	if (!uph_ui_dropdown(items, count, &selected, id) || selected == index)
		return false;

	if (first_label && selected == 0)
	{
		*value = (Naui_String){ 0 };
		return true;
	}

	if (selected >= offset && selected - offset < list->count)
	{
		*value = list->name[selected - offset];
		return true;
	}

	return false;
}

static bool uph_settings_language_dropdown(Uph_SettingsData *data, const Leaf_ID id)
{
	Uph_GeneralSettings *general = &uph_state.settings.general;

	const char *items[UPH_SETTINGS_MAX_LANGUAGES + 1];
	uint32_t count = 0;
	uint32_t index = 0;
	bool found = false;

	for (uint32_t i = 0; i < data->language_count; i++)
	{
		const Uph_SettingsLanguage *language = &data->languages[i];
		if (!found
			&& strcmp(language->language.data, general->language_code.data) == 0
			&& strcmp(language->region.data, general->region_code.data) == 0)
		{
			index = count;
			found = true;
		}

		items[count++] = language->label.data;
	}

	if (!found)
	{
		data->language_unlisted = naui_string_format("%s-%s", general->language_code.data, general->region_code.data);
		index = count;
		items[count++] = data->language_unlisted.data;
	}

	uint32_t selected = index;
	if (uph_ui_dropdown(items, count, &selected, id) && selected != index && selected < data->language_count)
	{
		general->language_code = data->languages[selected].language;
		general->region_code = data->languages[selected].region;
		data->language_changed = true;
		return true;
	}

	return false;
}

static bool uph_settings_midi_dropdown(Uph_SettingsData *data, const bool input, const Leaf_ID id)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;
	const Uph_SettingsMidiList *list = input ? &data->midi_inputs : &data->midi_outputs;
	Naui_String *unlisted = &data->midi_unlisted[input ? 0 : 1];
	cmidi_port_t *port = input ? midi->input : midi->output;
	const char *port_name = port ? cmidi_port_name(port) : NULL;

	const char *items[UPH_SETTINGS_MAX_OPTIONS + 2];
	uint32_t count = 0;
	uint32_t index = 0;

	items[count++] = NAUI_TR("settings.midi.none");

	for (uint32_t i = 0; i < list->count; i++)
	{
		if (index == 0 && port_name && strcmp(port_name, list->device[i].name) == 0)
			index = count;

		items[count++] = list->device[i].name;
	}

	if (port && index == 0)
	{
		*unlisted = naui_string_from_cstr(port_name ? port_name : "?");
		index = count;
		items[count++] = unlisted->data;
	}

	uint32_t selected = index;
	if (!uph_ui_dropdown(items, count, &selected, id) || selected == index)
		return false;

	const cmidi_device_t *device = (selected >= 1 && selected - 1 < list->count) ? &list->device[selected - 1] : NULL;
	if (selected != 0 && !device)
		return false;

	if (input)
		uph_settings_select_midi_input(device);
	else
		uph_settings_select_midi_output(device);

	return true;
}

static bool uph_settings_drag_u32(uint32_t *value, const Leaf_ID id, const float speed, const int32_t min, const int32_t max, const char *format)
{
	static int32_t scratch;
	scratch = (int32_t)NAUI_CLAMP((int64_t)*value, (int64_t)min, (int64_t)max);

	if (!uph_ui_drag_int(&scratch, id, speed, min, max, format, UPH_UI_DRAG_CLAMPED))
		return false;

	*value = (uint32_t)scratch;
	return true;
}

static bool uph_settings_drag_u8(uint8_t *value, const Leaf_ID id, const float speed, const int32_t min, const int32_t max, const char *format)
{
	static int32_t scratch;
	scratch = NAUI_CLAMP((int32_t)*value, min, max);

	if (!uph_ui_drag_int(&scratch, id, speed, min, max, format, UPH_UI_DRAG_CLAMPED))
		return false;

	*value = (uint8_t)scratch;
	return true;
}

static bool uph_settings_slider_u8(uint8_t *value, const Leaf_ID id, const int32_t min, const int32_t max, const char *format)
{
	static int32_t scratch;
	scratch = NAUI_CLAMP((int32_t)*value, min, max);

	if (!uph_ui_slider_int(&scratch, id, min, max, format, UPH_UI_SLIDER_FLAGS_NONE))
		return false;

	*value = (uint8_t)scratch;
	return true;
}

static bool uph_settings_checkbox_row(const char *label, bool *value, const Leaf_ID id)
{
	const Leaf_ID label_id = { .value = id.value ^ 0x9E3779B97F4A7C15ull };
	const bool before = *value;

	if (uph_ui_widget_hovered(label_id))
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
			*value = !*value;
	}

	UPH_SETTINGS_ROW(label, label_id)
	{
		uph_ui_checkbox(value, id);
	}

	return *value != before;
}

static void uph_settings_page_general(Uph_SettingsData *data)
{
	Uph_GeneralSettings *general = &uph_state.settings.general;

	uph_settings_section(NAUI_TR("settings.general.appearance"), true, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.theme"), UPH_SETTINGS_NO_ID)
	{
		if (uph_settings_name_dropdown(&general->theme, &data->themes, NULL, leaf_id("uph_settings_theme")))
			uph_settings_apply_theme(general->theme.data);
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.language"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_language_dropdown(data, leaf_id("uph_settings_language"));

		if (data->language_changed)
			uph_settings_hint(NAUI_TR("settings.general.language_restart"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.ui_scale"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_slider_float(&general->ui_scale, leaf_id("uph_settings_ui_scale"), UPH_SETTINGS_UI_SCALE_MIN, UPH_SETTINGS_UI_SCALE_MAX, "%.2fx", UPH_UI_SLIDER_FLAGS_NONE);
	}

	uph_settings_section(NAUI_TR("settings.general.behavior"), false, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.autosave"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_drag_int(&general->autosave_timer, leaf_id("uph_settings_autosave"), 0.25f, 0, UPH_SETTINGS_AUTOSAVE_MAX, "%d min", UPH_UI_DRAG_CLAMPED);
		uph_settings_hint(NAUI_TR("settings.general.autosave_hint"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.undo_limit"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_drag_u32(&general->undo_history_limit, leaf_id("uph_settings_undo_limit"), 1.0f, UPH_SETTINGS_UNDO_MIN, UPH_SETTINGS_UNDO_MAX, "%d steps");
	}

	uph_settings_checkbox_row(NAUI_TR("settings.general.confirm_on_exit"), &general->confirm_on_exit, leaf_id("uph_settings_confirm_exit"));
	uph_settings_checkbox_row(NAUI_TR("settings.general.confirm_on_delete"), &general->confirm_on_delete, leaf_id("uph_settings_confirm_delete"));
	uph_settings_checkbox_row(NAUI_TR("settings.general.copy_resources"), &general->copy_resources, leaf_id("uph_settings_copy_resources"));
}

static void uph_settings_page_audio(Uph_SettingsData *data)
{
	Uph_AudioSettings *audio = &uph_state.settings.audio;

	if (uph_settings_section(NAUI_TR("settings.audio.devices"), true, NAUI_TR("settings.refresh"), leaf_id("uph_settings_audio_refresh")))
		uph_settings_refresh_audio_devices(data);

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.output_device"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_name_dropdown(&audio->output_device, &data->audio_outputs, NAUI_TR("settings.audio.default_device"), leaf_id("uph_settings_audio_output"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.input_device"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_name_dropdown(&audio->input_device, &data->audio_inputs, NAUI_TR("settings.audio.default_device"), leaf_id("uph_settings_audio_input"));
	}

	uph_settings_checkbox_row(NAUI_TR("settings.audio.exclusive_mode"), &audio->exclusive_mode, leaf_id("uph_settings_exclusive"));
	uph_settings_checkbox_row(NAUI_TR("settings.audio.auto_restart"), &audio->auto_restart_on_device_change, leaf_id("uph_settings_auto_restart"));

	uph_settings_section(NAUI_TR("settings.audio.performance"), false, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.sample_rate"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&audio->sample_rate, uph_settings_sample_rates, uph_settings_sample_rate_labels,
			UPH_SETTINGS_COUNT(uph_settings_sample_rates), leaf_id("uph_settings_sample_rate"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.buffer_size"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&audio->buffer_size, uph_settings_buffer_sizes, uph_settings_buffer_size_labels,
			UPH_SETTINGS_COUNT(uph_settings_buffer_sizes), leaf_id("uph_settings_buffer_size"));

		if (audio->sample_rate > 0)
		{
			const Naui_String latency = naui_string_format("%.1f ms", (float)audio->buffer_size * 1000.0f / (float)audio->sample_rate);
			uph_settings_hint(latency.data);
		}
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.channels"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&audio->channels, uph_settings_channel_counts, uph_settings_channel_labels,
			UPH_SETTINGS_COUNT(uph_settings_channel_counts), leaf_id("uph_settings_channels"));
	}

	uph_settings_checkbox_row(NAUI_TR("settings.audio.underrun_protection"), &audio->underrun_protection, leaf_id("uph_settings_underrun"));

	uph_settings_section(NAUI_TR("settings.audio.monitoring"), false, NULL, UPH_SETTINGS_NO_ID);

	uph_settings_checkbox_row(NAUI_TR("settings.audio.software_monitoring"), &audio->software_monitoring, leaf_id("uph_settings_software_monitoring"));

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.monitoring_gain"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_slider_float(&audio->monitoring_gain, leaf_id("uph_settings_monitoring_gain"), 0.0f, 1.0f, "%.2f", UPH_UI_SLIDER_FLAGS_NONE);
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.vu_hold"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_drag_float(&audio->vu_meter_hold_time, leaf_id("uph_settings_vu_hold"), 0.01f, 0.0f, UPH_SETTINGS_VU_HOLD_MAX, "%.2f s", UPH_UI_DRAG_CLAMPED);
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.vu_decay"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_drag_float(&audio->vu_meter_decay_time, leaf_id("uph_settings_vu_decay"), 0.01f, UPH_SETTINGS_VU_DECAY_MIN, UPH_SETTINGS_VU_DECAY_MAX, "%.2f s", UPH_UI_DRAG_CLAMPED);
	}
}

static void uph_settings_axis_pair(Naui_Vec2 *value, const float speed, const float min, const float max, const char *id_x, const char *id_y)
{
	const Leaf_Color axis_color = naui_theme_color("naui_panel_text_color");
	const float font_size = uph_settings_font_size();

	leaf_text("X", { .font_size = { font_size }, .color = { axis_color } });
	uph_ui_drag_float(&value->x, leaf_id(id_x), speed, min, max, "%.2f", UPH_UI_DRAG_CLAMPED);
	leaf_text("Y", { .font_size = { font_size }, .color = { axis_color } });
	uph_ui_drag_float(&value->y, leaf_id(id_y), speed, min, max, "%.2f", UPH_UI_DRAG_CLAMPED);
}

static void uph_settings_page_interface(Uph_SettingsData *data)
{
	(void)data;
	Uph_UISettings *ui = &uph_state.settings.ui;

	uph_settings_section(NAUI_TR("settings.ui.navigation"), true, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.scroll_sensitivity"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_axis_pair(&ui->scroll_sensitivity, 0.01f, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX, "uph_settings_scroll_x", "uph_settings_scroll_y");
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.zoom_sensitivity"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_axis_pair(&ui->zoom_sensitivity, 0.01f, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX, "uph_settings_zoom_x", "uph_settings_zoom_y");
	}

	uph_settings_section(NAUI_TR("settings.ui.editing"), false, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.default_snap"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&ui->default_snap, uph_settings_snap_values, uph_settings_snap_labels,
			UPH_SETTINGS_COUNT(uph_settings_snap_values), leaf_id("uph_settings_default_snap"));
	}

	uph_settings_checkbox_row(NAUI_TR("settings.ui.follow_playhead"), &ui->follow_playhead, leaf_id("uph_settings_follow_playhead"));

	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.playhead_lock"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_slider_float(&ui->playhead_lock_position, leaf_id("uph_settings_playhead_lock"), 0.0f, 1.0f, "%.2f", UPH_UI_SLIDER_FLAGS_NONE);
	}

	uph_settings_checkbox_row(NAUI_TR("settings.ui.show_tooltips"), &ui->show_tooltips, leaf_id("uph_settings_tooltips"));
}

static void uph_settings_page_midi(Uph_SettingsData *data)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;

	if (uph_settings_section(NAUI_TR("settings.midi.devices"), true, NAUI_TR("settings.refresh"), leaf_id("uph_settings_midi_refresh")))
		uph_settings_refresh_midi_devices(data);

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.input"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_midi_dropdown(data, true, leaf_id("uph_settings_midi_input"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.output"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_midi_dropdown(data, false, leaf_id("uph_settings_midi_output"));
	}

	if (uph_settings_checkbox_row(NAUI_TR("settings.midi.thru"), &midi->midi_thru_enabled, leaf_id("uph_settings_midi_thru")))
		uph_settings_apply_midi_thru();

	uph_settings_section(NAUI_TR("settings.midi.recording"), false, NULL, UPH_SETTINGS_NO_ID);

	uph_settings_checkbox_row(NAUI_TR("settings.midi.record_quantize"), &midi->record_quantize, leaf_id("uph_settings_record_quantize"));

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.quantize_grid"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&midi->record_quantize_grid, uph_settings_snap_values, uph_settings_snap_labels,
			UPH_SETTINGS_COUNT(uph_settings_snap_values), leaf_id("uph_settings_quantize_grid"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.count_in"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_drag_u32(&midi->count_in_bars, leaf_id("uph_settings_count_in"), 0.1f, 0, UPH_SETTINGS_COUNT_IN_MAX, "%d bars");
	}

	uph_settings_checkbox_row(NAUI_TR("settings.midi.overdub"), &midi->record_over_dub, leaf_id("uph_settings_overdub"));

	uph_settings_section(NAUI_TR("settings.midi.editing"), false, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.grid_division"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&midi->grid_division, uph_settings_snap_values, uph_settings_snap_labels,
			UPH_SETTINGS_COUNT(uph_settings_snap_values), leaf_id("uph_settings_grid_division"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.default_velocity"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_slider_u8(&midi->default_velocity, leaf_id("uph_settings_default_velocity"), UPH_SETTINGS_VELOCITY_MIN, UPH_SETTINGS_VELOCITY_MAX, "%d");
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.default_channel"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_drag_u8(&midi->default_channel, leaf_id("uph_settings_default_channel"), 0.1f, UPH_SETTINGS_CHANNEL_MIN, UPH_SETTINGS_CHANNEL_MAX, "%d");
	}

	uph_settings_checkbox_row(NAUI_TR("settings.midi.filter_channel"), &midi->filter_by_channel, leaf_id("uph_settings_filter_channel"));
}

static bool uph_settings_add_plugin_path(Uph_SettingsData *data)
{
	Uph_PluginSettings *plugin = &uph_state.settings.plugin;

	const Naui_String text = naui_string_trim(data->new_plugin_path);
	if (text.length == 0)
		return false;

	const Naui_Path path = naui_path_from_cstr(text.data);
	if (!naui_path_is_directory(path))
	{
		data->rejected_plugin_path = text;
		return false;
	}

	for (int32_t i = 0; i < (int32_t)naui_list_len(plugin->plugin_paths); i++)
	{
		if (strcmp(plugin->plugin_paths[i].data, path.data) == 0)
		{
			data->new_plugin_path = (Naui_String){ 0 };
			return false;
		}
	}

	naui_list_push(plugin->plugin_paths, path);
	data->new_plugin_path = (Naui_String){ 0 };
	data->rejected_plugin_path = (Naui_String){ 0 };
	return true;
}

static void uph_settings_page_plugins(Uph_SettingsData *data)
{
	Uph_PluginSettings *plugin = &uph_state.settings.plugin;
	const float font_size = uph_settings_font_size();

	uph_settings_section(NAUI_TR("settings.plugins.paths"), true, NULL, UPH_SETTINGS_NO_ID);

	const int32_t path_count = (int32_t)naui_list_len(plugin->plugin_paths);
	int32_t remove_index = -1;

	for (int32_t i = 0; i < path_count; i++)
	{
		leaf({
			.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIT },
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(8.0f)
		})
		{
			leaf({
				.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIT },
				.padding = LEAF_PADDING_AXES(NAUI_DPI(8.0f), NAUI_DPI(6.0f)),
				.color = { naui_theme_color("uph_ui_frame_bg_color") },
				.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(naui_theme_float("uph_ui_frame_rounding")), NAUI_CORNER_ALL),
				.clip_children = true
			})
			{
				leaf_text(plugin->plugin_paths[i].data, { .font_size = { font_size }, .color = { naui_theme_color("uph_ui_text_color") } });
			}

			if (uph_ui_text_button(NAUI_TR("settings.plugins.remove"), leaf_id_indexed("uph_settings_plugin_remove", i)))
				remove_index = i;
		}
	}

	if (path_count == 0)
		uph_settings_hint(NAUI_TR("settings.plugins.no_paths"));

	if (remove_index >= 0)
		naui_list_remove(plugin->plugin_paths, remove_index);

	bool add = false;
	leaf({
		.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIT },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8.0f),
		.padding = { 0, 0, (int32_t)NAUI_DPI(4.0f), 0 }
	})
	{
		if (uph_ui_textfield(&data->new_plugin_path, leaf_id("uph_settings_plugin_path"), UPH_UI_TEXTFIELD_FLAGS_NONE, NAUI_TR("settings.plugins.path_placeholder")))
			add = true;

		if (uph_ui_text_button(NAUI_TR("settings.plugins.add"), leaf_id("uph_settings_plugin_add")))
			add = true;
	}

	if (add)
		uph_settings_add_plugin_path(data);

	if (data->rejected_plugin_path.length > 0 && strcmp(data->rejected_plugin_path.data, data->new_plugin_path.data) == 0)
		leaf_text(NAUI_TR("settings.plugins.path_invalid"), { .font_size = { font_size }, .color = { naui_theme_color("uph_stop_icon_color") } });

	uph_settings_section(NAUI_TR("settings.plugins.options"), false, NULL, UPH_SETTINGS_NO_ID);
	uph_settings_checkbox_row(NAUI_TR("settings.plugins.sandbox"), &plugin->sandbox_plugins, leaf_id("uph_settings_sandbox"));
}

static void uph_settings_sidebar(Uph_SettingsData *data)
{
	static const char *const keys[UPH_SETTINGS_PAGE_COUNT] =
	{
		"settings.page.general",
		"settings.page.audio",
		"settings.page.interface",
		"settings.page.midi",
		"settings.page.plugins"
	};

	const float font_size = uph_settings_font_size();
	const float rounding = NAUI_DPI(naui_theme_float("uph_ui_frame_rounding"));

	leaf({
		.size = { LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_SIDEBAR_WIDTH)), LEAF_SIZE_FULL },
		.direction = LEAF_DIRECTION_VERTICAL,
		.padding = LEAF_PADDING_ALL(NAUI_DPI(8.0f)),
		.child_gap = NAUI_DPI(2.0f)
	})
	{
		for (uint32_t i = 0; i < UPH_SETTINGS_PAGE_COUNT; i++)
		{
			const Leaf_ID id = leaf_id_indexed("uph_settings_page", i);
			const bool selected = data->page == (Uph_SettingsPage)i;
			const bool hovered = uph_ui_widget_hovered(id);

			if (hovered)
			{
				naui_set_cursor(NAUI_CURSOR_HAND);
				if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
					data->page = (Uph_SettingsPage)i;
			}

			Leaf_Color background = LEAF_COLOR_TRANSPARENT;
			if (selected)
				background = naui_theme_color("uph_ui_frame_toggled_bg_color");
			else if (hovered)
				background = naui_theme_color("uph_ui_frame_secondary_bg_color");

			leaf({
				.id = id,
				.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(30.0f)) },
				.direction = LEAF_DIRECTION_HORIZONTAL,
				.child_alignment = { LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER },
				.color = { background },
				.rounding = LEAF_ROUNDING_FIXED(rounding, NAUI_CORNER_ALL)
			})
			{
				leaf({
					.size = { LEAF_SIZE_FIXED(NAUI_DPI(3.0f)), LEAF_SIZE_PERCENT(0.6f) },
					.color = { selected ? naui_theme_color("uph_ui_slider_fill_color") : LEAF_COLOR_TRANSPARENT },
					.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2.0f), NAUI_CORNER_ALL)
				});

				leaf({
					.size = { LEAF_SIZE_GROW, LEAF_SIZE_FIT },
					.padding = LEAF_PADDING_AXES(NAUI_DPI(10.0f), 0),
					.clip_children = true
				})
				{
					leaf_text(NAUI_TR(keys[i]), { .font_size = { font_size }, .color = { naui_theme_color("uph_ui_text_color") } });
				}
			}
		}
	}
}

static void uph_settings_on_attach(void)
{
	const Naui_PanelID this = naui_current_panel();

	naui_panel_set_title(this, NAUI_TR("settings.title"));
	naui_panel_enable_flags(this, NAUI_PANEL_FLAG_NO_DOCK);
	float scale = naui_app_dpi_scale();
	if (scale <= 0.0f)
		scale = 1.0f;

	Naui_Vec2 size = { 800.0f * scale, 580.0f * scale };
	if (naui_app_width() > 0)
		size.x = NAUI_MIN(size.x, (float)naui_app_width() - 64.0f);
	if (naui_app_height() > 0)
		size.y = NAUI_MIN(size.y, (float)naui_app_height() - 64.0f);

	naui_panel_set_size(this, size);
	naui_panel_set_min_size(this, (Naui_Vec2){ 620.0f * scale, 560.0f * scale });
}

static void uph_settings_on_detach(void)
{
	if (uph_settings_audio_context_ready)
	{
		ma_context_uninit(&uph_settings_audio_context);
		uph_settings_audio_context_ready = false;
	}
}

static void uph_settings_on_open(void)
{
	uph_settings_refresh_all(&uph_settings_data);
	uph_settings_data.was_opened = true;
}

static void uph_settings_on_close(void)
{
	if (uph_settings_data.was_opened)
		uph_settings_save();
}

static void uph_settings_on_update(void)
{
	Uph_SettingsData *data = &uph_settings_data;
	if (!data->lists_ready)
		uph_settings_refresh_all(data);

	leaf({
		.size = { LEAF_SIZE_FULL, LEAF_SIZE_FULL },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		uph_settings_sidebar(data);

		leaf({
			.size = { LEAF_SIZE_GROW, LEAF_SIZE_FULL },
			.direction = LEAF_DIRECTION_VERTICAL,
			.padding = LEAF_PADDING_AXES(NAUI_DPI(24.0f), NAUI_DPI(16.0f)),
			.child_gap = NAUI_DPI(4.0f),
			.color = { naui_theme_color("naui_panel_title_bg_color") },
			.clip_children = true
		})
		{
			switch (data->page)
			{
				case UPH_SETTINGS_PAGE_GENERAL:   uph_settings_page_general(data);   break;
				case UPH_SETTINGS_PAGE_AUDIO:	 uph_settings_page_audio(data);	 break;
				case UPH_SETTINGS_PAGE_INTERFACE: uph_settings_page_interface(data); break;
				case UPH_SETTINGS_PAGE_MIDI:	  uph_settings_page_midi(data);	  break;
				case UPH_SETTINGS_PAGE_PLUGINS:   uph_settings_page_plugins(data);   break;
				default: break;
			}
		}
	}
}
