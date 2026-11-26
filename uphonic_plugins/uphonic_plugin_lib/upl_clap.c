#define UPL_STATE_MAGIC 0x55504C31u

typedef struct
{
	clap_plugin_descriptor_t descriptor;
	uint32_t init_count;
}
Upl_ModuleState;

static Upl_ModuleState upl__module_state;

static const char *const upl__empty_features[] = { NULL };

#pragma region Params

void *upl_user_data(Upl_Plugin *plugin)
{
	return plugin ? plugin->user_data : NULL;
}

double upl_param(Upl_Plugin *plugin, uint32_t index)
{
	if (!plugin || index >= upl_descriptor.param_count)
		return 0.0;

	return atomic_load_explicit(&plugin->values[index], memory_order_relaxed);
}

double upl_param_normalized(Upl_Plugin *plugin, uint32_t index)
{
	if (!plugin || index >= upl_descriptor.param_count)
		return 0.0;

	const Upl_ParamInfo *info = &upl_descriptor.params[index];
	const double range = info->max - info->min;
	return range > 0.0 ? (upl_param(plugin, index) - info->min) / range : 0.0;
}

static void upl__param_request_flush(Upl_Plugin *plugin)
{
	if (plugin->host_params && plugin->host_params->request_flush)
		plugin->host_params->request_flush(plugin->host);
}

static double upl__param_constrain(uint32_t index, double value)
{
	const Upl_ParamInfo *info = &upl_descriptor.params[index];
	value = fmin(fmax(value, info->min), info->max);
	if (info->flags & (UPL_PARAM_STEPPED | UPL_PARAM_BYPASS))
		value = round(value);

	return value;
}

void upl_param_set(Upl_Plugin *plugin, uint32_t index, double value)
{
	if (!plugin || index >= upl_descriptor.param_count)
		return;

	value = upl__param_constrain(index, value);
	if (atomic_load(&plugin->values[index]) == value)
		return;

	atomic_store(&plugin->values[index], value);
	atomic_fetch_or(&plugin->pending[index], UPL_PENDING_VALUE);
	upl__param_request_flush(plugin);
	upl__gui_mark_dirty(plugin);
}

void upl_param_set_normalized(Upl_Plugin *plugin, uint32_t index, double normalized)
{
	if (!plugin || index >= upl_descriptor.param_count)
		return;

	const Upl_ParamInfo *info = &upl_descriptor.params[index];
	normalized = fmin(fmax(normalized, 0.0), 1.0);
	upl_param_set(plugin, index, info->min + (info->max - info->min) * normalized);
}

void upl_param_begin_gesture(Upl_Plugin *plugin, uint32_t index)
{
	if (!plugin || index >= upl_descriptor.param_count)
		return;

	atomic_fetch_or(&plugin->pending[index], UPL_PENDING_GESTURE_BEGIN);
	upl__param_request_flush(plugin);
}

void upl_param_end_gesture(Upl_Plugin *plugin, uint32_t index)
{
	if (!plugin || index >= upl_descriptor.param_count)
		return;

	atomic_fetch_or(&plugin->pending[index], UPL_PENDING_GESTURE_END);
	upl__param_request_flush(plugin);
}

void upl_param_text(Upl_Plugin *plugin, uint32_t index, double value, char *buffer, uint32_t size)
{
	(void)plugin;
	if (!buffer || size == 0)
		return;

	if (index >= upl_descriptor.param_count)
	{
		buffer[0] = '\0';
		return;
	}

	const Upl_ParamInfo *info = &upl_descriptor.params[index];
	const bool stepped = (info->flags & (UPL_PARAM_STEPPED | UPL_PARAM_BYPASS)) != 0;
	const bool has_unit = info->unit && info->unit[0];
	snprintf(buffer, size, stepped ? "%.0f%s%s" : "%.2f%s%s", value, has_unit ? " " : "", has_unit ? info->unit : "");
}

static void upl__params_flush_out(Upl_Plugin *plugin, const clap_output_events_t *out)
{
	if (!out)
		return;

	for (uint32_t i = 0; i < upl_descriptor.param_count; i++)
	{
		const uint32_t flags = atomic_exchange(&plugin->pending[i], 0u);
		if (!flags)
			continue;

		clap_event_param_gesture_t gesture = {
			.header = {
				.size = sizeof(clap_event_param_gesture_t),
				.time = 0,
				.space_id = CLAP_CORE_EVENT_SPACE_ID,
				.type = CLAP_EVENT_PARAM_GESTURE_BEGIN,
				.flags = 0
			},
			.param_id = i
		};

		if (flags & UPL_PENDING_GESTURE_BEGIN)
			out->try_push(out, &gesture.header);

		if (flags & UPL_PENDING_VALUE)
		{
			const clap_event_param_value_t value = {
				.header = {
					.size = sizeof(clap_event_param_value_t),
					.time = 0,
					.space_id = CLAP_CORE_EVENT_SPACE_ID,
					.type = CLAP_EVENT_PARAM_VALUE,
					.flags = 0
				},
				.param_id = i,
				.cookie = NULL,
				.note_id = -1,
				.port_index = -1,
				.channel = -1,
				.key = -1,
				.value = upl_param(plugin, i)
			};
			out->try_push(out, &value.header);
		}

		if (flags & UPL_PENDING_GESTURE_END)
		{
			gesture.header.type = CLAP_EVENT_PARAM_GESTURE_END;
			out->try_push(out, &gesture.header);
		}
	}
}

static bool upl__handle_param_event(Upl_Plugin *plugin, const clap_event_header_t *header)
{
	if (header->space_id != CLAP_CORE_EVENT_SPACE_ID || header->type != CLAP_EVENT_PARAM_VALUE)
		return false;

	const clap_event_param_value_t *event = (const clap_event_param_value_t *)header;
	if (event->param_id < upl_descriptor.param_count)
	{
		atomic_store(&plugin->values[event->param_id], upl__param_constrain(event->param_id, event->value));
		atomic_store(&plugin->ui_dirty, true);
	}

	return true;
}

static void upl__handle_note_event(Upl_Plugin *plugin, const clap_event_header_t *header)
{
	if (header->space_id != CLAP_CORE_EVENT_SPACE_ID || !upl_descriptor.note)
		return;

	Upl_NoteEvent note = { .note_id = -1 };
	switch (header->type)
	{
		case CLAP_EVENT_NOTE_ON:
		case CLAP_EVENT_NOTE_OFF:
		case CLAP_EVENT_NOTE_CHOKE:
		{
			const clap_event_note_t *event = (const clap_event_note_t *)header;
			note.type = header->type == CLAP_EVENT_NOTE_ON ? UPL_NOTE_ON : (header->type == CLAP_EVENT_NOTE_OFF ? UPL_NOTE_OFF : UPL_NOTE_CHOKE);
			note.key = event->key;
			note.channel = event->channel;
			note.note_id = event->note_id;
			note.velocity = event->velocity;
			break;
		}
		case CLAP_EVENT_MIDI:
		{
			const clap_event_midi_t *event = (const clap_event_midi_t *)header;
			const uint8_t status = event->data[0] & 0xF0;
			if (status != 0x80 && status != 0x90)
				return;

			note.type = (status == 0x90 && event->data[2] > 0) ? UPL_NOTE_ON : UPL_NOTE_OFF;
			note.key = event->data[1] & 0x7F;
			note.channel = event->data[0] & 0x0F;
			note.velocity = (double)event->data[2] / 127.0;
			break;
		}
		default:
			return;
	}

	upl_descriptor.note(plugin, &note);
}

#pragma endregion

#pragma region Plugin

static bool upl__clap_init(const clap_plugin_t *clap)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	const clap_host_t *host = plugin->host;
	plugin->host_params = (const clap_host_params_t *)host->get_extension(host, CLAP_EXT_PARAMS);
	plugin->host_timer = (const clap_host_timer_support_t *)host->get_extension(host, CLAP_EXT_TIMER_SUPPORT);
	plugin->host_gui = (const clap_host_gui_t *)host->get_extension(host, CLAP_EXT_GUI);

	if (upl_descriptor.create)
		plugin->user_data = upl_descriptor.create(plugin);

	return true;
}

static void upl__clap_gui_destroy(const clap_plugin_t *clap);

static void upl__clap_destroy(const clap_plugin_t *clap)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (plugin->gui.created)
		upl__clap_gui_destroy(clap);

	if (upl_descriptor.destroy)
		upl_descriptor.destroy(plugin);

	free(plugin->values);
	free(plugin->pending);
	free(plugin);
}

static bool upl__clap_activate(const clap_plugin_t *clap, double sample_rate, uint32_t min_frames, uint32_t max_frames)
{
	(void)min_frames;
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	plugin->sample_rate = sample_rate;
	plugin->active = true;

	if (upl_descriptor.activate)
		upl_descriptor.activate(plugin, sample_rate, max_frames);

	return true;
}

static void upl__clap_deactivate(const clap_plugin_t *clap)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	plugin->active = false;

	if (upl_descriptor.deactivate)
		upl_descriptor.deactivate(plugin);
}

static bool upl__clap_start_processing(const clap_plugin_t *clap)
{
	(void)clap;
	return true;
}

static void upl__clap_stop_processing(const clap_plugin_t *clap)
{
	(void)clap;
}

static void upl__clap_reset(const clap_plugin_t *clap)
{
	(void)clap;
}

static void upl__process_range(Upl_Plugin *plugin, const clap_process_t *process, uint32_t offset, uint32_t frames)
{
	const float *inputs[UPL_MAX_CHANNELS] = { 0 };
	float *outputs[UPL_MAX_CHANNELS] = { 0 };
	uint32_t input_channels = 0;
	uint32_t output_channels = 0;

	if (process->audio_inputs_count > 0 && process->audio_inputs[0].data32)
	{
		input_channels = process->audio_inputs[0].channel_count;
		input_channels = input_channels < upl_descriptor.input_channels ? input_channels : upl_descriptor.input_channels;
		input_channels = input_channels < UPL_MAX_CHANNELS ? input_channels : UPL_MAX_CHANNELS;
		for (uint32_t c = 0; c < input_channels; c++)
			inputs[c] = process->audio_inputs[0].data32[c] + offset;
	}

	if (process->audio_outputs_count > 0 && process->audio_outputs[0].data32)
	{
		output_channels = process->audio_outputs[0].channel_count;
		output_channels = output_channels < UPL_MAX_CHANNELS ? output_channels : UPL_MAX_CHANNELS;
		for (uint32_t c = 0; c < output_channels; c++)
			outputs[c] = process->audio_outputs[0].data32[c] + offset;
	}

	if (!upl_descriptor.process)
	{
		for (uint32_t c = 0; c < output_channels; c++)
			memset(outputs[c], 0, frames * sizeof(float));
		return;
	}

	const Upl_AudioBlock block = {
		.inputs = inputs,
		.outputs = outputs,
		.input_channels = input_channels,
		.output_channels = output_channels,
		.frame_count = frames,
		.sample_rate = plugin->sample_rate
	};
	upl_descriptor.process(plugin, &block);
}

static void upl__handle_event(Upl_Plugin *plugin, const clap_event_header_t *header)
{
	if (!upl__handle_param_event(plugin, header))
		upl__handle_note_event(plugin, header);
}

static clap_process_status upl__clap_process(const clap_plugin_t *clap, const clap_process_t *process)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	upl__params_flush_out(plugin, process->out_events);

	const clap_input_events_t *events = process->in_events;
	const uint32_t event_count = events ? events->size(events) : 0;
	const uint32_t frames = process->frames_count;
	uint32_t cursor = 0;
	uint32_t index = 0;

	for (;;)
	{
		while (index < event_count)
		{
			const clap_event_header_t *header = events->get(events, index);
			if (header->time > cursor)
				break;

			upl__handle_event(plugin, header);
			index++;
		}

		if (cursor >= frames)
			break;

		uint32_t next = frames;
		if (index < event_count)
		{
			const uint32_t time = events->get(events, index)->time;
			next = time < frames ? time : frames;
		}

		upl__process_range(plugin, process, cursor, next - cursor);
		cursor = next;
	}

	for (; index < event_count; index++)
		upl__handle_event(plugin, events->get(events, index));

	return CLAP_PROCESS_CONTINUE;
}

static void upl__clap_on_main_thread(const clap_plugin_t *clap)
{
	(void)clap;
}

#pragma endregion

#pragma region Ports

static uint32_t upl__audio_ports_count(const clap_plugin_t *clap, bool is_input)
{
	(void)clap;
	return (is_input ? upl_descriptor.input_channels : upl_descriptor.output_channels) > 0 ? 1 : 0;
}

static bool upl__audio_ports_get(const clap_plugin_t *clap, uint32_t index, bool is_input, clap_audio_port_info_t *info)
{
	if (index != 0 || upl__audio_ports_count(clap, is_input) == 0)
		return false;

	const uint32_t channels = is_input ? upl_descriptor.input_channels : upl_descriptor.output_channels;
	memset(info, 0, sizeof(*info));
	info->id = 0;
	snprintf(info->name, sizeof(info->name), "%s", is_input ? "Main In" : "Main Out");
	info->flags = CLAP_AUDIO_PORT_IS_MAIN;
	info->channel_count = channels < UPL_MAX_CHANNELS ? channels : UPL_MAX_CHANNELS;
	info->port_type = info->channel_count == 2 ? CLAP_PORT_STEREO : (info->channel_count == 1 ? CLAP_PORT_MONO : NULL);
	info->in_place_pair = CLAP_INVALID_ID;
	return true;
}

static const clap_plugin_audio_ports_t upl__audio_ports = {
	.count = upl__audio_ports_count,
	.get = upl__audio_ports_get
};

static uint32_t upl__note_ports_count(const clap_plugin_t *clap, bool is_input)
{
	(void)clap;
	return (is_input && upl_descriptor.note_input) ? 1 : 0;
}

static bool upl__note_ports_get(const clap_plugin_t *clap, uint32_t index, bool is_input, clap_note_port_info_t *info)
{
	if (index != 0 || upl__note_ports_count(clap, is_input) == 0)
		return false;

	memset(info, 0, sizeof(*info));
	info->id = 0;
	info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
	info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
	snprintf(info->name, sizeof(info->name), "%s", "Notes");
	return true;
}

static const clap_plugin_note_ports_t upl__note_ports = {
	.count = upl__note_ports_count,
	.get = upl__note_ports_get
};

#pragma endregion

#pragma region Params Extension

static uint32_t upl__params_count(const clap_plugin_t *clap)
{
	(void)clap;
	return upl_descriptor.param_count;
}

static bool upl__params_get_info(const clap_plugin_t *clap, uint32_t index, clap_param_info_t *info)
{
	(void)clap;
	if (index >= upl_descriptor.param_count)
		return false;

	const Upl_ParamInfo *param = &upl_descriptor.params[index];
	memset(info, 0, sizeof(*info));
	info->id = index;
	info->flags = CLAP_PARAM_IS_AUTOMATABLE;
	if (param->flags & UPL_PARAM_STEPPED)
		info->flags |= CLAP_PARAM_IS_STEPPED;
	if (param->flags & UPL_PARAM_BYPASS)
		info->flags |= CLAP_PARAM_IS_BYPASS | CLAP_PARAM_IS_STEPPED;
	if (param->flags & UPL_PARAM_HIDDEN)
		info->flags |= CLAP_PARAM_IS_HIDDEN;

	snprintf(info->name, sizeof(info->name), "%s", param->name ? param->name : "");
	info->min_value = param->min;
	info->max_value = param->max;
	info->default_value = param->default_value;
	return true;
}

static bool upl__params_get_value(const clap_plugin_t *clap, clap_id id, double *value)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (id >= upl_descriptor.param_count)
		return false;

	*value = upl_param(plugin, id);
	return true;
}

static bool upl__params_value_to_text(const clap_plugin_t *clap, clap_id id, double value, char *display, uint32_t size)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (id >= upl_descriptor.param_count)
		return false;

	upl_param_text(plugin, id, value, display, size);
	return true;
}

static bool upl__params_text_to_value(const clap_plugin_t *clap, clap_id id, const char *display, double *value)
{
	(void)clap;
	if (id >= upl_descriptor.param_count || !display)
		return false;

	char *end = NULL;
	const double parsed = strtod(display, &end);
	if (end == display)
		return false;

	*value = upl__param_constrain(id, parsed);
	return true;
}

static void upl__params_flush(const clap_plugin_t *clap, const clap_input_events_t *in, const clap_output_events_t *out)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	const uint32_t count = in ? in->size(in) : 0;
	for (uint32_t i = 0; i < count; i++)
		upl__handle_param_event(plugin, in->get(in, i));

	upl__params_flush_out(plugin, out);
}

static const clap_plugin_params_t upl__params = {
	.count = upl__params_count,
	.get_info = upl__params_get_info,
	.get_value = upl__params_get_value,
	.value_to_text = upl__params_value_to_text,
	.text_to_value = upl__params_text_to_value,
	.flush = upl__params_flush
};

#pragma endregion

#pragma region State

static bool upl__stream_write(const clap_ostream_t *stream, const void *data, uint64_t size)
{
	const uint8_t *bytes = (const uint8_t *)data;
	while (size > 0)
	{
		const int64_t written = stream->write(stream, bytes, size);
		if (written <= 0)
			return false;

		bytes += written;
		size -= (uint64_t)written;
	}

	return true;
}

static bool upl__stream_read(const clap_istream_t *stream, void *data, uint64_t size)
{
	uint8_t *bytes = (uint8_t *)data;
	while (size > 0)
	{
		const int64_t read = stream->read(stream, bytes, size);
		if (read <= 0)
			return false;

		bytes += read;
		size -= (uint64_t)read;
	}

	return true;
}

static bool upl__state_save(const clap_plugin_t *clap, const clap_ostream_t *stream)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	const uint32_t header[2] = { UPL_STATE_MAGIC, upl_descriptor.param_count };
	if (!upl__stream_write(stream, header, sizeof(header)))
		return false;

	for (uint32_t i = 0; i < upl_descriptor.param_count; i++)
	{
		const double value = upl_param(plugin, i);
		if (!upl__stream_write(stream, &i, sizeof(i)) || !upl__stream_write(stream, &value, sizeof(value)))
			return false;
	}

	return true;
}

static bool upl__state_load(const clap_plugin_t *clap, const clap_istream_t *stream)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	uint32_t header[2];
	if (!upl__stream_read(stream, header, sizeof(header)) || header[0] != UPL_STATE_MAGIC)
		return false;

	for (uint32_t i = 0; i < header[1]; i++)
	{
		uint32_t id;
		double value;
		if (!upl__stream_read(stream, &id, sizeof(id)) || !upl__stream_read(stream, &value, sizeof(value)))
			return false;

		if (id < upl_descriptor.param_count)
			atomic_store(&plugin->values[id], upl__param_constrain(id, value));
	}

	atomic_store(&plugin->ui_dirty, true);
	if (plugin->host_params && plugin->host_params->rescan)
		plugin->host_params->rescan(plugin->host, CLAP_PARAM_RESCAN_VALUES);

	return true;
}

static const clap_plugin_state_t upl__state = {
	.save = upl__state_save,
	.load = upl__state_load
};

#pragma endregion

#pragma region GUI Extension

static bool upl__clap_gui_is_api_supported(const clap_plugin_t *clap, const char *api, bool is_floating)
{
	(void)clap;
	return !is_floating && api && strcmp(api, UPL_WINDOW_API) == 0;
}

static bool upl__clap_gui_get_preferred_api(const clap_plugin_t *clap, const char **api, bool *is_floating)
{
	(void)clap;
	*api = UPL_WINDOW_API;
	*is_floating = false;
	return true;
}

static bool upl__clap_gui_create(const clap_plugin_t *clap, const char *api, bool is_floating)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (!upl__clap_gui_is_api_supported(clap, api, is_floating))
		return false;

	if (!plugin->gui.created)
		upl__gui_create(plugin);

#if UPL_USE_HOST_TIMER
	if (!plugin->gui.timer_registered && plugin->host_timer && plugin->host_timer->register_timer)
		plugin->gui.timer_registered = plugin->host_timer->register_timer(plugin->host, UPL_FRAME_INTERVAL_MS, &plugin->gui.timer_id);
#endif

	return true;
}

static void upl__clap_gui_destroy(const clap_plugin_t *clap)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (plugin->gui.timer_registered && plugin->host_timer && plugin->host_timer->unregister_timer)
		plugin->host_timer->unregister_timer(plugin->host, plugin->gui.timer_id);

	plugin->gui.timer_registered = false;
	upl__gui_destroy(plugin);
}

static bool upl__clap_gui_set_scale(const clap_plugin_t *clap, double scale)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (UPL_WINDOW_LOGICAL_SIZE || scale <= 0.0)
		return false;

	plugin->gui.scale = (float)scale;
	if (plugin->gui.created)
	{
		upl__gui_update_size(plugin);
		if (plugin->gui.window)
			upl__window_resize(plugin);
	}

	return true;
}

static bool upl__clap_gui_get_size(const clap_plugin_t *clap, uint32_t *width, uint32_t *height)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	const float scale = UPL_WINDOW_LOGICAL_SIZE ? 1.0f : (plugin->gui.scale > 0.0f ? plugin->gui.scale : 1.0f);
	*width = (uint32_t)ceilf((float)upl_descriptor.gui_width * scale);
	*height = (uint32_t)ceilf((float)upl_descriptor.gui_height * scale);
	return true;
}

static bool upl__clap_gui_can_resize(const clap_plugin_t *clap)
{
	(void)clap;
	return false;
}

static bool upl__clap_gui_get_resize_hints(const clap_plugin_t *clap, clap_gui_resize_hints_t *hints)
{
	(void)clap;
	(void)hints;
	return false;
}

static bool upl__clap_gui_adjust_size(const clap_plugin_t *clap, uint32_t *width, uint32_t *height)
{
	return upl__clap_gui_get_size(clap, width, height);
}

static bool upl__clap_gui_set_size(const clap_plugin_t *clap, uint32_t width, uint32_t height)
{
	uint32_t current_width, current_height;
	upl__clap_gui_get_size(clap, &current_width, &current_height);
	return width == current_width && height == current_height;
}

static bool upl__clap_gui_set_parent(const clap_plugin_t *clap, const clap_window_t *window)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (!plugin->gui.created || !window)
		return false;

	if (plugin->gui.window)
		upl__window_destroy(plugin);

	if (!upl__window_create(plugin, window))
		return false;

	upl__gui_mark_dirty(plugin);
	return true;
}

static bool upl__clap_gui_set_transient(const clap_plugin_t *clap, const clap_window_t *window)
{
	(void)clap;
	(void)window;
	return false;
}

static void upl__clap_gui_suggest_title(const clap_plugin_t *clap, const char *title)
{
	(void)clap;
	(void)title;
}

static bool upl__clap_gui_show(const clap_plugin_t *clap)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (!plugin->gui.window)
		return false;

	plugin->gui.visible = true;
	upl__window_set_visible(plugin, true);
	upl__gui_mark_dirty(plugin);
	return true;
}

static bool upl__clap_gui_hide(const clap_plugin_t *clap)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (!plugin->gui.window)
		return false;

	plugin->gui.visible = false;
	upl__window_set_visible(plugin, false);
	return true;
}

static const clap_plugin_gui_t upl__gui = {
	.is_api_supported = upl__clap_gui_is_api_supported,
	.get_preferred_api = upl__clap_gui_get_preferred_api,
	.create = upl__clap_gui_create,
	.destroy = upl__clap_gui_destroy,
	.set_scale = upl__clap_gui_set_scale,
	.get_size = upl__clap_gui_get_size,
	.can_resize = upl__clap_gui_can_resize,
	.get_resize_hints = upl__clap_gui_get_resize_hints,
	.adjust_size = upl__clap_gui_adjust_size,
	.set_size = upl__clap_gui_set_size,
	.set_parent = upl__clap_gui_set_parent,
	.set_transient = upl__clap_gui_set_transient,
	.suggest_title = upl__clap_gui_suggest_title,
	.show = upl__clap_gui_show,
	.hide = upl__clap_gui_hide
};

static void upl__clap_on_timer(const clap_plugin_t *clap, clap_id timer_id)
{
	Upl_Plugin *plugin = (Upl_Plugin *)clap->plugin_data;
	if (plugin->gui.timer_registered && timer_id == plugin->gui.timer_id)
		upl__gui_tick(plugin);
}

static const clap_plugin_timer_support_t upl__timer_support = {
	.on_timer = upl__clap_on_timer
};

#pragma endregion

#pragma region Entry

static const void *upl__clap_get_extension(const clap_plugin_t *clap, const char *id)
{
	(void)clap;
	if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
		return &upl__audio_ports;
	if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0 && upl_descriptor.note_input)
		return &upl__note_ports;
	if (strcmp(id, CLAP_EXT_PARAMS) == 0)
		return &upl__params;
	if (strcmp(id, CLAP_EXT_STATE) == 0)
		return &upl__state;
	if (strcmp(id, CLAP_EXT_GUI) == 0 && upl_descriptor.ui)
		return &upl__gui;
	if (strcmp(id, CLAP_EXT_TIMER_SUPPORT) == 0 && UPL_USE_HOST_TIMER)
		return &upl__timer_support;
	return NULL;
}

static uint32_t upl__factory_count(const clap_plugin_factory_t *factory)
{
	(void)factory;
	return 1;
}

static const clap_plugin_descriptor_t *upl__factory_descriptor(const clap_plugin_factory_t *factory, uint32_t index)
{
	(void)factory;
	return index == 0 ? &upl__module_state.descriptor : NULL;
}

static const clap_plugin_t *upl__factory_create(const clap_plugin_factory_t *factory, const clap_host_t *host, const char *plugin_id)
{
	(void)factory;
	if (!host || !plugin_id || strcmp(plugin_id, upl_descriptor.id) != 0 || !clap_version_is_compatible(host->clap_version))
		return NULL;

	Upl_Plugin *plugin = (Upl_Plugin *)calloc(1, sizeof(Upl_Plugin));
	const uint32_t count = upl_descriptor.param_count;
	plugin->host = host;
	plugin->values = (_Atomic double *)calloc(count ? count : 1, sizeof(_Atomic double));
	plugin->pending = (atomic_uint *)calloc(count ? count : 1, sizeof(atomic_uint));
	atomic_init(&plugin->ui_dirty, false);
	for (uint32_t i = 0; i < count; i++)
	{
		atomic_init(&plugin->values[i], upl__param_constrain(i, upl_descriptor.params[i].default_value));
		atomic_init(&plugin->pending[i], 0u);
	}

	plugin->clap = (clap_plugin_t) {
		.desc = &upl__module_state.descriptor,
		.plugin_data = plugin,
		.init = upl__clap_init,
		.destroy = upl__clap_destroy,
		.activate = upl__clap_activate,
		.deactivate = upl__clap_deactivate,
		.start_processing = upl__clap_start_processing,
		.stop_processing = upl__clap_stop_processing,
		.reset = upl__clap_reset,
		.process = upl__clap_process,
		.get_extension = upl__clap_get_extension,
		.on_main_thread = upl__clap_on_main_thread
	};

	return &plugin->clap;
}

static const clap_plugin_factory_t upl__factory = {
	.get_plugin_count = upl__factory_count,
	.get_plugin_descriptor = upl__factory_descriptor,
	.create_plugin = upl__factory_create
};

static bool upl__entry_init(const char *plugin_path)
{
	(void)plugin_path;
	Upl_ModuleState *state = &upl__module_state;
	if (state->init_count++ > 0)
		return true;

	state->descriptor = (clap_plugin_descriptor_t) {
		.clap_version = CLAP_VERSION_INIT,
		.id = upl_descriptor.id,
		.name = upl_descriptor.name,
		.vendor = upl_descriptor.vendor ? upl_descriptor.vendor : "",
		.url = upl_descriptor.url ? upl_descriptor.url : "",
		.manual_url = "",
		.support_url = "",
		.version = upl_descriptor.version ? upl_descriptor.version : "1.0.0",
		.description = upl_descriptor.description ? upl_descriptor.description : "",
		.features = upl_descriptor.features ? upl_descriptor.features : upl__empty_features
	};

	if (upl__renderer_state.font_count == 0)
		upl__font_add(upl__default_font_data);

	return true;
}

static void upl__entry_deinit(void)
{
	Upl_ModuleState *state = &upl__module_state;
	if (state->init_count == 0 || --state->init_count > 0)
		return;

	upl__glyph_cache_clear();
}

static const void *upl__entry_get_factory(const char *factory_id)
{
	return strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0 ? &upl__factory : NULL;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
	.clap_version = CLAP_VERSION_INIT,
	.init = upl__entry_init,
	.deinit = upl__entry_deinit,
	.get_factory = upl__entry_get_factory
};

#pragma endregion
