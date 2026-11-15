void uph_midi_pedal_init(Uph_PedalLane *lane);
void uph_midi_pedal_cancel_drag(Uph_PedalLane *lane);
void uph_midi_pedal_update(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern);
void uph_midi_pedal_render(const Uph_MidiEditorState *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern);
void uph_midi_pedal_render_header(Uph_MidiEditorState *editor, Leaf_BoundingBox box);
