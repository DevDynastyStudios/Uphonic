#pragma once
#include "audio_mixer.h"
#include "channel_rack.h"
#include "file_explorer.h"
#include "menu_bar.h"
#include "midi_editor.h"
#include "settings.h"
#include "song_timeline.h"
#include "tempo_tapper.h"

struct UphAudioMixer mixer_data;
struct UphChannelRack channel_data;
struct UphFileExplorer explorer_data;
struct UphMenuBar menu_bar_data;
struct UphMidiEditor midi_data;
struct UphSettings settings_data;
struct UphSongTimeline timeline_data;
struct UphTempoTapper tapper_data;

void uph_panel_init(void)
{

}

void uph_panel_renderer(void)
{
    uph_audio_mixer_render(mixer_data);
    uph_channel_rack_render(channel_data);
    uph_file_explorer_render(explorer_data);
    uph_menu_bar_render(menu_bar_data);
    uph_midi_editor_render(midi_data);
    uph_settings_render(settings_data);
    uph_song_timeline_render(timeline_data);
    uph_tempo_tapper_render(tapper_data);
}