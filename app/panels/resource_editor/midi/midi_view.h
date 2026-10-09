typedef struct
{
	Naui_Vec2 scroll;
	Naui_Vec2 zoom;
}
Uph_MidiView;

static inline float uph_midi_view_lane_height(const Uph_MidiView *view)
{
	return view->zoom.y * 7.0f / 12.0f;
}

static inline float uph_midi_view_content_height(const Uph_MidiView *view, const Uph_NoteMap *map)
{
	return (float)uph_note_map_row_count(map) * uph_midi_view_lane_height(view);
}

static inline float uph_midi_view_max_scroll_y(const Uph_MidiView *view, const Uph_NoteMap *map, float viewport_height)
{
	return fmaxf(0.0f, uph_midi_view_content_height(view, map) - viewport_height);
}

static inline double uph_midi_view_x_to_beat(const Uph_MidiView *view, Leaf_BoundingBox box, float x)
{
	return ((double)x - box.x + view->scroll.x) / view->zoom.x;
}

static inline float uph_midi_view_beat_to_x(const Uph_MidiView *view, Leaf_BoundingBox box, double beat)
{
	return box.x + (float)(beat * view->zoom.x) - view->scroll.x;
}

static inline float uph_midi_view_row_to_y(const Uph_MidiView *view, Leaf_BoundingBox box, int32_t row)
{
	return box.y + (float)row * uph_midi_view_lane_height(view) - view->scroll.y;
}

static inline int32_t uph_midi_view_y_to_row(const Uph_MidiView *view, Leaf_BoundingBox box, float y)
{
	return (int32_t)floor(((double)y - box.y + view->scroll.y) / uph_midi_view_lane_height(view));
}

static inline Naui_Vec4 uph_midi_view_note_box(const Uph_MidiView *view, const Uph_NoteMap *map, Leaf_BoundingBox box, const Uph_MidiNote *note)
{
	return (Naui_Vec4) {
		uph_midi_view_beat_to_x(view, box, note->start_beat),
		uph_midi_view_row_to_y(view, box, uph_note_map_row(map, note->key_number)),
		(float)(note->length_beats * view->zoom.x),
		uph_midi_view_lane_height(view)
	};
}

static inline Uph_PianoCanvas uph_midi_view_piano_canvas(const Uph_MidiView *view, Leaf_BoundingBox box)
{
	return (Uph_PianoCanvas) { box, uph_midi_view_lane_height(view), view->scroll.y };
}
