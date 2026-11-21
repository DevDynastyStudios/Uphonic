#define UPH_SCALE_CHROMATIC_MASK 0xFFF

#define UPH_MIXER_DB_MIN -60.0f
#define UPH_MIXER_DB_MAX 6.0f

#define UPH_TEMPO_MIN_BPM 20.0f
#define UPH_TEMPO_MAX_BPM 1000.0f

#if NAUI_WINDOWS
    #define UPH_VST2_EXTENSION ".dll"
#else
    #define UPH_VST2_EXTENSION ".so"
#endif

#define UPH_WAVEFORM_BIN_FRAMES 256

#define UPH_MIDI_PPQ 960

#define UPH_DRAG_TYPE_SAMPLE_FILE "uph_drag_sample_file"
#define UPH_DRAG_TYPE_RESOURCE "uph_drag_resource"
