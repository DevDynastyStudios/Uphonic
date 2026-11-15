void uph_midi_velocity_init(Uph_VelocityLane *lane);
void uph_midi_velocity_cancel_drag(Uph_VelocityLane *lane);
void uph_midi_velocity_update_splitter(Uph_MidiEditorState *editor, bool handle_hovered);
void uph_midi_velocity_update(Uph_MidiEditorState *editor, Leaf_BoundingBox box, Uph_MidiPattern *pattern);
void uph_midi_velocity_render(const Uph_MidiEditorState *editor, Leaf_BoundingBox box, const Uph_MidiPattern *pattern);
void uph_midi_velocity_render_header(Leaf_BoundingBox box);
