typedef uint8_t Uph_PluginCommandKind;
enum
{
	UPH_PLUGIN_COMMAND_LOAD,
	UPH_PLUGIN_COMMAND_UNLOAD,
	UPH_PLUGIN_COMMAND_SHOW,
	UPH_PLUGIN_COMMAND_HIDE,
	UPH_PLUGIN_COMMAND_SAVE_STATE,
	UPH_PLUGIN_COMMAND_SYNC,
	UPH_PLUGIN_COMMAND_MOUSE_MOVE,
	UPH_PLUGIN_COMMAND_MOUSE_BUTTON,
	UPH_PLUGIN_COMMAND_MOUSE_SCROLL,
	UPH_PLUGIN_COMMAND_KEY,
	UPH_PLUGIN_COMMAND_TEXT,
	UPH_PLUGIN_COMMAND_NATIVE_INPUT
};

typedef uint8_t Uph_PluginModifiers;
enum
{
	UPH_PLUGIN_MODIFIER_SHIFT = 1 << 0,
	UPH_PLUGIN_MODIFIER_CONTROL = 1 << 1,
	UPH_PLUGIN_MODIFIER_ALT = 1 << 2,
	UPH_PLUGIN_MODIFIER_SUPER = 1 << 3
};

typedef struct
{
	Uph_PluginCommandKind kind;
	Uph_PluginInternalHandle *handle;
	Uph_PluginLoad *load;
	const Naui_Path *path;
	bool *result;
	int32_t x, y;
	int32_t delta;
	Naui_MouseButton button;
	Naui_Key key;
	Uph_PluginModifiers modifiers;
	uint32_t codepoint;
	bool down;
}
Uph_PluginCommand;

void uph_plugin_thread_init(void);

void uph_plugin_thread_shutdown(void);

void uph_plugin_thread_frame(void);

void uph_plugin_thread_push(
	Uph_PluginCommand command
);

void uph_plugin_thread_call(
	Uph_PluginCommand command
);
