#pragma once

#include "types.h"

void uph_sound_device_initialize(void);
void uph_sound_device_shutdown(void);

void uph_sound_device_instrument_notes_off(UphInstrument *instrument);
void uph_sound_device_all_notes_off(void);