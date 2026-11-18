typedef struct Uph_PluginInternalHandle_ Uph_PluginInternalHandle;

Naui_PanelID uph_plugin_editor_create(
	Uph_PluginInternalHandle *handle
);

bool uph_plugin_editor_take_keyboard(void);

void uph_plugin_editor_destroy(
	Naui_PanelID panel
);
