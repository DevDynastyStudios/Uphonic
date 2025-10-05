#pragma once
#include "types.h"

void uph_queue_instrument_load(const char *path, uint32_t track_index);
void uph_queue_instrument_unload(uint32_t track_index);
void uph_process_plugin_loader(void);