extern char **environ;

#define UPH_BRIDGE_CONNECT_TIMEOUT_MS   60000
#define UPH_BRIDGE_LOAD_TIMEOUT_MS      180000
#define UPH_BRIDGE_PROCESS_TIMEOUT_MS   5000
#define UPH_BRIDGE_STATE_TIMEOUT_MS     60000
#define UPH_BRIDGE_INFO_TIMEOUT_MS      60000
#define UPH_BRIDGE_QUIT_TIMEOUT_MS      3000
#define UPH_BRIDGE_POLL_SLICE_MS        100

static atomic_uint s_uph_bridge_counter;

static void _uph_bridge_report(const char *message_key)
{
	if (uph_dialog_is_open())
		return;

	uph_dialog_open(&(Uph_DialogConfig){
		.title = NAUI_TR("plugin.bridge.title"),
		.message = NAUI_TR(message_key),
		.buttons = { NAUI_TR("dialog.ok") },
		.default_button = 0,
		.cancel_button = 0
	});
}

static bool _uph_bridge_is_executable(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISREG(st.st_mode) && access(path, X_OK) == 0;
}

static bool _uph_bridge_find_wine(char *out, size_t out_size)
{
	const char *env_path = getenv("PATH");
	if (!env_path)
		return false;

	const char *cursor = env_path;
	while (*cursor)
	{
		const char *end = strchr(cursor, ':');
		const size_t length = end ? (size_t)(end - cursor) : strlen(cursor);

		if (length > 0)
		{
			snprintf(out, out_size, "%.*s/wine", (int)length, cursor);
			if (_uph_bridge_is_executable(out))
				return true;
		}

		if (!end)
			break;
		cursor = end + 1;
	}

	return false;
}

static bool _uph_bridge_exe_path(char *out, size_t out_size)
{
	char self[PATH_MAX];
	const ssize_t length = readlink("/proc/self/exe", self, sizeof(self) - 1);
	if (length <= 0)
		return false;

	self[length] = '\0';
	char *slash = strrchr(self, '/');
	if (!slash)
		return false;

	*slash = '\0';
	snprintf(out, out_size, "%s/%s", self, UPH_BRIDGE_EXE_NAME);
	return access(out, R_OK) == 0;
}

bool uph_bridge_wine_available(void)
{
	char wine[PATH_MAX];
	return _uph_bridge_find_wine(wine, sizeof(wine));
}

bool uph_bridge_exe_available(void)
{
	char exe[PATH_MAX];
	return _uph_bridge_exe_path(exe, sizeof(exe));
}

static bool _uph_bridge_file_is_pe(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (!file)
		return false;

	char magic[2] = { 0 };
	const bool is_pe = fread(magic, 1, 2, file) == 2 && magic[0] == 'M' && magic[1] == 'Z';
	fclose(file);
	return is_pe;
}

bool uph_bridge_is_windows_plugin(const Naui_Path path)
{
	const Naui_StringView extension = naui_file_extension(&path);
	if (!naui_string_view_equals_cstr(extension, ".vst3", false) && !naui_string_view_equals_cstr(extension, ".clap", false))
		return false;

	struct stat st;
	if (stat(path.data, &st) != 0)
		return false;

	if (!S_ISDIR(st.st_mode))
		return _uph_bridge_file_is_pe(path.data);

	char native[PATH_MAX];
	char resolved[PATH_MAX];
	if (uph_vst3_resolve_binary(path.data, native, sizeof(native)))
		return false;

	snprintf(resolved, sizeof(resolved), "%s/Contents/x86_64-win", path.data);
	return stat(resolved, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool _uph_bridge_prefix_for(const char *path, char *out, size_t out_size)
{
	const char *drive_c = strstr(path, "/drive_c/");
	if (drive_c)
	{
		snprintf(out, out_size, "%.*s", (int)(drive_c - path), path);
		return true;
	}

	const char *env_prefix = getenv("WINEPREFIX");
	if (env_prefix && env_prefix[0])
	{
		snprintf(out, out_size, "%s", env_prefix);
		return true;
	}

	out[0] = '\0';
	return false;
}

static void _uph_bridge_windows_path(const char *unix_path, const char *prefix, char *out, size_t out_size)
{
	char absolute[PATH_MAX];
	if (!realpath(unix_path, absolute))
		snprintf(absolute, sizeof(absolute), "%s", unix_path);

	const size_t prefix_length = prefix ? strlen(prefix) : 0;
	if (prefix_length > 0 && strncmp(absolute, prefix, prefix_length) == 0 && strncmp(absolute + prefix_length, "/drive_c/", 9) == 0)
		snprintf(out, out_size, "C:\\%s", absolute + prefix_length + 9);
	else
		snprintf(out, out_size, "Z:%s", absolute);

	for (char *c = out; *c; c++)
	{
		if (*c == '/')
			*c = '\\';
	}
}

static char **_uph_bridge_build_env(const char *prefix, char *prefix_var, size_t prefix_var_size)
{
	size_t count = 0;
	while (environ[count])
		count++;

	char **env = (char**)calloc(count + 4, sizeof(char*));
	if (!env)
		return NULL;

	size_t n = 0;
	bool has_debug = false;
	bool has_dxvk_log = false;
	for (size_t i = 0; i < count; i++)
	{
		if (strncmp(environ[i], "WINEPREFIX=", 11) == 0)
			continue;

		has_debug |= strncmp(environ[i], "WINEDEBUG=", 10) == 0;
		has_dxvk_log |= strncmp(environ[i], "DXVK_LOG_LEVEL=", 15) == 0;
		env[n++] = environ[i];
	}

	if (!has_debug)
		env[n++] = "WINEDEBUG=-all";

	if (!has_dxvk_log)
		env[n++] = "DXVK_LOG_LEVEL=none";

	if (prefix && prefix[0])
	{
		snprintf(prefix_var, prefix_var_size, "WINEPREFIX=%s", prefix);
		env[n++] = prefix_var;
	}

	env[n] = NULL;
	return env;
}

static pid_t _uph_bridge_spawn(const char *wine, char *const argv[], const char *prefix, int stdout_fd)
{
	char prefix_var[PATH_MAX + 16];
	char **env = _uph_bridge_build_env(prefix, prefix_var, sizeof(prefix_var));
	if (!env)
		return -1;

	const pid_t pid = fork();
	if (pid == 0)
	{
		if (stdout_fd >= 0)
			dup2(stdout_fd, STDOUT_FILENO);

		if (syscall(SYS_close_range, 3, ~0U, 0) != 0)
		{
			for (int fd = 3; fd < 4096; fd++)
				close(fd);
		}

		execve(wine, argv, env);
		_exit(127);
	}

	free(env);
	return pid;
}

static bool _uph_bridge_child_running(pid_t *pid)
{
	if (*pid <= 0)
		return false;

	int status = 0;
	const pid_t result = waitpid(*pid, &status, WNOHANG);
	if (result == 0)
		return true;

	*pid = -1;
	return false;
}

static void _uph_bridge_child_stop(pid_t *pid, uint32_t grace_ms)
{
	for (uint32_t waited = 0; waited < grace_ms; waited += 10)
	{
		if (!_uph_bridge_child_running(pid))
			return;

		naui_thread_sleep_ms(10);
	}

	if (*pid > 0)
	{
		kill(*pid, SIGKILL);
		waitpid(*pid, NULL, 0);
		*pid = -1;
	}
}

static bool _uph_bridge_wait_child_readable(Uph_BridgeSocket sock, pid_t *pid, uint32_t timeout_ms)
{
	for (uint32_t waited = 0; waited < timeout_ms; waited += UPH_BRIDGE_POLL_SLICE_MS)
	{
		if (_uph_bridge_wait_readable(sock, UPH_BRIDGE_POLL_SLICE_MS))
			return true;

		if (!_uph_bridge_child_running(pid))
			return false;
	}

	return false;
}

static void _uph_bridge_close(Uph_BridgeSocket *sock)
{
	if (*sock != UPH_BRIDGE_INVALID_SOCKET)
		close(*sock);

	*sock = UPH_BRIDGE_INVALID_SOCKET;
}

static void _uph_bridge_destroy(Uph_PluginBridge *bridge)
{
	_uph_bridge_close(&bridge->control);
	_uph_bridge_close(&bridge->audio);
	_uph_bridge_child_stop(&bridge->pid, 0);

	if (bridge->shared)
		munmap(bridge->shared, sizeof(Uph_BridgeShared));

	if (bridge->shm_path[0])
		unlink(bridge->shm_path);

	if (bridge->control_lock)
		naui_mutex_destroy(bridge->control_lock);

	free(bridge);
}

static Uph_BridgeSocket _uph_bridge_listen(uint16_t *port)
{
	const Uph_BridgeSocket sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (sock < 0)
		return UPH_BRIDGE_INVALID_SOCKET;

	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;

	socklen_t length = sizeof(addr);
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0 || listen(sock, 2) != 0 || getsockname(sock, (struct sockaddr*)&addr, &length) != 0)
	{
		close(sock);
		return UPH_BRIDGE_INVALID_SOCKET;
	}

	*port = ntohs(addr.sin_port);
	return sock;
}

static bool _uph_bridge_accept_both(Uph_PluginBridge *bridge, Uph_BridgeSocket listener)
{
	while (bridge->control == UPH_BRIDGE_INVALID_SOCKET || bridge->audio == UPH_BRIDGE_INVALID_SOCKET)
	{
		if (!_uph_bridge_wait_child_readable(listener, &bridge->pid, UPH_BRIDGE_CONNECT_TIMEOUT_MS))
			return false;

		const Uph_BridgeSocket client = accept(listener, NULL, NULL);
		if (client < 0)
			return false;

		fcntl(client, F_SETFD, FD_CLOEXEC);

		char tag = 0;
		if (!_uph_bridge_wait_readable(client, UPH_BRIDGE_CONNECT_TIMEOUT_MS) || !_uph_bridge_recv_all(client, &tag, 1))
		{
			close(client);
			return false;
		}

		_uph_bridge_set_nodelay(client);

		Uph_BridgeSocket *slot = tag == UPH_BRIDGE_TAG_CONTROL ? &bridge->control : (tag == UPH_BRIDGE_TAG_AUDIO ? &bridge->audio : NULL);
		if (!slot || *slot != UPH_BRIDGE_INVALID_SOCKET)
		{
			close(client);
			return false;
		}

		*slot = client;
	}

	return true;
}

static bool _uph_bridge_create_shared(Uph_PluginBridge *bridge)
{
	snprintf(bridge->shm_path, sizeof(bridge->shm_path), "/dev/shm/uph-bridge-%d-%u", (int)getpid(), atomic_fetch_add(&s_uph_bridge_counter, 1));

	const int fd = open(bridge->shm_path, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
	if (fd < 0)
	{
		bridge->shm_path[0] = '\0';
		return false;
	}

	if (ftruncate(fd, sizeof(Uph_BridgeShared)) != 0)
	{
		close(fd);
		return false;
	}

	void *memory = mmap(NULL, sizeof(Uph_BridgeShared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (memory == MAP_FAILED)
		return false;

	bridge->shared = (Uph_BridgeShared*)memory;
	uph_note_ring_init(&bridge->shared->notes);
	uph_param_ring_init(&bridge->shared->params);
	return true;
}

static bool _uph_bridge_request(Uph_PluginBridge *bridge, Uph_BridgeOp op, const Uph_BridgeBuffer *payload, Uph_BridgeBuffer *reply, uint32_t timeout_ms)
{
	if (!atomic_load(&bridge->alive))
		return false;

	naui_mutex_lock(bridge->control_lock);

	bool ok = _uph_bridge_send_message(bridge->control, op, payload);
	if (ok && reply)
	{
		Uph_BridgeOp reply_op = 0;
		ok = _uph_bridge_wait_readable(bridge->control, (int32_t)timeout_ms)
			&& _uph_bridge_recv_message(bridge->control, &reply_op, reply)
			&& reply_op == UPH_BRIDGE_OP_REPLY;
	}

	naui_mutex_unlock(bridge->control_lock);

	if (!ok)
		atomic_store(&bridge->alive, false);

	return ok;
}

static void _uph_bridge_attach_window(uint64_t x11_window)
{
	const Window parent = (Window)mgapp_primary_handle();
	if (!x11_window || !parent)
		return;

	static Display *display;
	if (!display)
		display = XOpenDisplay(NULL);

	if (!display)
		return;

	XSetTransientForHint(display, (Window)x11_window, parent);
	XSync(display, False);
}

Uph_Plugin uph_bridge_load(Naui_Path path, bool show_window)
{
	Uph_Plugin plug = { 0 };
	plug.file_path = path;
	plug.format = naui_string_view_equals_cstr(naui_file_extension(&path), ".clap", false) ? UPH_PLUGIN_CLAP : UPH_PLUGIN_VST3;

	char wine[PATH_MAX];
	if (!_uph_bridge_find_wine(wine, sizeof(wine)))
	{
		_uph_bridge_report("plugin.bridge.wine_missing");
		return plug;
	}

	char exe[PATH_MAX];
	if (!_uph_bridge_exe_path(exe, sizeof(exe)))
	{
		_uph_bridge_report("plugin.bridge.exe_missing");
		return plug;
	}

	Uph_PluginBridge *bridge = (Uph_PluginBridge*)calloc(1, sizeof(*bridge));
	if (!bridge)
		return plug;

	bridge->control = UPH_BRIDGE_INVALID_SOCKET;
	bridge->audio = UPH_BRIDGE_INVALID_SOCKET;
	bridge->pid = -1;
	bridge->control_lock = naui_mutex_create();

	uint16_t port = 0;
	Uph_BridgeSocket listener = UPH_BRIDGE_INVALID_SOCKET;
	Uph_BridgeBuffer reply = { 0 };
	uint64_t x11_window = 0;
	bool ok = _uph_bridge_create_shared(bridge);

	if (ok)
	{
		listener = _uph_bridge_listen(&port);
		ok = listener != UPH_BRIDGE_INVALID_SOCKET;
	}

	if (ok)
	{
		char prefix[PATH_MAX];
		_uph_bridge_prefix_for(path.data, prefix, sizeof(prefix));

		char port_arg[16], rate_arg[32];
		char shm_arg[PATH_MAX], plugin_arg[PATH_MAX];
		snprintf(port_arg, sizeof(port_arg), "%u", (unsigned)port);
		snprintf(rate_arg, sizeof(rate_arg), "%u", (unsigned)uph_state.settings.audio.sample_rate);
		_uph_bridge_windows_path(bridge->shm_path, NULL, shm_arg, sizeof(shm_arg));
		_uph_bridge_windows_path(path.data, prefix, plugin_arg, sizeof(plugin_arg));

		char *argv[] = {
			wine, exe,
			"--host", port_arg,
			"--shm", shm_arg,
			"--plugin", plugin_arg,
			"--rate", rate_arg,
			NULL
		};

		bridge->pid = _uph_bridge_spawn(wine, argv, prefix, -1);
		ok = bridge->pid > 0 && _uph_bridge_accept_both(bridge, listener);
	}

	if (listener != UPH_BRIDGE_INVALID_SOCKET)
		close(listener);

	if (ok)
	{
		Uph_BridgeOp op = 0;
		ok = _uph_bridge_wait_child_readable(bridge->control, &bridge->pid, UPH_BRIDGE_LOAD_TIMEOUT_MS)
			&& _uph_bridge_recv_message(bridge->control, &op, &reply)
			&& op == UPH_BRIDGE_OP_LOADED
			&& _uph_bridge_read_u32(&reply) != 0;
	}

	if (ok)
	{
		plug.name = _uph_bridge_read_string(&reply);
		plug.params = _uph_bridge_read_params(&reply);
		x11_window = _uph_bridge_read_u64(&reply);
		ok = !reply.failed;
	}

	_uph_bridge_buffer_free(&reply);

	if (bridge->shm_path[0])
	{
		unlink(bridge->shm_path);
		bridge->shm_path[0] = '\0';
	}

	if (!ok)
	{
		naui_list_free(plug.params);
		plug.params = NULL;
		_uph_bridge_destroy(bridge);
		_uph_bridge_report("plugin.bridge.failed");
		return plug;
	}

	atomic_store(&bridge->alive, true);
	_uph_bridge_attach_window(x11_window);

	plug.internal_handle = bridge;
	plug.bridged = true;
	plug.loaded = true;

	if (show_window)
		uph_bridge_show_window(&plug, true);

	return plug;
}

void uph_bridge_unload(Uph_Plugin *plug)
{
	Uph_PluginBridge *bridge = (Uph_PluginBridge*)plug->internal_handle;
	if (!bridge)
		return;

	if (atomic_load(&bridge->alive))
		_uph_bridge_request(bridge, UPH_BRIDGE_OP_QUIT, NULL, NULL, 0);

	_uph_bridge_child_stop(&bridge->pid, UPH_BRIDGE_QUIT_TIMEOUT_MS);
	_uph_bridge_destroy(bridge);

	plug->internal_handle = NULL;
}

void uph_bridge_update(Uph_Plugin *plug)
{
	Uph_PluginBridge *bridge = (Uph_PluginBridge*)plug->internal_handle;
	if (!bridge)
		return;

	if (atomic_load(&bridge->alive) && !_uph_bridge_child_running(&bridge->pid))
		atomic_store(&bridge->alive, false);

	if (!atomic_load(&bridge->alive) && !bridge->crash_reported)
	{
		bridge->crash_reported = true;
		atomic_store(&bridge->shared->visible, 0);
		fprintf(stderr, "uph bridge: %s stopped responding\n", plug->name.data);
		_uph_bridge_report("plugin.bridge.crashed");
	}
}

void uph_bridge_show_window(Uph_Plugin *plug, bool show)
{
	Uph_PluginBridge *bridge = (Uph_PluginBridge*)plug->internal_handle;
	if (!bridge)
		return;

	atomic_store(&bridge->shared->visible, show ? 1 : 0);
	_uph_bridge_request(bridge, show ? UPH_BRIDGE_OP_SHOW : UPH_BRIDGE_OP_HIDE, NULL, NULL, 0);
}

bool uph_bridge_window_visible(const Uph_Plugin *plug)
{
	const Uph_PluginBridge *bridge = (const Uph_PluginBridge*)plug->internal_handle;
	return bridge && atomic_load(&bridge->shared->visible) != 0;
}

void uph_bridge_process(Uph_Plugin *plug, float **inputs, float **outputs, uint32_t frame_count, double playhead_beat, bool is_playing)
{
	Uph_PluginBridge *bridge = (Uph_PluginBridge*)plug->internal_handle;
	const uint32_t frames = frame_count < UPH_BRIDGE_MAX_FRAMES ? frame_count : UPH_BRIDGE_MAX_FRAMES;

	if (!bridge || !atomic_load(&bridge->alive))
	{
		for (uint32_t ch = 0; ch < UPH_BRIDGE_CHANNELS; ch++)
			memset(outputs[ch], 0, frame_count * sizeof(float));
		return;
	}

	Uph_BridgeShared *shared = bridge->shared;
	shared->frame_count = frames;
	shared->is_playing = is_playing ? 1 : 0;
	shared->playhead_beat = playhead_beat;
	shared->tempo = uph_tempo_current_bpm();

	for (uint32_t ch = 0; ch < UPH_BRIDGE_CHANNELS; ch++)
		memcpy(shared->inputs[ch], inputs[ch], frames * sizeof(float));

	atomic_thread_fence(memory_order_release);

	char ping = UPH_BRIDGE_PING;
	const bool ok = _uph_bridge_send_all(bridge->audio, &ping, 1)
		&& _uph_bridge_wait_readable(bridge->audio, UPH_BRIDGE_PROCESS_TIMEOUT_MS)
		&& _uph_bridge_recv_all(bridge->audio, &ping, 1);

	atomic_thread_fence(memory_order_acquire);

	if (!ok)
	{
		atomic_store(&bridge->alive, false);
		for (uint32_t ch = 0; ch < UPH_BRIDGE_CHANNELS; ch++)
			memset(outputs[ch], 0, frame_count * sizeof(float));
		return;
	}

	for (uint32_t ch = 0; ch < UPH_BRIDGE_CHANNELS; ch++)
	{
		memcpy(outputs[ch], shared->outputs[ch], frames * sizeof(float));
		if (frame_count > frames)
			memset(outputs[ch] + frames, 0, (frame_count - frames) * sizeof(float));
	}
}

static bool _uph_bridge_state_request(Uph_PluginBridge *bridge, Uph_BridgeOp op, const Naui_Path path, Uph_BridgeBuffer *reply)
{
	char windows_path[PATH_MAX];
	_uph_bridge_windows_path(path.data, NULL, windows_path, sizeof(windows_path));

	Uph_BridgeBuffer payload = { 0 };
	_uph_bridge_write_cstr(&payload, windows_path);

	const bool ok = !payload.failed
		&& _uph_bridge_request(bridge, op, &payload, reply, UPH_BRIDGE_STATE_TIMEOUT_MS)
		&& _uph_bridge_read_u32(reply) != 0;

	_uph_bridge_buffer_free(&payload);
	return ok;
}

bool uph_bridge_save_state(const Uph_Plugin *plug, const Naui_Path path)
{
	Uph_PluginBridge *bridge = (Uph_PluginBridge*)plug->internal_handle;
	if (!bridge)
		return false;

	Uph_BridgeBuffer reply = { 0 };
	const bool ok = _uph_bridge_state_request(bridge, UPH_BRIDGE_OP_SAVE_STATE, path, &reply);
	_uph_bridge_buffer_free(&reply);
	return ok;
}

bool uph_bridge_load_state(Uph_Plugin *plug, const Naui_Path path)
{
	Uph_PluginBridge *bridge = (Uph_PluginBridge*)plug->internal_handle;
	if (!bridge)
		return false;

	Uph_BridgeBuffer reply = { 0 };
	bool ok = _uph_bridge_state_request(bridge, UPH_BRIDGE_OP_LOAD_STATE, path, &reply);
	if (ok)
	{
		Naui_List(Uph_PluginParam) params = _uph_bridge_read_params(&reply);
		if (!reply.failed)
		{
			naui_list_free(plug->params);
			plug->params = params;
		}
		else
			naui_list_free(params);
	}

	_uph_bridge_buffer_free(&reply);
	return ok;
}

static void _uph_bridge_info_fallback(Naui_Path path, Uph_PluginInfo *out)
{
	const char *base = strrchr(path.data, '/');
	base = base ? base + 1 : path.data;

	char stem[NAUI_STRING_MAX_SIZE];
	snprintf(stem, sizeof(stem), "%s", base);
	char *dot = strrchr(stem, '.');
	if (dot)
		*dot = '\0';

	out->path = path;
	out->name = naui_string_from_cstr(stem);
	out->vendor = naui_string_from_cstr("");
	out->type = UPH_PLUGIN_EFFECT;
	out->format = naui_string_view_equals_cstr(naui_file_extension(&path), ".clap", false) ? UPH_PLUGIN_CLAP : UPH_PLUGIN_VST3;
}

static bool _uph_bridge_parse_info(char *output, Naui_Path path, Uph_PluginInfo *out)
{
	char *line = strstr(output, UPH_BRIDGE_INFO_PREFIX);
	if (!line)
		return false;

	line += strlen(UPH_BRIDGE_INFO_PREFIX);
	char *fields[4] = { 0 };
	for (int i = 0; i < 4; i++)
	{
		fields[i] = line;
		line = strpbrk(line, i < 3 ? "\t" : "\r\n");
		if (!line && i < 3)
			return false;

		if (line)
			*line++ = '\0';
	}

	out->path = path;
	out->name = naui_string_from_cstr(fields[0]);
	out->vendor = naui_string_from_cstr(fields[1]);
	out->type = strcmp(fields[2], "INST") == 0 ? UPH_PLUGIN_INSTRUMENT : UPH_PLUGIN_EFFECT;
	out->format = strcmp(fields[3], "CLAP") == 0 ? UPH_PLUGIN_CLAP : UPH_PLUGIN_VST3;
	return true;
}

bool uph_bridge_get_info(Naui_Path path, Uph_PluginInfo *out)
{
	char wine[PATH_MAX];
	char exe[PATH_MAX];
	if (!_uph_bridge_find_wine(wine, sizeof(wine)) || !_uph_bridge_exe_path(exe, sizeof(exe)))
	{
		fprintf(stderr, "uph bridge: wine or %s missing, listing %s without details\n", UPH_BRIDGE_EXE_NAME, path.data);
		_uph_bridge_info_fallback(path, out);
		return true;
	}

	int pipe_fds[2];
	if (pipe(pipe_fds) != 0)
		return false;

	fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
	fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);

	char prefix[PATH_MAX];
	char plugin_arg[PATH_MAX];
	_uph_bridge_prefix_for(path.data, prefix, sizeof(prefix));
	_uph_bridge_windows_path(path.data, prefix, plugin_arg, sizeof(plugin_arg));

	char *argv[] = { wine, exe, "--info", plugin_arg, NULL };
	pid_t pid = _uph_bridge_spawn(wine, argv, prefix, pipe_fds[1]);
	close(pipe_fds[1]);

	char output[4096];
	size_t length = 0;
	for (uint32_t waited = 0; pid > 0 && waited < UPH_BRIDGE_INFO_TIMEOUT_MS && length < sizeof(output) - 1; )
	{
		if (!_uph_bridge_wait_readable(pipe_fds[0], UPH_BRIDGE_POLL_SLICE_MS))
		{
			waited += UPH_BRIDGE_POLL_SLICE_MS;
			continue;
		}

		const ssize_t received = read(pipe_fds[0], output + length, sizeof(output) - 1 - length);
		if (received <= 0)
			break;

		length += (size_t)received;
	}

	output[length] = '\0';
	close(pipe_fds[0]);
	_uph_bridge_child_stop(&pid, UPH_BRIDGE_QUIT_TIMEOUT_MS);

	if (_uph_bridge_parse_info(output, path, out))
		return true;

	fprintf(stderr, "uph bridge: could not read plugin info for %s\n", path.data);
	return false;
}
