#include "panel_manager.h"

struct UphSongTimeline
{
    
};

static UphSongTimeline timeline_data {}; 

static void uph_song_timeline_render(UphPanel* panel)
{

}

UPH_REGISTER_PANEL("Song Timeline", ImGuiWindowFlags_None, uph_song_timeline_render);