#pragma once

#include "types.h"

void uph_sound_device_initialize(void);
void uph_sound_device_shutdown(void);

void uph_sound_device_all_notes_off(void);

UphSample uph_create_sample_from_file(const char *path);
void uph_destroy_sample(const UphSample *sample);

void uph_export_song_to_wav(const char* output_path);