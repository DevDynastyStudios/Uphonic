#define UPH_MIDI_PPQ 960

Uph_MidiPattern uph_midi_convert_to_pattern(const Naui_Path midi_path);
cmidi_file_t* uph_midi_convert_to_midi(const Uph_MidiPattern* pattern);
void uph_midi_on_input(const cmidi_event_t* event, void* userdata);