#include "panel_manager.h"
#include "types.h"

struct UphSongTimeline
{
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    float zoomX  = 16.0f;
    float zoomY  = 2.0f;

    float song_position = 0.0f;

    // Drag state
    bool dragging = false;
    bool resizing = false;
    bool is_playing = false;
    int resizeSide = 0; // -1 = left, +1 = right
    UphMidiPatternInstance* draggedPattern = nullptr;
    UphTrack* draggedTrack = nullptr;
    float dragOffset = 0.0f;
};

static UphSongTimeline timeline_data {}; 

static void uph_song_timeline_render(UphPanel* panel)
{
    
}

UPH_REGISTER_PANEL("Song Timeline", ImGuiWindowFlags_None, ImGuiDockNodeFlags_None, uph_song_timeline_render);