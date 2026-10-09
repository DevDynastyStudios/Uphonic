#define UPH_MIDI_GRID_SCALE_TINT 0.10f
#define UPH_MIDI_GRID_ROOT_TINT 0.22f

void uph_midi_grid_render_lanes(Leaf_BoundingBox box, const Uph_MidiView *view, const Uph_NoteMap *map, const Uph_Piano *piano);
void uph_midi_grid_render_beats(Leaf_BoundingBox box, const Uph_MidiView *view, Uph_SnapResolution snap);
void uph_midi_grid_render_ruler(Leaf_BoundingBox box, const Uph_MidiView *view, Uph_SnapResolution snap);
