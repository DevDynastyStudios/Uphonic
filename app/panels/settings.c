#define UPH_IO_FILE_SETTINGS "settings.json"
#define UPHONIC_SETTINGS_FILE naui_path_join(UPHONIC_FOLDER, NAUI_PATH(UPH_IO_FILE_SETTINGS))

NAUI_PANEL(uph_settings)

#define UPH_SETTINGS_MAX_OPTIONS		64
#define UPH_SETTINGS_MAX_LANGUAGES		32

#define UPH_SETTINGS_THEME_NAME_MAX		48
#define UPH_SETTINGS_THEME_ROWS			3
#define UPH_SETTINGS_THEME_CARD_WIDTH	184
#define UPH_SETTINGS_THEME_CARD_PREVIEW	104
#define UPH_SETTINGS_THEME_HEADER_WIDTH	36

#define UPH_SETTINGS_SIDEBAR_WIDTH		150
#define UPH_SETTINGS_LABEL_WIDTH		200
#define UPH_SETTINGS_CONTROL_WIDTH		320
#define UPH_SETTINGS_ROW_HEIGHT			28
#define UPH_SETTINGS_CHORD_WIDTH		150
#define UPH_SETTINGS_KEYCAP_INSET		3
#define UPH_SETTINGS_KEYCAP_PADDING		6
#define UPH_SETTINGS_KEYCAP_FONT_SIZE	12

#define UPH_SETTINGS_UI_SCALE_MIN		0.5f
#define UPH_SETTINGS_UI_SCALE_MAX		2.0f
#define UPH_SETTINGS_AUTOSAVE_MAX		600
#define UPH_SETTINGS_UNDO_MIN			1
#define UPH_SETTINGS_UNDO_MAX			1000
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
	UPH_SETTINGS_PAGE_SHORTCUTS,
	UPH_SETTINGS_PAGE_THEMES,
	UPH_SETTINGS_PAGE_AUDIO,
	// UPH_SETTINGS_PAGE_INTERFACE,
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
	Leaf_Color viewport;
	Leaf_Color toolbox;
	Leaf_Color border;
	Leaf_Color header;
	Leaf_Color header_border;
	Leaf_Color row[2];
	Leaf_Color logo;
	Leaf_Color text;
	Leaf_Color accent;
	Leaf_Color palette[UPH_SETTINGS_THEME_ROWS];
}
Uph_SettingsThemePreview;

typedef struct
{
	Uph_SettingsPage page;
	bool lists_ready;
	bool was_opened;

	Uph_SettingsNameList themes;
	Uph_SettingsThemePreview theme_previews[UPH_SETTINGS_MAX_OPTIONS];
	float themes_scroll;
	Uph_SettingsNameList audio_outputs;
	Uph_SettingsNameList audio_inputs;
	Uph_SettingsNameList asio_drivers;
	Uph_SettingsMidiList midi_inputs;
	Uph_SettingsMidiList midi_outputs;

	Uph_AudioSettings audio_pending;

	Uph_SettingsLanguage languages[UPH_SETTINGS_MAX_LANGUAGES];
	uint32_t language_count;

	Naui_String language_unlisted;
	Naui_String midi_unlisted[2];

	const Uph_KeybindInfo *capturing_keybind;
	float shortcuts_scroll;
}
Uph_SettingsState;

static Uph_SettingsState _uph_settings_state;

static const uint32_t uph_settings_sample_rates[] = { 44100, 48000, 88200, 96000, 176400, 192000 };
static const char *const uph_settings_sample_rate_labels[] = { "44.1 kHz", "48 kHz", "88.2 kHz", "96 kHz", "176.4 kHz", "192 kHz" };

static const uint32_t uph_settings_buffer_sizes[] = { 64, 128, 256, 512, 1024, 2048 };
static const char *const uph_settings_buffer_size_labels[] = { "64 samples", "128 samples", "256 samples", "512 samples", "1024 samples", "2048 samples" };

static const uint32_t uph_settings_channel_counts[] = { 1, 2 };
static const char *const uph_settings_channel_labels[] = { "1 (Mono)", "2 (Stereo)" };

static const uint32_t uph_settings_input_channels[] = { 0, 1 };
static const char *const uph_settings_input_channel_labels[] = { "1", "2" };

static const uint32_t uph_settings_snap_values[] = { UPH_SNAP_BEAT, UPH_SNAP_HALF, UPH_SNAP_QUARTER, UPH_SNAP_EIGHTH, UPH_SNAP_SIXTEENTH };
static const char *const uph_settings_snap_labels[] = { "Beat", "1/2", "1/4", "1/8", "1/16" };

void uph_settings_set_defaults(void)
{
	Uph_Settings *settings = &uph_state.settings;

	uph_keybinds_set_defaults();

	settings->general.theme = naui_string_from_cstr("Fish");
	settings->general.language_code = naui_string_from_cstr("en");
	settings->general.region_code = naui_string_from_cstr("US");
	settings->general.layout = naui_string_from_cstr("");
	settings->general.ui_scale = 1.0f;
	settings->general.autosave_idle_timer = 5;
	settings->general.undo_history_limit = 100;
	settings->general.confirm_on_exit = true;
	settings->general.confirm_on_delete = true;
	settings->general.copy_resources = false;
	settings->general.animations = true;

	settings->audio.output_device = (Uph_AudioIODevice){ 0 };
	settings->audio.input_device = (Uph_AudioIODevice){ 0 };
	settings->audio.driver = UPH_AUDIO_DRIVER_SYSTEM;
	settings->audio.asio_device = (Naui_String){ 0 };
	settings->audio.sample_rate = 48000;
	settings->audio.buffer_size = 512;
	settings->audio.channels = 2;
	settings->audio.input_channel = 0;
	settings->audio.monitor_volume_db = 0.0f;
	settings->audio.exclusive_mode = false;
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

	settings->browser.preview_volume = 0.8f;
	settings->browser.auto_preview = true;
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

	settings->browser.preview_volume = NAUI_CLAMP(settings->browser.preview_volume, 0.0f, 1.0f);

	if (settings->general.theme.length == 0)
		settings->general.theme = naui_string_from_cstr("Fish");

	if (settings->general.language_code.length == 0 || settings->general.region_code.length == 0)
	{
		settings->general.language_code = naui_string_from_cstr("en");
		settings->general.region_code = naui_string_from_cstr("US");
	}

	settings->general.ui_scale = NAUI_CLAMP(settings->general.ui_scale, UPH_SETTINGS_UI_SCALE_MIN, UPH_SETTINGS_UI_SCALE_MAX);
	settings->general.autosave_idle_timer = NAUI_CLAMP(settings->general.autosave_idle_timer, 0, UPH_SETTINGS_AUTOSAVE_MAX);
	settings->general.undo_history_limit = NAUI_CLAMP(settings->general.undo_history_limit, (uint32_t)UPH_SETTINGS_UNDO_MIN, (uint32_t)UPH_SETTINGS_UNDO_MAX);

	settings->audio.sample_rate = uph_settings_nearest(settings->audio.sample_rate, uph_settings_sample_rates, UPH_SETTINGS_COUNT(uph_settings_sample_rates));
	settings->audio.buffer_size = uph_settings_nearest(settings->audio.buffer_size, uph_settings_buffer_sizes, UPH_SETTINGS_COUNT(uph_settings_buffer_sizes));
	settings->audio.channels = uph_settings_nearest(settings->audio.channels, uph_settings_channel_counts, UPH_SETTINGS_COUNT(uph_settings_channel_counts));
	settings->audio.input_channel = NAUI_MIN(settings->audio.input_channel, 1u);
	settings->audio.monitor_volume_db = NAUI_CLAMP(settings->audio.monitor_volume_db, UPH_MIXER_DB_MIN, UPH_MIXER_DB_MAX);

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

static uint8_t uph_settings_hex_byte(const char *hex)
{
	uint8_t value = 0;

	for (int i = 0; i < 2; i++)
	{
		const char c = hex[i];
		uint8_t digit = 0;

		if (c >= '0' && c <= '9')
			digit = (uint8_t)(c - '0');
		else if (c >= 'a' && c <= 'f')
			digit = (uint8_t)(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			digit = (uint8_t)(c - 'A' + 10);

		value = (uint8_t)(value * 16 + digit);
	}

	return value;
}

static Leaf_Color uph_settings_parse_color(const char *hex, const Leaf_Color fallback)
{
	if (!hex)
		return fallback;

	if (*hex == '#')
		hex++;

	const size_t length = strlen(hex);
	if (length != 6 && length != 8)
		return fallback;

	return (Leaf_Color){
		uph_settings_hex_byte(hex),
		uph_settings_hex_byte(hex + 2),
		uph_settings_hex_byte(hex + 4),
		length == 8 ? uph_settings_hex_byte(hex + 6) : 255
	};
}

static Leaf_Color uph_settings_theme_key(const Naui_JsonValue *root, const char *key, const Leaf_Color fallback)
{
	return uph_settings_parse_color(naui_json_get_string(naui_json_object_get(root, key), NULL), fallback);
}

static void uph_settings_load_theme_preview(const Naui_Path path, Uph_SettingsThemePreview *preview)
{
	const Leaf_Color gray = leaf_rgb(40, 40, 40);
	const Leaf_Color light = leaf_rgb(200, 200, 200);

	Naui_Json json = naui_json_parse_file(path);
	const Naui_JsonValue *root = json.root;

	preview->viewport = uph_settings_theme_key(root, "naui_viewport_bg_color", gray);
	preview->toolbox = uph_settings_theme_key(root, "uph_song_timeline_toolbox_bg_color", gray);
	preview->border = uph_settings_theme_key(root, "uph_song_timeline_toolbox_border_color", gray);
	preview->header = uph_settings_theme_key(root, "uph_song_timeline_header_color", gray);
	preview->header_border = uph_settings_theme_key(root, "uph_song_timeline_header_border_color", gray);
	preview->row[0] = uph_settings_theme_key(root, "uph_song_timeline_row_bg1_color", gray);
	preview->row[1] = uph_settings_theme_key(root, "uph_song_timeline_row_bg2_color", gray);
	preview->logo = uph_settings_theme_key(root, "uph_logo_color", light);
	preview->text = uph_settings_theme_key(root, "naui_panel_text_color", light);
	preview->accent = uph_settings_theme_key(root, "uph_ui_slider_fill_color", light);

	const Naui_JsonValue *palette = naui_json_object_get(root, "uph_track_palette");
	const size_t palette_count = palette && palette->type == NAUI_JSON_ARRAY ? palette->array.count : 0;

	for (size_t i = 0; i < UPH_SETTINGS_THEME_ROWS; i++)
	{
		const char *hex = palette_count > 0 ? naui_json_get_string(naui_json_array_get(palette, i % palette_count), NULL) : NULL;
		preview->palette[i] = uph_settings_parse_color(hex, preview->accent);
	}

	naui_json_free(&json);
}

static void uph_settings_refresh_themes(Uph_SettingsState *data)
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

		uph_settings_load_theme_preview(entries[i].path, &data->theme_previews[data->themes.count]);
		data->themes.name[data->themes.count++] = naui_view_to_string(stem);
	}

	naui_directory_filter_free(entries);
}

static void uph_settings_refresh_languages(Uph_SettingsState *data)
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

static void uph_settings_refresh_audio_devices(Uph_SettingsState *data)
{
	data->audio_outputs.count = 0;
	data->audio_inputs.count = 0;
	data->asio_drivers.count = uph_asio_list_drivers(data->asio_drivers.name);

	ma_context temporary;
	ma_context *context = uph_audio_engine_get_context();
	const bool use_temporary = uph_audio_engine_active_driver() != UPH_AUDIO_DRIVER_SYSTEM;

	if (use_temporary)
		context = ma_context_init(NULL, 0, NULL, &temporary) == MA_SUCCESS ? &temporary : NULL;

	if (!context)
		return;

	ma_device_info *playback = NULL;
	ma_device_info *capture = NULL;
	ma_uint32 playback_count = 0;
	ma_uint32 capture_count = 0;

	if (ma_context_get_devices(context, &playback, &playback_count, &capture, &capture_count) == MA_SUCCESS)
	{
		for (ma_uint32 i = 0; i < playback_count && data->audio_outputs.count < UPH_SETTINGS_MAX_OPTIONS; i++)
			data->audio_outputs.name[data->audio_outputs.count++] = naui_string_from_cstr(playback[i].name);

		for (ma_uint32 i = 0; i < capture_count && data->audio_inputs.count < UPH_SETTINGS_MAX_OPTIONS; i++)
			data->audio_inputs.name[data->audio_inputs.count++] = naui_string_from_cstr(capture[i].name);
	}

	if (use_temporary)
		ma_context_uninit(&temporary);
}

static void uph_settings_refresh_midi_devices(Uph_SettingsState *data)
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

static void uph_settings_refresh_all(Uph_SettingsState *data)
{
	uph_settings_refresh_themes(data);
	uph_settings_refresh_languages(data);
	uph_settings_refresh_audio_devices(data);
	uph_settings_refresh_midi_devices(data);
	data->audio_pending = uph_state.settings.audio;
	data->lists_ready = true;
}

static bool uph_settings_audio_pending_changed(const Uph_AudioSettings *pending)
{
	const Uph_AudioSettings *audio = &uph_state.settings.audio;

	return pending->driver != audio->driver
		|| strcmp(pending->asio_device.data, audio->asio_device.data) != 0
		|| strcmp(pending->output_device.name.data, audio->output_device.name.data) != 0
		|| strcmp(pending->input_device.name.data, audio->input_device.name.data) != 0
		|| pending->exclusive_mode != audio->exclusive_mode
		|| pending->sample_rate != audio->sample_rate
		|| pending->buffer_size != audio->buffer_size
		|| pending->channels != audio->channels
		|| pending->input_channel != audio->input_channel;
}

static void uph_settings_audio_copy_device_fields(Uph_AudioSettings *dst, const Uph_AudioSettings *src)
{
	dst->output_device = src->output_device;
	dst->input_device = src->input_device;
	dst->driver = src->driver;
	dst->asio_device = src->asio_device;
	dst->exclusive_mode = src->exclusive_mode;
	dst->sample_rate = src->sample_rate;
	dst->buffer_size = src->buffer_size;
	dst->channels = src->channels;
	dst->input_channel = src->input_channel;
}

static void uph_settings_apply_audio(Uph_SettingsState *data)
{
	Uph_AudioSettings *audio = &uph_state.settings.audio;
	const Uph_AudioSettings previous = *audio;

	uph_audio_engine_stop();
	uph_settings_audio_copy_device_fields(audio, &data->audio_pending);

	if (!uph_audio_engine_recreate())
	{
		uph_settings_audio_copy_device_fields(audio, &previous);
		uph_audio_engine_recreate();
	}

	uph_settings_audio_copy_device_fields(&data->audio_pending, audio);
	uph_settings_refresh_audio_devices(data);
	uph_settings_save();
}

static void uph_settings_restart_deferred(void *unused)
{
	(void)unused;
	naui_app_relaunch();
}

static void uph_settings_restart_dialog_result(int32_t button, void *user_data)
{
	if (button != 0)
		return;

	Uph_SettingsState *data = (Uph_SettingsState*)user_data;
	const bool unsaved_changes = uph_autosave_has_unsaved_changes();
	const bool reopen_project = unsaved_changes || uph_project_has_been_saved(&uph_state.project);

	if (unsaved_changes && !uph_project_save(&uph_state.project, UPH_SAVE_TYPE_CANONICAL))
	{
		naui_log(NAUI_LOG_ERROR, "Save failed, not restarting");
		return;
	}

	uph_settings_audio_copy_device_fields(&uph_state.settings.audio, &data->audio_pending);
	uph_state.settings.general.reopen_project = reopen_project ? uph_project_get_path(&uph_state.project) : (Naui_Path){ 0 };
	uph_settings_save();

	naui_defer(uph_settings_restart_deferred, NULL, 0);
}

static void uph_settings_request_audio_apply(Uph_SettingsState *data)
{
	if (data->audio_pending.sample_rate == uph_state.settings.audio.sample_rate)
	{
		uph_settings_apply_audio(data);
		return;
	}

	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("dialog.restart.title"),
		.message = NAUI_TR("dialog.restart.message"),
		.buttons = { NAUI_TR("dialog.restart.confirm"), NAUI_TR("dialog.cancel") },
		.default_button = 0,
		.cancel_button = 1,
		.callback = uph_settings_restart_dialog_result,
		.user_data = data
	});
}

static void uph_settings_apply_theme_deferred(void *name)
{
	uph_theme_load((const char*)name);
}

static void uph_settings_apply_theme(const char *name)
{
	char buffer[UPH_SETTINGS_THEME_NAME_MAX];
	snprintf(buffer, sizeof(buffer), "%s", name);
	naui_defer(uph_settings_apply_theme_deferred, buffer, sizeof(buffer));
}

static void uph_settings_apply_ui_scale_deferred(void *value)
{
	naui_app_set_ui_scale(*(const float*)value);
}

static void uph_settings_apply_ui_scale(float value)
{
	naui_defer(uph_settings_apply_ui_scale_deferred, &value, sizeof(value));
}

static void uph_settings_apply_midi_thru(void)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;

	if (!midi->input)
		return;

	if (!midi->midi_thru_enabled)
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

// Opens the ports named in the settings. A non connected device is skipped and its name stays saved.
void uph_settings_open_midi_ports(const bool use_first_device)
{
	Uph_SettingsState *state = &_uph_settings_state;
	Uph_MIDISettings *midi = &uph_state.settings.midi;

	uph_settings_refresh_midi_devices(state);

	const cmidi_device_t *input = NULL;

	if (use_first_device)
	{
		input = state->midi_inputs.count > 0 ? &state->midi_inputs.device[0] : NULL;
	}
	else
	{
		input = uph_settings_find_midi_device(&state->midi_inputs, midi->input_device.data);
	}

	if (input)
		uph_settings_select_midi_input(input);
}

static void uph_settings_row_begin(const char *label, const Leaf_ID label_id)
{
	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);

	leaf_begin_element((Leaf_ElementConfig){
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_ROW_HEIGHT)) },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(12),
		.direction = LEAF_DIRECTION_HORIZONTAL
	});

	leaf_begin_element((Leaf_ElementConfig){
		.id = label_id,
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_LABEL_WIDTH)), .height = LEAF_SIZE_FULL },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.clip_children = true
	});
	{
		leaf_text(label, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(font_size) });
	}
	leaf_end_element();

	leaf_begin_element((Leaf_ElementConfig){
		.size = { .width = LEAF_SIZE_GROW_MAX(NAUI_DPI(UPH_SETTINGS_CONTROL_WIDTH)), .height = LEAF_SIZE_FULL },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.child_gap = NAUI_DPI(8),
		.direction = LEAF_DIRECTION_HORIZONTAL
	});
}

static void uph_settings_row_end(void)
{
	leaf_end_element();
	leaf_end_element();
}

static void uph_settings_hint(const char *text)
{
	leaf_text(text,
	{
		.color = uph_state.colors.panel_text,
		.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
	});
}

static bool uph_settings_section(const char *title, const bool first, const char *button_text, const Leaf_ID button_id)
{
	bool pressed = false;

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = { .left = 0, .right = 0, .top = first ? 0 : (int32_t)NAUI_DPI(14), .bottom = 0 },
		.child_gap = NAUI_DPI(6),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT } })
			{
				leaf_text(title, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE) * 1.1f) });
			}

			if (button_text)
				pressed = uph_ui_text_button(button_text, button_id);
		}

		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(1)) },
			.color = uph_state.colors.ui_frame_secondary_bg
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

static bool uph_settings_language_dropdown(Uph_SettingsState *data, const Leaf_ID id)
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
		uph_language_request_change();
		return true;
	}

	return false;
}

static bool uph_settings_midi_dropdown(Uph_SettingsState *data, const bool input, const Leaf_ID id)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;
	const Uph_SettingsMidiList *list = input ? &data->midi_inputs : &data->midi_outputs;
	Naui_String *unlisted = &data->midi_unlisted[input ? 0 : 1];
	cmidi_port_t *port = midi->input;
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

// static bool uph_settings_drag_u32(uint32_t *value, const Leaf_ID id, const float speed, const int32_t min, const int32_t max, const char *format)
// {
// 	static int32_t scratch;
// 	scratch = (int32_t)NAUI_CLAMP((int64_t)*value, (int64_t)min, (int64_t)max);

// 	if (!uph_ui_drag_int(&scratch, id, speed, min, max, format, UPH_UI_DRAG_FLAG_CLAMPED))
// 		return false;

// 	*value = (uint32_t)scratch;
// 	return true;
// }

// static bool uph_settings_drag_u8(uint8_t *value, const Leaf_ID id, const float speed, const int32_t min, const int32_t max, const char *format)
// {
// 	static int32_t scratch;
// 	scratch = NAUI_CLAMP((int32_t)*value, min, max);

// 	if (!uph_ui_drag_int(&scratch, id, speed, min, max, format, UPH_UI_DRAG_FLAG_CLAMPED))
// 		return false;

// 	*value = (uint8_t)scratch;
// 	return true;
// }

// static bool uph_settings_slider_u8(uint8_t *value, const Leaf_ID id, const int32_t min, const int32_t max, const char *format)
// {
// 	static int32_t scratch;
// 	scratch = NAUI_CLAMP((int32_t)*value, min, max);

// 	if (!uph_ui_slider_int(&scratch, id, min, max, format, UPH_UI_SLIDER_FLAG_NONE))
// 		return false;

// 	*value = (uint8_t)scratch;
// 	return true;
// }

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

static void uph_settings_page_general(Uph_SettingsState *data)
{
	Uph_GeneralSettings *general = &uph_state.settings.general;

	uph_settings_section(NAUI_TR("settings.general.appearance"), true, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.language"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_language_dropdown(data, leaf_id("uph_settings_language"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.ui_scale"), UPH_SETTINGS_NO_ID)
	{
		static const float scales[] = { 0.5f, 0.75f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 1.75f, 2.0f };
		static const char *labels[] = { "50%", "75%", "90%", "100%", "110%", "125%", "150%", "175%", "200%" };
		const uint32_t count = sizeof(scales) / sizeof(scales[0]);

		uint32_t selected = 0;
		for (uint32_t i = 1; i < count; i++)
		{
			if (fabsf(scales[i] - general->ui_scale) < fabsf(scales[selected] - general->ui_scale))
				selected = i;
		}

		if (uph_ui_dropdown(labels, count, &selected, leaf_id("uph_settings_ui_scale")))
			general->ui_scale = scales[selected];

		if (general->ui_scale != naui_app_get_ui_scale())
			uph_settings_apply_ui_scale(general->ui_scale);
	}

	if (uph_settings_checkbox_row(NAUI_TR("settings.general.animations"), &general->animations, leaf_id("uph_settings_animations")))
		naui_app_set_animations_enabled(general->animations);

	uph_settings_section(NAUI_TR("settings.general.behavior"), false, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.general.autosave"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_drag_int(&general->autosave_idle_timer, leaf_id("uph_settings_autosave"), 0.25f, 0, UPH_SETTINGS_AUTOSAVE_MAX, NAUI_TR("settings.general.autosave.format"), UPH_UI_DRAG_FLAG_CLAMPED);
		uph_settings_hint(NAUI_TR("settings.general.autosave_hint"));
	}

	// UPH_SETTINGS_ROW(NAUI_TR("settings.general.undo_limit"), UPH_SETTINGS_NO_ID)
	// {
	// 	uph_settings_drag_u32(&general->undo_history_limit, leaf_id("uph_settings_undo_limit"), 1.0f, UPH_SETTINGS_UNDO_MIN, UPH_SETTINGS_UNDO_MAX, "%d steps");
	// }

	uph_settings_checkbox_row(NAUI_TR("settings.general.confirm_on_exit"), &general->confirm_on_exit, leaf_id("uph_settings_confirm_exit"));
	// uph_settings_checkbox_row(NAUI_TR("settings.general.confirm_on_delete"), &general->confirm_on_delete, leaf_id("uph_settings_confirm_delete"));
	uph_settings_checkbox_row(NAUI_TR("settings.general.copy_resources"), &general->copy_resources, leaf_id("uph_settings_copy_resources"));
}

static void uph_settings_theme_dot(const Leaf_Color color, const float size)
{
	leaf({
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(size)), .height = LEAF_SIZE_FIXED(NAUI_DPI(size)) },
		.color = color,
		.rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
	});
}

static void uph_settings_theme_thumbnail(const Uph_SettingsThemePreview *preview)
{
	static const float block_offsets[UPH_SETTINGS_THEME_ROWS] = { 8.0f, 30.0f, 16.0f };
	static const float block_widths[UPH_SETTINGS_THEME_ROWS] = { 48.0f, 40.0f, 60.0f };

	const int32_t rounding = NAUI_DPI(UPH_UI_FRAME_ROUNDING);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_THEME_CARD_PREVIEW)) },
		.color = preview->viewport,
		.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL),
		.direction = LEAF_DIRECTION_VERTICAL,
		.clip_children = true
	})
	{
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(16)) },
			.padding = LEAF_PADDING_AXES(NAUI_DPI(7), 0),
			.border = { .width = 1, .sides = LEAF_SIDE_BOTTOM, .color = preview->border },
			.color = preview->toolbox,
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			uph_settings_theme_dot(preview->logo, 6.0f);
			leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT } });
			leaf({
				.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(16)), .height = LEAF_SIZE_FIXED(NAUI_DPI(3)) },
				.color = preview->text,
				.rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
			});
		}

		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			leaf({
				.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_THEME_HEADER_WIDTH)), .height = LEAF_SIZE_FULL },
				.border = { .width = 1, .sides = LEAF_SIDE_RIGHT, .color = preview->header_border },
				.color = preview->header,
				.direction = LEAF_DIRECTION_VERTICAL
			})
			{
				for (uint32_t r = 0; r < UPH_SETTINGS_THEME_ROWS; r++)
				{
					leaf({
						.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
						.padding = { .left = (int32_t)NAUI_DPI(6), .right = 0, .top = 0, .bottom = 0 },
						.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER }
					})
					{
						leaf({
							.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(3)), .height = LEAF_SIZE_PERCENT(0.5f) },
							.color = preview->palette[r],
							.rounding = LEAF_ROUNDING_FULL(LEAF_CORNER_ALL)
						});
					}
				}
			}

			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL },
				.direction = LEAF_DIRECTION_VERTICAL
			})
			{
				for (uint32_t r = 0; r < UPH_SETTINGS_THEME_ROWS; r++)
				{
					leaf({
						.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_GROW },
						.padding = { .left = (int32_t)NAUI_DPI(block_offsets[r]), .right = 0, .top = 0, .bottom = 0 },
						.color = preview->row[r % 2],
						.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
						.direction = LEAF_DIRECTION_HORIZONTAL
					})
					{
						leaf({
							.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(block_widths[r])), .height = LEAF_SIZE_PERCENT(0.6f) },
							.color = preview->palette[r],
							.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL)
						});
					}
				}
			}
		}
	}
}

static void uph_settings_theme_card(Uph_SettingsState *data, const uint32_t index)
{
	Uph_GeneralSettings *general = &uph_state.settings.general;
	const Naui_String *name = &data->themes.name[index];
	const Leaf_ID id = leaf_id_indexed("uph_settings_theme_card", index);
	const bool selected = strcmp(name->data, general->theme.data) == 0;
	const bool hovered = uph_ui_widget_hovered(id);

	if (hovered)
	{
		naui_set_cursor(NAUI_CURSOR_HAND);
		if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && !selected)
		{
			general->theme = *name;
			uph_settings_apply_theme(general->theme.data);
		}
	}

	const int32_t rounding = NAUI_DPI(UPH_UI_FRAME_ROUNDING);
	Leaf_Color border = uph_state.colors.ui_frame_border;
	if (selected)
		border = uph_state.colors.ui_slider_fill;
	else if (hovered)
		border = uph_state.colors.ui_frame_pressed_bg;

	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_THEME_CARD_WIDTH)), .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(4)),
		.border = { .width = (int32_t)NAUI_DPI(selected ? 2 : 1), .sides = LEAF_SIDE_ALL, .color = border },
		.color = hovered || selected ? uph_state.colors.ui_frame_hovered_bg : uph_state.colors.ui_frame_secondary_bg,
		.rounding = LEAF_ROUNDING_FIXED(rounding + NAUI_DPI(2), LEAF_CORNER_ALL),
		.child_gap = NAUI_DPI(4),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		uph_settings_theme_thumbnail(&data->theme_previews[index]);

		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(24)) },
			.padding = LEAF_PADDING_AXES(NAUI_DPI(4), 0),
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(6),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			uph_settings_theme_dot(data->theme_previews[index].accent, 8.0f);

			leaf({ .size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT }, .clip_children = true })
			{
				leaf_text(name->data, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE)) });
			}
		}
	}
}

static void uph_settings_page_themes(Uph_SettingsState *data)
{
	uph_settings_section(NAUI_TR("settings.general.theme"), true, NULL, UPH_SETTINGS_NO_ID);

	Uph_UIScrollContainer container = uph_ui_begin_scroll_container(
		UPH_UI_SCROLL_DIRECTION_VERTICAL,
		&data->themes_scroll,
		leaf_id("uph_settings_themes_scrollbar")
	);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(4)),
		.child_gap = NAUI_DPI(12),
		.child_cross_gap = NAUI_DPI(12),
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.wrap_children = true
	})
	{
		for (uint32_t i = 0; i < data->themes.count; i++)
			uph_settings_theme_card(data, i);
	}

	uph_ui_end_scroll_container(&container);
}

static void uph_settings_page_audio(Uph_SettingsState *data)
{
	Uph_AudioSettings *audio = &uph_state.settings.audio;
	Uph_AudioSettings *pending = &data->audio_pending;

	if (uph_settings_section(NAUI_TR("settings.audio.devices"), true, NAUI_TR("settings.refresh"), leaf_id("uph_settings_audio_refresh")))
		uph_settings_refresh_audio_devices(data);

	if (uph_asio_supported())
	{
		UPH_SETTINGS_ROW(NAUI_TR("settings.audio.driver"), UPH_SETTINGS_NO_ID)
		{
			const uint32_t values[] = { UPH_AUDIO_DRIVER_SYSTEM, UPH_AUDIO_DRIVER_ASIO };
			const char *const labels[] = { NAUI_TR("settings.audio.driver.system"), NAUI_TR("settings.audio.driver.asio") };
			uint32_t driver = (uint32_t)pending->driver;

			if (uph_settings_value_dropdown(&driver, values, labels, UPH_SETTINGS_COUNT(values), leaf_id("uph_settings_audio_driver")))
				pending->driver = (Uph_AudioDriver)driver;
		}
	}

	if (pending->driver == UPH_AUDIO_DRIVER_ASIO)
	{
		UPH_SETTINGS_ROW(NAUI_TR("settings.audio.asio_device"), UPH_SETTINGS_NO_ID)
		{
			if (data->asio_drivers.count == 0)
				uph_settings_hint(NAUI_TR("settings.audio.asio_none"));
			else
				uph_settings_name_dropdown(&pending->asio_device, &data->asio_drivers, NULL, leaf_id("uph_settings_asio_device"));
		}

		if (audio->driver == UPH_AUDIO_DRIVER_ASIO && uph_audio_engine_active_driver() == UPH_AUDIO_DRIVER_ASIO)
		{
			UPH_SETTINGS_ROW("", UPH_SETTINGS_NO_ID)
			{
				if (uph_ui_text_button(NAUI_TR("settings.audio.asio_control_panel"), leaf_id("uph_settings_asio_panel")))
					uph_asio_show_control_panel();
			}
		}
	}
	else
	{
		UPH_SETTINGS_ROW(NAUI_TR("settings.audio.output_device"), UPH_SETTINGS_NO_ID)
		{
			uph_settings_name_dropdown(&pending->output_device.name, &data->audio_outputs, NAUI_TR("settings.audio.default_device"), leaf_id("uph_settings_audio_output"));
		}

		UPH_SETTINGS_ROW(NAUI_TR("settings.audio.input_device"), UPH_SETTINGS_NO_ID)
		{
			uph_settings_name_dropdown(&pending->input_device.name, &data->audio_inputs, NAUI_TR("settings.audio.default_device"), leaf_id("uph_settings_audio_input"));
		}

		uph_settings_checkbox_row(NAUI_TR("settings.audio.exclusive_mode"), &pending->exclusive_mode, leaf_id("uph_settings_exclusive"));
	}

	if (uph_audio_engine_input_channel_count() > 1)
	{
		UPH_SETTINGS_ROW(NAUI_TR("settings.audio.input_channel"), UPH_SETTINGS_NO_ID)
		{
			uph_settings_value_dropdown(&pending->input_channel, uph_settings_input_channels, uph_settings_input_channel_labels,
				UPH_SETTINGS_COUNT(uph_settings_input_channels), leaf_id("uph_settings_input_channel"));
		}
	}
	// uph_settings_checkbox_row(NAUI_TR("settings.audio.auto_restart"), &audio->auto_restart_on_device_change, leaf_id("uph_settings_auto_restart"));

	uph_settings_section(NAUI_TR("settings.audio.monitoring"), false, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.monitor_volume"), UPH_SETTINGS_NO_ID)
	{
		uph_ui_drag_float(&audio->monitor_volume_db, leaf_id("uph_settings_monitor_volume"), 0.1f, UPH_MIXER_DB_MIN, UPH_MIXER_DB_MAX, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED);
		uph_settings_hint("dB");
	}

	uph_settings_section(NAUI_TR("settings.audio.performance"), false, NULL, UPH_SETTINGS_NO_ID);

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.sample_rate"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&pending->sample_rate, uph_settings_sample_rates, uph_settings_sample_rate_labels,
			UPH_SETTINGS_COUNT(uph_settings_sample_rates), leaf_id("uph_settings_sample_rate"));
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.buffer_size"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&pending->buffer_size, uph_settings_buffer_sizes, uph_settings_buffer_size_labels,
			UPH_SETTINGS_COUNT(uph_settings_buffer_sizes), leaf_id("uph_settings_buffer_size"));

		if (pending->sample_rate > 0)
		{
			const Naui_String latency = naui_string_format("%.1f ms", (float)pending->buffer_size * 1000.0f / (float)pending->sample_rate);
			uph_settings_hint(latency.data);
		}
	}

	UPH_SETTINGS_ROW(NAUI_TR("settings.audio.channels"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&pending->channels, uph_settings_channel_counts, uph_settings_channel_labels,
			UPH_SETTINGS_COUNT(uph_settings_channel_counts), leaf_id("uph_settings_channels"));
	}

	// uph_settings_checkbox_row(NAUI_TR("settings.audio.underrun_protection"), &audio->underrun_protection, leaf_id("uph_settings_underrun"));

	if (!uph_settings_audio_pending_changed(pending))
		return;

	bool apply = false;

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = { .left = 0, .right = 0, .top = (int32_t)NAUI_DPI(4), .bottom = 0 },
		.child_alignment = { .x = LEAF_ALIGN_X_RIGHT, .y = LEAF_ALIGN_Y_CENTER },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		apply = uph_ui_text_button(NAUI_TR("settings.audio.apply"), leaf_id("uph_settings_audio_apply"));
	}

	if (apply)
		uph_settings_request_audio_apply(data);
}

// static void uph_settings_axis_pair(Naui_Vec2 *value, const float speed, const float min, const float max, const char *id_x, const char *id_y)
// {
// 	const Leaf_Color axis_color = uph_state.colors.panel_text;
// 	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);

// 	leaf_text("X", { .color = axis_color, .font_size = LEAF_SIZE_FIXED(font_size) });
// 	uph_ui_drag_float(&value->x, leaf_id(id_x), speed, min, max, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED);
// 	leaf_text("Y", { .color = axis_color, .font_size = LEAF_SIZE_FIXED(font_size) });
// 	uph_ui_drag_float(&value->y, leaf_id(id_y), speed, min, max, "%.2f", UPH_UI_DRAG_FLAG_CLAMPED);
// }

// static void uph_settings_page_interface(Uph_SettingsState *data)
// {
// 	(void)data;
// 	Uph_UISettings *ui = &uph_state.settings.ui;

// 	uph_settings_section(NAUI_TR("settings.ui.navigation"), true, NULL, UPH_SETTINGS_NO_ID);

// 	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.scroll_sensitivity"), UPH_SETTINGS_NO_ID)
// 	{
// 		uph_settings_axis_pair(&ui->scroll_sensitivity, 0.01f, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX, "uph_settings_scroll_x", "uph_settings_scroll_y");
// 	}

// 	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.zoom_sensitivity"), UPH_SETTINGS_NO_ID)
// 	{
// 		uph_settings_axis_pair(&ui->zoom_sensitivity, 0.01f, UPH_SETTINGS_SENSITIVITY_MIN, UPH_SETTINGS_SENSITIVITY_MAX, "uph_settings_zoom_x", "uph_settings_zoom_y");
// 	}

// 	uph_settings_section(NAUI_TR("settings.ui.editing"), false, NULL, UPH_SETTINGS_NO_ID);

// 	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.default_snap"), UPH_SETTINGS_NO_ID)
// 	{
// 		uph_settings_value_dropdown(&ui->default_snap, uph_settings_snap_values, uph_settings_snap_labels,
// 			UPH_SETTINGS_COUNT(uph_settings_snap_values), leaf_id("uph_settings_default_snap"));
// 	}

// 	uph_settings_checkbox_row(NAUI_TR("settings.ui.follow_playhead"), &ui->follow_playhead, leaf_id("uph_settings_follow_playhead"));

// 	UPH_SETTINGS_ROW(NAUI_TR("settings.ui.playhead_lock"), UPH_SETTINGS_NO_ID)
// 	{
// 		uph_ui_slider_float(&ui->playhead_lock_position, leaf_id("uph_settings_playhead_lock"), 0.0f, 1.0f, "%.2f", UPH_UI_SLIDER_FLAG_NONE);
// 	}

// 	uph_settings_checkbox_row(NAUI_TR("settings.ui.show_tooltips"), &ui->show_tooltips, leaf_id("uph_settings_tooltips"));
// }

static void uph_settings_page_midi(Uph_SettingsState *data)
{
	Uph_MIDISettings *midi = &uph_state.settings.midi;

	if (uph_settings_section(NAUI_TR("settings.midi.devices"), true, NAUI_TR("settings.refresh"), leaf_id("uph_settings_midi_refresh")))
		uph_settings_refresh_midi_devices(data);

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.input"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_midi_dropdown(data, true, leaf_id("uph_settings_midi_input"));
	}

	// UPH_SETTINGS_ROW(NAUI_TR("settings.midi.output"), UPH_SETTINGS_NO_ID)
	// {
	// 	uph_settings_midi_dropdown(data, false, leaf_id("uph_settings_midi_output"));
	// }

	// if (uph_settings_checkbox_row(NAUI_TR("settings.midi.thru"), &midi->midi_thru_enabled, leaf_id("uph_settings_midi_thru")))
	// 	uph_settings_apply_midi_thru();

	uph_settings_section(NAUI_TR("settings.midi.recording"), false, NULL, UPH_SETTINGS_NO_ID);

	uph_settings_checkbox_row(NAUI_TR("settings.midi.record_quantize"), &midi->record_quantize, leaf_id("uph_settings_record_quantize"));

	UPH_SETTINGS_ROW(NAUI_TR("settings.midi.quantize_grid"), UPH_SETTINGS_NO_ID)
	{
		uph_settings_value_dropdown(&midi->record_quantize_grid, uph_settings_snap_values, uph_settings_snap_labels,
			UPH_SETTINGS_COUNT(uph_settings_snap_values), leaf_id("uph_settings_quantize_grid"));
	}

	// UPH_SETTINGS_ROW(NAUI_TR("settings.midi.count_in"), UPH_SETTINGS_NO_ID)
	// {
	// 	uph_settings_drag_u32(&midi->count_in_bars, leaf_id("uph_settings_count_in"), 0.1f, 0, UPH_SETTINGS_COUNT_IN_MAX, "%d bars");
	// }

	// uph_settings_checkbox_row(NAUI_TR("settings.midi.overdub"), &midi->record_over_dub, leaf_id("uph_settings_overdub"));

	// uph_settings_section(NAUI_TR("settings.midi.editing"), false, NULL, UPH_SETTINGS_NO_ID);

	// UPH_SETTINGS_ROW(NAUI_TR("settings.midi.grid_division"), UPH_SETTINGS_NO_ID)
	// {
	// 	uph_settings_value_dropdown(&midi->grid_division, uph_settings_snap_values, uph_settings_snap_labels,
	// 		UPH_SETTINGS_COUNT(uph_settings_snap_values), leaf_id("uph_settings_grid_division"));
	// }

	// UPH_SETTINGS_ROW(NAUI_TR("settings.midi.default_velocity"), UPH_SETTINGS_NO_ID)
	// {
	// 	uph_settings_slider_u8(&midi->default_velocity, leaf_id("uph_settings_default_velocity"), UPH_SETTINGS_VELOCITY_MIN, UPH_SETTINGS_VELOCITY_MAX, "%d");
	// }

	// UPH_SETTINGS_ROW(NAUI_TR("settings.midi.default_channel"), UPH_SETTINGS_NO_ID)
	// {
	// 	uph_settings_drag_u8(&midi->default_channel, leaf_id("uph_settings_default_channel"), 0.1f, UPH_SETTINGS_CHANNEL_MIN, UPH_SETTINGS_CHANNEL_MAX, "%d");
	// }

	// uph_settings_checkbox_row(NAUI_TR("settings.midi.filter_channel"), &midi->filter_by_channel, leaf_id("uph_settings_filter_channel"));
}

#define UPH_SETTINGS_PLUGIN_DIALOG_KEY "uph_settings_plugin_folder"

static bool uph_settings_add_plugin_path(const Naui_Path folder)
{
	Uph_PluginSettings *plugin = &uph_state.settings.plugin;

	const Naui_Path path = naui_path_normalize(folder);
	if (!naui_path_is_directory(path))
	{
		naui_log(NAUI_LOG_WARNING, "Not adding plugin path, it isn't a folder: %s", path.data);
		return false;
	}

	for (int32_t i = 0; i < (int32_t)naui_list_len(plugin->plugin_paths); i++)
	{
		if (strcmp(naui_path_normalize(plugin->plugin_paths[i]).data, path.data) == 0)
			return false;
	}

	naui_list_push(plugin->plugin_paths, path);
	return true;
}

static void uph_settings_plugin_folder_picked(const Naui_DialogResult *result, void *user_data, bool is_cancelled)
{
	(void)user_data;
	if (is_cancelled || result->count == 0)
		return;

	uph_settings_add_plugin_path(result->paths[0]);
}

static void uph_settings_open_plugin_folder_dialog(void)
{
	if (!naui_dialog_open_folder(UPH_SETTINGS_PLUGIN_DIALOG_KEY, NULL, uph_settings_plugin_folder_picked, NULL))
		return;

	naui_dialog_title_set(UPH_SETTINGS_PLUGIN_DIALOG_KEY, NAUI_TR("settings.plugins.browse_title"));
}

static void uph_settings_page_plugins(Uph_SettingsState *data)
{
	(void)data;
	Uph_PluginSettings *plugin = &uph_state.settings.plugin;
	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);

	uph_settings_section(NAUI_TR("settings.plugins.paths"), true, NULL, UPH_SETTINGS_NO_ID);

	const int32_t path_count = (int32_t)naui_list_len(plugin->plugin_paths);
	int32_t remove_index = -1;

	for (int32_t i = 0; i < path_count; i++)
	{
		leaf({
			.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
			.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = NAUI_DPI(8),
			.direction = LEAF_DIRECTION_HORIZONTAL
		})
		{
			leaf({
				.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
				.padding = LEAF_PADDING_AXES(NAUI_DPI(8), NAUI_DPI(6)),
				.color = uph_state.colors.ui_frame_bg,
				.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
				.clip_children = true
			})
			{
				leaf_text(plugin->plugin_paths[i].data, { .color = uph_state.colors.ui_text, .font_size = LEAF_SIZE_FIXED(font_size) });
			}

			if (uph_ui_text_button(NAUI_TR("settings.plugins.remove"), leaf_id_indexed("uph_settings_plugin_remove", i)))
				remove_index = i;
		}
	}

	if (path_count == 0)
		uph_settings_hint(NAUI_TR("settings.plugins.no_paths"));

	if (remove_index >= 0)
		naui_list_remove(plugin->plugin_paths, remove_index);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = { .left = 0, .right = 0, .top = (int32_t)NAUI_DPI(4), .bottom = 0 },
		.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		if (uph_ui_text_button(NAUI_TR("settings.plugins.add"), leaf_id("uph_settings_plugin_add")))
			uph_settings_open_plugin_folder_dialog();
	}

	// uph_settings_section(NAUI_TR("settings.plugins.options"), false, NULL, UPH_SETTINGS_NO_ID);
	// uph_settings_checkbox_row(NAUI_TR("settings.plugins.sandbox"), &plugin->sandbox_plugins, leaf_id("uph_settings_sandbox"));
}

static void uph_settings_cancel_keybind_capture(Uph_SettingsState *data)
{
	if (!data->capturing_keybind)
		return;

	data->capturing_keybind = NULL;
	naui_keybind_capture_cancel();
}

static bool uph_settings_row_button(const char *text, const Leaf_ID id, const bool enabled)
{
	if (enabled)
		return uph_ui_text_button(text, id);

	const float frame_height = _uph_ui_frame_height();
	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIT_MIN(frame_height), .height = UPH_UI_SIZE_EXACT(frame_height) },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_UI_FRAME_PADDING.x), 0),
		.border = _uph_ui_frame_border(),
		.color = uph_state.colors.ui_frame_bg,
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.transparency = 0.6f,
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL)
	})
	{
		leaf_text(text,
		{
			.color = uph_state.colors.ui_text,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
		});
	}

	return false;
}

static void uph_settings_keycap(const char *name)
{
	const float inset = NAUI_DPI(UPH_SETTINGS_KEYCAP_INSET);
	leaf({
		.size = { .width = LEAF_SIZE_FIT, .height = UPH_UI_SIZE_EXACT(_uph_ui_frame_height() - inset * 2.0f) },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(UPH_SETTINGS_KEYCAP_PADDING), 0),
		.border = _uph_ui_frame_border(),
		.color = uph_state.colors.ui_well_bg,
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING) * 0.75f, NAUI_CORNER_ALL)
	})
	{
		leaf_text(name,
		{
			.color = uph_state.colors.ui_text,
			.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_KEYCAP_FONT_SIZE))
		});
	}
}

static bool uph_settings_chord_button(const Uph_KeybindInfo *info, const bool capturing, const Leaf_ID id)
{
	const Naui_Keybind progress = naui_keybind_capture_progress();
	const bool recording = capturing && naui_keybind_valid(progress);
	const char *names[NAUI_KEYBIND_MAX_NAMES];
	const uint32_t name_count = naui_keybind_names(recording ? progress : *info->chord, names);

	const char *label = NULL;
	if (capturing && !recording)
		label = NAUI_TR("settings.shortcuts.press_keys");
	else if (name_count == 0)
		label = NAUI_TR("settings.shortcuts.unbound");

	const bool hovered = uph_ui_widget_hovered(id);
	if (hovered)
		naui_set_cursor(NAUI_CURSOR_HAND);

	Leaf_Color color = uph_state.colors.ui_frame_bg;
	if (capturing)
		color = uph_state.colors.ui_frame_toggled_bg;
	else if (hovered)
		color = uph_state.colors.ui_frame_hovered_bg;

	const float frame_height = _uph_ui_frame_height();
	leaf({
		.id = id,
		.size = { .width = LEAF_SIZE_FIT_MIN(NAUI_DPI(UPH_SETTINGS_CHORD_WIDTH)), .height = UPH_UI_SIZE_EXACT(frame_height) },
		.padding = LEAF_PADDING_AXES(NAUI_DPI(label ? UPH_UI_FRAME_PADDING.x : UPH_SETTINGS_KEYCAP_INSET), 0),
		.border = _uph_ui_frame_border(),
		.color = color,
		.child_gap = NAUI_DPI(UPH_SETTINGS_KEYCAP_INSET),
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
		.direction = LEAF_DIRECTION_HORIZONTAL,
		.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL),
		.clip_children = true
	})
	{
		if (label)
		{
			const bool dim = !capturing;
			leaf_text(label,
			{
				.color = dim ? uph_state.colors.panel_text : uph_state.colors.ui_text,
				.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
			});
		}
		else
		{
			for (uint32_t i = 0; i < name_count; i++)
				uph_settings_keycap(names[i]);
		}
	}

	return hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT);
}

static void uph_settings_page_shortcuts(Uph_SettingsState *data)
{
	if (data->capturing_keybind)
	{
		Naui_Keybind chord;
		if (naui_keybind_capture(&chord))
		{
			*data->capturing_keybind->chord = chord;
			data->capturing_keybind = NULL;
		}
		else if (!naui_keybind_capturing())
			data->capturing_keybind = NULL;
	}

	uint32_t count = 0;
	const Uph_KeybindInfo *infos = uph_keybind_infos(&count);
	bool capture_button_clicked = false;

	Uph_UIScrollContainer container = uph_ui_begin_scroll_container(
		UPH_UI_SCROLL_DIRECTION_VERTICAL,
		&data->shortcuts_scroll,
		leaf_id("uph_settings_shortcuts_scrollbar")
	);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIT },
		.padding = { .left = 0, .right = (int32_t)NAUI_DPI(8), .top = NAUI_MAX(1, (int32_t)NAUI_DPI(1)), .bottom = 0 },
		.child_gap = NAUI_DPI(4),
		.direction = LEAF_DIRECTION_VERTICAL
	})
	{
		const char *group = NULL;
		for (uint32_t i = 0; i < count; i++)
		{
			const Uph_KeybindInfo *info = &infos[i];
			if (!group || strcmp(group, info->group_key) != 0)
			{
				const bool first = group == NULL;
				group = info->group_key;
				if (uph_settings_section(NAUI_TR(group), first, first ? NAUI_TR("settings.shortcuts.reset_all") : NULL, leaf_id("uph_settings_shortcuts_reset_all")))
				{
					uph_settings_cancel_keybind_capture(data);
					uph_keybinds_set_defaults();
				}
			}

			const bool capturing = data->capturing_keybind == info;
			UPH_SETTINGS_ROW(NAUI_TR(info->label_key), UPH_SETTINGS_NO_ID)
			{
				if (uph_settings_chord_button(info, capturing, leaf_id_indexed("uph_settings_keybind", i)))
				{
					capture_button_clicked = true;
					if (capturing)
						uph_settings_cancel_keybind_capture(data);
					else
					{
						data->capturing_keybind = info;
						naui_keybind_capture_begin();
					}
				}

				const bool modified = !naui_keybind_equal(*info->chord, info->default_chord);
				const bool enabled = modified || naui_keybind_valid(*info->chord);
				const char *action = NAUI_TR(modified ? "settings.shortcuts.reset" : "settings.shortcuts.clear");

				if (uph_settings_row_button(action, leaf_id_indexed("uph_settings_keybind_action", i), enabled))
				{
					uph_settings_cancel_keybind_capture(data);
					if (modified)
						uph_keybind_reset(info);
					else
						*info->chord = NAUI_KEYBIND_NONE;
				}

				const Uph_KeybindInfo *conflict = uph_keybind_conflict(info);
				if (conflict && !capturing)
				{
					char hint[128];
					snprintf(hint, sizeof(hint), NAUI_TR("settings.shortcuts.conflict"), NAUI_TR(conflict->label_key));
					leaf_text(hint,
					{
						.color = uph_state.colors.ui_slider_fill,
						.font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE))
					});
				}
			}
		}
	}

	uph_ui_end_scroll_container(&container);

	if (data->capturing_keybind && !capture_button_clicked && naui_any_mouse_pressed())
		uph_settings_cancel_keybind_capture(data);
}

static void uph_settings_sidebar(Uph_SettingsState *data)
{
	static const char *const keys[UPH_SETTINGS_PAGE_COUNT] =
	{
		"settings.page.general",
		"settings.page.shortcuts",
		"settings.page.themes",
		"settings.page.audio",
		// "settings.page.interface",
		"settings.page.midi",
		"settings.page.plugins"
	};

	const float font_size = NAUI_DPI(UPH_UI_FONT_SIZE);
	const int32_t rounding = NAUI_DPI(UPH_UI_FRAME_ROUNDING);

	leaf({
		.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(UPH_SETTINGS_SIDEBAR_WIDTH)), .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_ALL(NAUI_DPI(8)),
		.child_gap = NAUI_DPI(2),
		.direction = LEAF_DIRECTION_VERTICAL
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
				background = uph_state.colors.ui_frame_toggled_bg;
			else if (hovered)
				background = uph_state.colors.ui_frame_secondary_bg;

			leaf({
				.id = id,
				.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FIXED(NAUI_DPI(30)) },
				.color = background,
				.child_alignment = { .x = LEAF_ALIGN_X_LEFT, .y = LEAF_ALIGN_Y_CENTER },
				.rounding = LEAF_ROUNDING_FIXED(rounding, NAUI_CORNER_ALL),
				.direction = LEAF_DIRECTION_HORIZONTAL
			})
			{
				leaf({
					.size = { .width = LEAF_SIZE_FIXED(NAUI_DPI(3)), .height = LEAF_SIZE_PERCENT(0.6f) },
					.color = selected ? uph_state.colors.ui_slider_fill : LEAF_COLOR_TRANSPARENT,
					.rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), NAUI_CORNER_ALL)
				});

				leaf({
					.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
					.padding = LEAF_PADDING_AXES(NAUI_DPI(10), 0),
					.clip_children = true
				})
				{
					leaf_text(NAUI_TR(keys[i]),
					{
						.color = uph_state.colors.ui_text,
						.font_size = LEAF_SIZE_FIXED(font_size)
					});
				}
			}
		}
	}
}

static void uph_settings_on_attach(void)
{
	const Naui_PanelID this = naui_panel_current();

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
}

static void uph_settings_on_detach(void)
{

}

static void uph_settings_on_open(void)
{
	Uph_SettingsState *state = &_uph_settings_state;
	uph_settings_refresh_all(state);
	state->was_opened = true;
}

static void uph_settings_on_close(void)
{
	Uph_SettingsState *state = &_uph_settings_state;
	uph_settings_cancel_keybind_capture(state);

	if (state->was_opened)
		uph_settings_save();
}

static void uph_settings_on_update(void)
{
	Uph_SettingsState *state = &_uph_settings_state;
	if (!state->lists_ready)
		uph_settings_refresh_all(state);

	if (state->page != UPH_SETTINGS_PAGE_SHORTCUTS)
		uph_settings_cancel_keybind_capture(state);

	leaf({
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.direction = LEAF_DIRECTION_HORIZONTAL
	})
	{
		uph_settings_sidebar(state);

		leaf({
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FULL },
			.padding = LEAF_PADDING_AXES(NAUI_DPI(24), NAUI_DPI(16)),
			.color = uph_state.colors.panel_title_bg,
			.child_gap = NAUI_DPI(4),
			.direction = LEAF_DIRECTION_VERTICAL,
			.clip_children = true
		})
		{
			switch (state->page)
			{
				case UPH_SETTINGS_PAGE_GENERAL:
					uph_settings_page_general(state);
					break;

				case UPH_SETTINGS_PAGE_THEMES:
					uph_settings_page_themes(state);
					break;

				case UPH_SETTINGS_PAGE_AUDIO:
					uph_settings_page_audio(state);
					break;

				// case UPH_SETTINGS_PAGE_INTERFACE:
				// 	uph_settings_page_interface(state);
				// 	break;

				case UPH_SETTINGS_PAGE_MIDI:
					uph_settings_page_midi(state);
					break;

				case UPH_SETTINGS_PAGE_PLUGINS:
					uph_settings_page_plugins(state);
					break;

				case UPH_SETTINGS_PAGE_SHORTCUTS:
					uph_settings_page_shortcuts(state);
					break;

				default: break;
			}
		}
	}
}
