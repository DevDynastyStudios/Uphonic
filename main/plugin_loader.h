#pragma once
#include "types.h"

void uph_queue_plugin_load(const char *path, UphTrack *track);
void uph_queue_plugin_unload(UphPluginInstance *plugin);
void uph_process_plugins(void);