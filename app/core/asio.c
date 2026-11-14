#ifdef _WIN32

#include <windows.h>
#include <objbase.h>

#if defined(_M_IX86) || defined(__i386__)
#define UPH_ASIO_METHOD __thiscall
#else
#define UPH_ASIO_METHOD
#endif

#define UPH_ASIO_MAX_CHANNELS 2

typedef long Uph_AsioError;
typedef long Uph_AsioBool;

enum
{
	UPH_ASE_OK = 0,
	UPH_ASE_SUCCESS = 0x3f4847a0
};

enum
{
	UPH_ASIO_TYPE_INT16_LSB = 16,
	UPH_ASIO_TYPE_INT24_LSB = 17,
	UPH_ASIO_TYPE_INT32_LSB = 18,
	UPH_ASIO_TYPE_FLOAT32_LSB = 19,
	UPH_ASIO_TYPE_FLOAT64_LSB = 20,
	UPH_ASIO_TYPE_INT32_LSB16 = 24,
	UPH_ASIO_TYPE_INT32_LSB18 = 25,
	UPH_ASIO_TYPE_INT32_LSB20 = 26,
	UPH_ASIO_TYPE_INT32_LSB24 = 27
};

enum
{
	UPH_ASIO_MSG_SELECTOR_SUPPORTED = 1,
	UPH_ASIO_MSG_ENGINE_VERSION = 2,
	UPH_ASIO_MSG_RESET_REQUEST = 3,
	UPH_ASIO_MSG_BUFFER_SIZE_CHANGE = 4,
	UPH_ASIO_MSG_RESYNC_REQUEST = 5,
	UPH_ASIO_MSG_LATENCIES_CHANGED = 6,
	UPH_ASIO_MSG_SUPPORTS_TIME_INFO = 7
};

typedef struct
{
	long channel;
	Uph_AsioBool is_input;
	Uph_AsioBool is_active;
	long channel_group;
	long type;
	char name[32];
}
Uph_AsioChannelInfo;

typedef struct
{
	Uph_AsioBool is_input;
	long channel;
	void *buffers[2];
}
Uph_AsioBufferInfo;

typedef struct Uph_AsioTime Uph_AsioTime;

typedef struct
{
	void (*buffer_switch)(long index, Uph_AsioBool direct);
	void (*sample_rate_changed)(double rate);
	long (*message)(long selector, long value, void *message, double *opt);
	Uph_AsioTime *(*buffer_switch_time_info)(Uph_AsioTime *params, long index, Uph_AsioBool direct);
}
Uph_AsioCallbacks;

typedef struct Uph_AsioDriver Uph_AsioDriver;

typedef struct
{
	HRESULT (STDMETHODCALLTYPE *query_interface)(Uph_AsioDriver *self, const IID *riid, void **out);
	ULONG (STDMETHODCALLTYPE *add_ref)(Uph_AsioDriver *self);
	ULONG (STDMETHODCALLTYPE *release)(Uph_AsioDriver *self);
	Uph_AsioBool (UPH_ASIO_METHOD *init)(Uph_AsioDriver *self, void *sys_handle);
	void (UPH_ASIO_METHOD *get_driver_name)(Uph_AsioDriver *self, char *name);
	long (UPH_ASIO_METHOD *get_driver_version)(Uph_AsioDriver *self);
	void (UPH_ASIO_METHOD *get_error_message)(Uph_AsioDriver *self, char *message);
	Uph_AsioError (UPH_ASIO_METHOD *start)(Uph_AsioDriver *self);
	Uph_AsioError (UPH_ASIO_METHOD *stop)(Uph_AsioDriver *self);
	Uph_AsioError (UPH_ASIO_METHOD *get_channels)(Uph_AsioDriver *self, long *inputs, long *outputs);
	Uph_AsioError (UPH_ASIO_METHOD *get_latencies)(Uph_AsioDriver *self, long *input, long *output);
	Uph_AsioError (UPH_ASIO_METHOD *get_buffer_size)(Uph_AsioDriver *self, long *min, long *max, long *preferred, long *granularity);
	Uph_AsioError (UPH_ASIO_METHOD *can_sample_rate)(Uph_AsioDriver *self, double rate);
	Uph_AsioError (UPH_ASIO_METHOD *get_sample_rate)(Uph_AsioDriver *self, double *rate);
	Uph_AsioError (UPH_ASIO_METHOD *set_sample_rate)(Uph_AsioDriver *self, double rate);
	Uph_AsioError (UPH_ASIO_METHOD *get_clock_sources)(Uph_AsioDriver *self, void *clocks, long *count);
	Uph_AsioError (UPH_ASIO_METHOD *set_clock_source)(Uph_AsioDriver *self, long reference);
	Uph_AsioError (UPH_ASIO_METHOD *get_sample_position)(Uph_AsioDriver *self, void *samples, void *timestamp);
	Uph_AsioError (UPH_ASIO_METHOD *get_channel_info)(Uph_AsioDriver *self, Uph_AsioChannelInfo *info);
	Uph_AsioError (UPH_ASIO_METHOD *create_buffers)(Uph_AsioDriver *self, Uph_AsioBufferInfo *infos, long count, long buffer_size, Uph_AsioCallbacks *callbacks);
	Uph_AsioError (UPH_ASIO_METHOD *dispose_buffers)(Uph_AsioDriver *self);
	Uph_AsioError (UPH_ASIO_METHOD *control_panel)(Uph_AsioDriver *self);
	Uph_AsioError (UPH_ASIO_METHOD *future)(Uph_AsioDriver *self, long selector, void *opt);
	Uph_AsioError (UPH_ASIO_METHOD *output_ready)(Uph_AsioDriver *self);
}
Uph_AsioDriverVtbl;

struct Uph_AsioDriver
{
	const Uph_AsioDriverVtbl *vtbl;
};

typedef struct
{
	char name[128];
	CLSID clsid;
}
Uph_AsioDriverEntry;

typedef struct
{
	Uph_AsioDriver *driver;
	HMODULE module;
	ma_device *device;

	long buffer_size;
	long input_count;
	long output_count;
	long input_types[UPH_ASIO_MAX_CHANNELS];
	long output_types[UPH_ASIO_MAX_CHANNELS];
	Uph_AsioBufferInfo buffers[UPH_ASIO_MAX_CHANNELS * 2];

	ma_uint32 playback_channels;
	float *input_scratch;
	float *output_scratch;

	bool post_output;
	bool buffers_created;
	_Atomic bool reset_requested;
	Uph_AsioCallbacks callbacks;
}
Uph_AsioState;

static Uph_AsioState uph_asio;

static const IID uph_asio_iid_class_factory = { 0x00000001, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

static uint32_t uph_asio_enumerate(Uph_AsioDriverEntry *out)
{
	HKEY root;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ASIO", 0, KEY_READ, &root) != ERROR_SUCCESS)
		return 0;

	uint32_t count = 0;
	for (DWORD i = 0; count < UPH_ASIO_MAX_DRIVERS; i++)
	{
		char key_name[128];
		DWORD key_length = sizeof(key_name);
		const LONG result = RegEnumKeyExA(root, i, key_name, &key_length, NULL, NULL, NULL, NULL);
		if (result == ERROR_NO_MORE_ITEMS)
			break;
		if (result != ERROR_SUCCESS)
			continue;

		HKEY key;
		if (RegOpenKeyExA(root, key_name, 0, KEY_READ, &key) != ERROR_SUCCESS)
			continue;

		char clsid_text[64] = { 0 };
		DWORD clsid_length = sizeof(clsid_text) - 1;
		DWORD type = 0;
		const bool has_clsid = RegQueryValueExA(key, "CLSID", NULL, &type, (BYTE*)clsid_text, &clsid_length) == ERROR_SUCCESS && type == REG_SZ;
		RegCloseKey(key);

		WCHAR clsid_wide[64];
		if (!has_clsid || !MultiByteToWideChar(CP_ACP, 0, clsid_text, -1, clsid_wide, 64) || FAILED(CLSIDFromString(clsid_wide, &out[count].clsid)))
			continue;

		strncpy(out[count].name, key_name, sizeof(out[count].name) - 1);
		out[count].name[sizeof(out[count].name) - 1] = '\0';
		count++;
	}

	RegCloseKey(root);
	return count;
}

static bool uph_asio_find_driver(const ma_device_id *id, Uph_AsioDriverEntry *out)
{
	Uph_AsioDriverEntry entries[UPH_ASIO_MAX_DRIVERS];
	const uint32_t count = uph_asio_enumerate(entries);
	if (count == 0)
		return false;

	if (!id || !id->custom.s[0])
	{
		*out = entries[0];
		return true;
	}

	for (uint32_t i = 0; i < count; i++)
	{
		if (strcmp(entries[i].name, id->custom.s) == 0)
		{
			*out = entries[i];
			return true;
		}
	}

	return false;
}

static bool uph_asio_load(const CLSID *clsid)
{
	WCHAR clsid_text[64];
	if (!StringFromGUID2(clsid, clsid_text, 64))
		return false;

	WCHAR key_path[128] = L"CLSID\\";
	wcscat(key_path, clsid_text);
	wcscat(key_path, L"\\InprocServer32");

	HKEY key;
	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, key_path, 0, KEY_READ, &key) != ERROR_SUCCESS)
		return false;

	WCHAR raw_path[MAX_PATH] = { 0 };
	DWORD size = sizeof(raw_path) - sizeof(WCHAR);
	DWORD type = 0;
	const LONG result = RegQueryValueExW(key, NULL, NULL, &type, (BYTE*)raw_path, &size);
	RegCloseKey(key);

	if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
		return false;

	WCHAR dll_path[MAX_PATH];
	if (!ExpandEnvironmentStringsW(raw_path, dll_path, MAX_PATH))
		return false;

	HMODULE module = LoadLibraryW(dll_path);
	if (!module)
		return false;

	typedef HRESULT (STDAPICALLTYPE *Uph_DllGetClassObject)(const CLSID*, const IID*, void**);
	Uph_DllGetClassObject get_class_object = (Uph_DllGetClassObject)(void*)GetProcAddress(module, "DllGetClassObject");

	IClassFactory *factory = NULL;
	if (!get_class_object || FAILED(get_class_object(clsid, &uph_asio_iid_class_factory, (void**)&factory)) || !factory)
	{
		FreeLibrary(module);
		return false;
	}

	void *instance = NULL;
	const HRESULT created = factory->lpVtbl->CreateInstance(factory, NULL, clsid, &instance);
	factory->lpVtbl->Release(factory);

	if (FAILED(created) || !instance)
	{
		FreeLibrary(module);
		return false;
	}

	uph_asio.driver = (Uph_AsioDriver*)instance;
	uph_asio.module = module;
	return true;
}

static void uph_asio_unload(void)
{
	if (uph_asio.driver)
	{
		uph_asio.driver->vtbl->release(uph_asio.driver);
		uph_asio.driver = NULL;
	}

	if (uph_asio.module)
	{
		FreeLibrary(uph_asio.module);
		uph_asio.module = NULL;
	}
}

static bool uph_asio_type_supported(const long type)
{
	switch (type)
	{
		case UPH_ASIO_TYPE_INT16_LSB:
		case UPH_ASIO_TYPE_INT24_LSB:
		case UPH_ASIO_TYPE_INT32_LSB:
		case UPH_ASIO_TYPE_FLOAT32_LSB:
		case UPH_ASIO_TYPE_FLOAT64_LSB:
		case UPH_ASIO_TYPE_INT32_LSB16:
		case UPH_ASIO_TYPE_INT32_LSB18:
		case UPH_ASIO_TYPE_INT32_LSB20:
		case UPH_ASIO_TYPE_INT32_LSB24:
			return true;
	}

	return false;
}

static float uph_asio_int32_scale(const long type)
{
	switch (type)
	{
		case UPH_ASIO_TYPE_INT32_LSB16: return 32768.0f;
		case UPH_ASIO_TYPE_INT32_LSB18: return 131072.0f;
		case UPH_ASIO_TYPE_INT32_LSB20: return 524288.0f;
		case UPH_ASIO_TYPE_INT32_LSB24: return 8388608.0f;
	}

	return 2147483648.0f;
}

static void uph_asio_read_channel(const void *src, const long type, float *dst, const ma_uint32 stride, const long frames)
{
	switch (type)
	{
		case UPH_ASIO_TYPE_INT16_LSB:
		{
			const int16_t *s = (const int16_t*)src;
			for (long f = 0; f < frames; f++)
				dst[f * stride] = (float)s[f] / 32768.0f;
			break;
		}
		case UPH_ASIO_TYPE_INT24_LSB:
		{
			const uint8_t *s = (const uint8_t*)src;
			for (long f = 0; f < frames; f++)
			{
				const uint32_t packed = ((uint32_t)s[f * 3] << 8) | ((uint32_t)s[f * 3 + 1] << 16) | ((uint32_t)s[f * 3 + 2] << 24);
				dst[f * stride] = (float)(int32_t)packed / 2147483648.0f;
			}
			break;
		}
		case UPH_ASIO_TYPE_FLOAT32_LSB:
		{
			const float *s = (const float*)src;
			for (long f = 0; f < frames; f++)
				dst[f * stride] = s[f];
			break;
		}
		case UPH_ASIO_TYPE_FLOAT64_LSB:
		{
			const double *s = (const double*)src;
			for (long f = 0; f < frames; f++)
				dst[f * stride] = (float)s[f];
			break;
		}
		default:
		{
			const int32_t *s = (const int32_t*)src;
			const float scale = 1.0f / uph_asio_int32_scale(type);
			for (long f = 0; f < frames; f++)
				dst[f * stride] = (float)s[f] * scale;
			break;
		}
	}
}

static inline float uph_asio_clamp(const float v)
{
	return v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
}

static void uph_asio_write_channel(void *dst, const long type, const float *src, const ma_uint32 stride, const long frames)
{
	switch (type)
	{
		case UPH_ASIO_TYPE_INT16_LSB:
		{
			int16_t *d = (int16_t*)dst;
			for (long f = 0; f < frames; f++)
				d[f] = (int16_t)lrintf(uph_asio_clamp(src[f * stride]) * 32767.0f);
			break;
		}
		case UPH_ASIO_TYPE_INT24_LSB:
		{
			uint8_t *d = (uint8_t*)dst;
			for (long f = 0; f < frames; f++)
			{
				const int32_t v = (int32_t)lrintf(uph_asio_clamp(src[f * stride]) * 8388607.0f);
				d[f * 3 + 0] = (uint8_t)(v & 0xFF);
				d[f * 3 + 1] = (uint8_t)((v >> 8) & 0xFF);
				d[f * 3 + 2] = (uint8_t)((v >> 16) & 0xFF);
			}
			break;
		}
		case UPH_ASIO_TYPE_FLOAT32_LSB:
		{
			float *d = (float*)dst;
			for (long f = 0; f < frames; f++)
				d[f] = src[f * stride];
			break;
		}
		case UPH_ASIO_TYPE_FLOAT64_LSB:
		{
			double *d = (double*)dst;
			for (long f = 0; f < frames; f++)
				d[f] = (double)src[f * stride];
			break;
		}
		default:
		{
			int32_t *d = (int32_t*)dst;
			const double scale = (double)uph_asio_int32_scale(type) - 1.0;
			for (long f = 0; f < frames; f++)
				d[f] = (int32_t)llrint((double)uph_asio_clamp(src[f * stride]) * scale);
			break;
		}
	}
}

static void uph_asio_process(const long index)
{
	const long frames = uph_asio.buffer_size;
	float *input = uph_asio.input_scratch;
	float *output = uph_asio.output_scratch;

	if (uph_asio.input_count < UPH_ASIO_MAX_CHANNELS)
		memset(input, 0, (size_t)frames * UPH_ASIO_MAX_CHANNELS * sizeof(float));

	for (long c = 0; c < uph_asio.input_count; c++)
		uph_asio_read_channel(uph_asio.buffers[c].buffers[index], uph_asio.input_types[c], input + c, UPH_ASIO_MAX_CHANNELS, frames);

	ma_device_handle_backend_data_callback(uph_asio.device, output, input, (ma_uint32)frames);

	for (long c = 0; c < uph_asio.output_count; c++)
	{
		const ma_uint32 source = c < (long)uph_asio.playback_channels ? (ma_uint32)c : 0;
		const Uph_AsioBufferInfo *info = &uph_asio.buffers[uph_asio.input_count + c];
		uph_asio_write_channel(info->buffers[index], uph_asio.output_types[c], output + source, uph_asio.playback_channels, frames);
	}

	if (uph_asio.post_output)
		uph_asio.driver->vtbl->output_ready(uph_asio.driver);
}

static void uph_asio_on_buffer_switch(long index, Uph_AsioBool direct)
{
	(void)direct;
	uph_asio_process(index);
}

static Uph_AsioTime *uph_asio_on_buffer_switch_time_info(Uph_AsioTime *params, long index, Uph_AsioBool direct)
{
	(void)direct;
	uph_asio_process(index);
	return params;
}

static void uph_asio_on_sample_rate_changed(double rate)
{
	(void)rate;
	atomic_store(&uph_asio.reset_requested, true);
}

static long uph_asio_on_message(long selector, long value, void *message, double *opt)
{
	(void)message;
	(void)opt;

	switch (selector)
	{
		case UPH_ASIO_MSG_SELECTOR_SUPPORTED:
			return value == UPH_ASIO_MSG_ENGINE_VERSION
				|| value == UPH_ASIO_MSG_RESET_REQUEST
				|| value == UPH_ASIO_MSG_BUFFER_SIZE_CHANGE
				|| value == UPH_ASIO_MSG_RESYNC_REQUEST
				|| value == UPH_ASIO_MSG_LATENCIES_CHANGED;
		case UPH_ASIO_MSG_ENGINE_VERSION:
			return 2;
		case UPH_ASIO_MSG_RESET_REQUEST:
		case UPH_ASIO_MSG_BUFFER_SIZE_CHANGE:
			atomic_store(&uph_asio.reset_requested, true);
			return 1;
		case UPH_ASIO_MSG_RESYNC_REQUEST:
		case UPH_ASIO_MSG_LATENCIES_CHANGED:
			return 1;
	}

	return 0;
}

static long uph_asio_pick_buffer_size(const long requested, const long min, const long max, const long preferred, const long granularity)
{
	if (requested <= 0 || granularity == 0 || min >= max)
		return preferred;

	if (granularity == -1)
	{
		long best = min;
		for (long size = min; size <= max && size > 0; size *= 2)
		{
			best = size;
			if (size >= requested)
				break;
		}
		return best;
	}

	const long clamped = requested < min ? min : (requested > max ? max : requested);
	const long steps = (clamped - min + granularity - 1) / granularity;
	const long size = min + steps * granularity;
	return size > max ? max : size;
}

static void uph_asio_log_driver_error(const char *what)
{
	char message[124] = { 0 };
	if (uph_asio.driver)
		uph_asio.driver->vtbl->get_error_message(uph_asio.driver, message);

	fprintf(stderr, "uph_asio: %s%s%s\n", what, message[0] ? ": " : "", message);
}

static void uph_asio_release_device(void)
{
	if (uph_asio.buffers_created)
	{
		uph_asio.driver->vtbl->dispose_buffers(uph_asio.driver);
		uph_asio.buffers_created = false;
	}

	uph_asio_unload();

	free(uph_asio.input_scratch);
	free(uph_asio.output_scratch);
	uph_asio.input_scratch = NULL;
	uph_asio.output_scratch = NULL;
	uph_asio.device = NULL;
}

static ma_result uph_asio_on_context_init(ma_context *context, const ma_context_config *config, ma_backend_callbacks *callbacks)
{
	(void)context;
	(void)config;
	(void)callbacks;
	return MA_SUCCESS;
}

static ma_result uph_asio_on_context_enumerate_devices(ma_context *context, ma_enum_devices_callback_proc callback, void *user_data)
{
	Uph_AsioDriverEntry entries[UPH_ASIO_MAX_DRIVERS];
	const uint32_t count = uph_asio_enumerate(entries);

	for (uint32_t i = 0; i < count; i++)
	{
		ma_device_info info = { 0 };
		strncpy(info.id.custom.s, entries[i].name, sizeof(info.id.custom.s) - 1);
		strncpy(info.name, entries[i].name, sizeof(info.name) - 1);

		if (!callback(context, ma_device_type_playback, &info, user_data))
			break;
	}

	return MA_SUCCESS;
}

static void uph_asio_fill_descriptor(ma_device_descriptor *descriptor, const ma_uint32 channels, const ma_uint32 sample_rate)
{
	descriptor->format = ma_format_f32;
	descriptor->channels = channels;
	descriptor->sampleRate = sample_rate;
	descriptor->periodSizeInFrames = (ma_uint32)uph_asio.buffer_size;
	descriptor->periodCount = 2;
	ma_channel_map_init_standard(ma_standard_channel_map_default, descriptor->channelMap, MA_MAX_CHANNELS, channels);
}

static ma_result uph_asio_on_device_init(ma_device *device, const ma_device_config *config, ma_device_descriptor *playback, ma_device_descriptor *capture)
{
	if (config->deviceType != ma_device_type_duplex)
		return MA_DEVICE_TYPE_NOT_SUPPORTED;

	Uph_AsioDriverEntry entry;
	if (!uph_asio_find_driver(playback->pDeviceID, &entry))
		return MA_NO_DEVICE;

	if (!uph_asio_load(&entry.clsid))
	{
		fprintf(stderr, "uph_asio: failed to load driver '%s'\n", entry.name);
		return MA_FAILED_TO_INIT_BACKEND;
	}

	Uph_AsioDriver *driver = uph_asio.driver;
	if (!driver->vtbl->init(driver, GetDesktopWindow()))
	{
		uph_asio_log_driver_error("failed to init driver");
		uph_asio_release_device();
		return MA_FAILED_TO_INIT_BACKEND;
	}

	long driver_inputs = 0;
	long driver_outputs = 0;
	if (driver->vtbl->get_channels(driver, &driver_inputs, &driver_outputs) != UPH_ASE_OK || driver_outputs <= 0)
	{
		uph_asio_log_driver_error("driver has no usable channels");
		uph_asio_release_device();
		return MA_FAILED_TO_INIT_BACKEND;
	}

	if (driver->vtbl->can_sample_rate(driver, (double)playback->sampleRate) == UPH_ASE_OK)
		driver->vtbl->set_sample_rate(driver, (double)playback->sampleRate);

	double rate = 0.0;
	if (driver->vtbl->get_sample_rate(driver, &rate) != UPH_ASE_OK || rate <= 0.0)
		rate = (double)playback->sampleRate;

	long min_size = 0;
	long max_size = 0;
	long preferred_size = 0;
	long granularity = 0;
	if (driver->vtbl->get_buffer_size(driver, &min_size, &max_size, &preferred_size, &granularity) != UPH_ASE_OK || preferred_size <= 0)
	{
		uph_asio_log_driver_error("failed to query buffer size");
		uph_asio_release_device();
		return MA_FAILED_TO_INIT_BACKEND;
	}

	uph_asio.buffer_size = uph_asio_pick_buffer_size((long)playback->periodSizeInFrames, min_size, max_size, preferred_size, granularity);
	uph_asio.input_count = NAUI_MIN(driver_inputs, UPH_ASIO_MAX_CHANNELS);
	uph_asio.output_count = NAUI_MIN(driver_outputs, UPH_ASIO_MAX_CHANNELS);
	uph_asio.playback_channels = NAUI_MIN(playback->channels, (ma_uint32)uph_asio.output_count);

	long buffer_count = 0;
	for (long c = 0; c < uph_asio.input_count; c++)
		uph_asio.buffers[buffer_count++] = (Uph_AsioBufferInfo){ .is_input = 1, .channel = c };
	for (long c = 0; c < uph_asio.output_count; c++)
		uph_asio.buffers[buffer_count++] = (Uph_AsioBufferInfo){ .is_input = 0, .channel = c };

	uph_asio.callbacks = (Uph_AsioCallbacks){
		.buffer_switch = uph_asio_on_buffer_switch,
		.sample_rate_changed = uph_asio_on_sample_rate_changed,
		.message = uph_asio_on_message,
		.buffer_switch_time_info = uph_asio_on_buffer_switch_time_info
	};

	if (driver->vtbl->create_buffers(driver, uph_asio.buffers, buffer_count, uph_asio.buffer_size, &uph_asio.callbacks) != UPH_ASE_OK)
	{
		uph_asio_log_driver_error("failed to create buffers");
		uph_asio_release_device();
		return MA_FAILED_TO_INIT_BACKEND;
	}
	uph_asio.buffers_created = true;

	for (long i = 0; i < buffer_count; i++)
	{
		Uph_AsioChannelInfo info = { .channel = uph_asio.buffers[i].channel, .is_input = uph_asio.buffers[i].is_input };
		if (driver->vtbl->get_channel_info(driver, &info) != UPH_ASE_OK || !uph_asio_type_supported(info.type))
		{
			fprintf(stderr, "uph_asio: unsupported sample type %ld on channel %ld\n", info.type, info.channel);
			uph_asio_release_device();
			return MA_FORMAT_NOT_SUPPORTED;
		}

		if (i < uph_asio.input_count)
			uph_asio.input_types[i] = info.type;
		else
			uph_asio.output_types[i - uph_asio.input_count] = info.type;
	}

	uph_asio.input_scratch = (float*)malloc((size_t)uph_asio.buffer_size * UPH_ASIO_MAX_CHANNELS * sizeof(float));
	uph_asio.output_scratch = (float*)malloc((size_t)uph_asio.buffer_size * uph_asio.playback_channels * sizeof(float));
	uph_asio_fill_descriptor(capture, UPH_ASIO_MAX_CHANNELS, (ma_uint32)rate);
	uph_asio_fill_descriptor(playback, uph_asio.playback_channels, (ma_uint32)rate);

	uph_asio.post_output = driver->vtbl->output_ready(driver) == UPH_ASE_OK;
	atomic_store(&uph_asio.reset_requested, false);
	uph_asio.device = device;
	return MA_SUCCESS;
}

static ma_result uph_asio_on_device_uninit(ma_device *device)
{
	(void)device;
	uph_asio_release_device();
	return MA_SUCCESS;
}

static ma_result uph_asio_on_device_start(ma_device *device)
{
	(void)device;
	if (uph_asio.driver->vtbl->start(uph_asio.driver) != UPH_ASE_OK)
	{
		uph_asio_log_driver_error("failed to start");
		return MA_FAILED_TO_START_BACKEND_DEVICE;
	}

	return MA_SUCCESS;
}

static ma_result uph_asio_on_device_stop(ma_device *device)
{
	(void)device;
	uph_asio.driver->vtbl->stop(uph_asio.driver);
	return MA_SUCCESS;
}

bool uph_asio_supported(void)
{
	return true;
}

uint32_t uph_asio_list_drivers(Naui_String *names)
{
	Uph_AsioDriverEntry entries[UPH_ASIO_MAX_DRIVERS];
	const uint32_t count = uph_asio_enumerate(entries);

	for (uint32_t i = 0; i < count; i++)
		names[i] = naui_string_from_cstr(entries[i].name);

	return count;
}

void uph_asio_get_backend_callbacks(ma_backend_callbacks *callbacks)
{
	*callbacks = (ma_backend_callbacks){
		.onContextInit = uph_asio_on_context_init,
		.onContextEnumerateDevices = uph_asio_on_context_enumerate_devices,
		.onDeviceInit = uph_asio_on_device_init,
		.onDeviceUninit = uph_asio_on_device_uninit,
		.onDeviceStart = uph_asio_on_device_start,
		.onDeviceStop = uph_asio_on_device_stop
	};
}

bool uph_asio_show_control_panel(void)
{
	if (!uph_asio.driver)
		return false;

	return uph_asio.driver->vtbl->control_panel(uph_asio.driver) == UPH_ASE_OK;
}

bool uph_asio_consume_reset_request(void)
{
	return atomic_exchange(&uph_asio.reset_requested, false);
}

uint32_t uph_asio_input_count(void)
{
	return uph_asio.device ? (uint32_t)uph_asio.input_count : 0;
}

#else

bool uph_asio_supported(void)
{
	return false;
}

uint32_t uph_asio_list_drivers(Naui_String *names)
{
	(void)names;
	return 0;
}

void uph_asio_get_backend_callbacks(ma_backend_callbacks *callbacks)
{
	*callbacks = (ma_backend_callbacks){ 0 };
}

bool uph_asio_show_control_panel(void)
{
	return false;
}

bool uph_asio_consume_reset_request(void)
{
	return false;
}

uint32_t uph_asio_input_count(void)
{
	return 0;
}

#endif
