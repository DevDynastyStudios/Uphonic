#define UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH 6.0f
#define UPH_MIDI_EDITOR_DEFAULT_NOTE_LENGTH 1.0
#define UPH_MIDI_EDITOR_DEFAULT_NOTE_VELOCITY 100

#define UPH_MIDI_EDITOR_ZOOM_X_MIN 8.0f
#define UPH_MIDI_EDITOR_ZOOM_X_MAX 256.0f
#define UPH_MIDI_EDITOR_ZOOM_Y_MIN 8.0f
#define UPH_MIDI_EDITOR_ZOOM_Y_MAX 60.0f
#define UPH_MIDI_EDITOR_PAN_SPEED 1.0f
#define UPH_MIDI_EDITOR_SCROLL_Y_SPEED 40.0f
#define UPH_MIDI_EDITOR_ZOOM_SPEED 0.1f
#define UPH_MIDI_EDITOR_TOP_RULER_HEIGHT 24

#define UPH_MIDI_EDITOR_INITIAL_TOP_KEY 70

#pragma region Note interaction state

typedef uint8_t Uph_NoteInteractionMode;
enum
{
	UPH_NOTE_INTERACTION_NONE,
	UPH_NOTE_INTERACTION_MOVE,
	UPH_NOTE_INTERACTION_RESIZE_LEFT,
	UPH_NOTE_INTERACTION_RESIZE_RIGHT
};

typedef struct
{
	double initial_drag_beat_offset;
	double initial_start_beat;
	double initial_length_beats;
	uint8_t initial_key_number;
	int32_t initial_drag_key_offset;
	uint32_t note_index;
	Uph_ResourceIndex pattern_index;
	Naui_List(Uph_NoteSnapshot) group_notes;
	bool active;
	bool creating;
	bool group;
	bool additive;
	Uph_NoteInteractionMode mode;
}
Uph_DraggingNoteState;

typedef struct
{
	uint32_t note_index;
	bool active;
}
Uph_HoveredNoteState;

typedef struct
{
	double start_beat;
	float start_content_y;
	bool active;
}
Uph_NoteMarqueeState;

typedef struct
{
	Uph_DraggingNoteState drag;
	Uph_HoveredNoteState hovered_note;
	Uph_NoteMarqueeState marquee;
	double last_note_length;
	double last_note_velocity;
}
Uph_MidiNoteInput;

#pragma endregion

typedef struct
{
	Uph_MidiView view;
	Uph_NoteMap note_map;
	Uph_Piano piano;
	Uph_MidiNoteInput note_input;
	Leaf_BoundingBox lanes_bounding_box;
	Uph_ActionMode current_action_mode;
	Uph_SnapResolution snap_resolution;
	Naui_Vec2 pan_last_mouse;
	bool panning;
	bool lanes_hovered;
	bool panel_hovered;
	bool piano_hovered;
}
Uph_MidiEditorData;

void uph_midi_editor_init(Uph_MidiEditorData *editor);
void uph_midi_editor_update(Uph_MidiEditorData *editor);
void uph_midi_editor_reset_interaction(Uph_MidiEditorData *editor);
