#pragma once

void uph_audio_engine_init(void);
void uph_audio_engine_shutdown(void);

Uph_Sample uph_audio_engine_load_sample(Naui_Path path);
void uph_audio_engine_unload_sample(Uph_Sample *sample);