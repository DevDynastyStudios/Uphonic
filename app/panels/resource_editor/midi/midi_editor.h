#define UPH_MIDI_EDITOR_RESIZE_HANDLE_WIDTH 6.0f
#define UPH_MIDI_EDITOR_CLICK_THRESHOLD 3.0f
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

#define UPH_MIDI_EDITOR_VELOCITY_LANE_HEIGHT 110.0f
#define UPH_MIDI_EDITOR_VELOCITY_LANE_MIN_HEIGHT 48.0f
#define UPH_MIDI_EDITOR_VELOCITY_LANE_MAX_HEIGHT 400.0f
#define UPH_MIDI_EDITOR_VELOCITY_SPLITTER_HEIGHT 5.0f
#define UPH_MIDI_EDITOR_VELOCITY_HANDLE_RADIUS 4.0f
#define UPH_MIDI_EDITOR_VELOCITY_PADDING 6.0f
#define UPH_MIDI_EDITOR_VELOCITY_MIN 1
#define UPH_MIDI_EDITOR_VELOCITY_MAX 127

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
	Naui_Vec2 initial_mouse;
	bool active;
	bool moved;
	bool creating;
	bool group;
	bool additive;
	bool select_tool;
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

#pragma region Velocity lane state

typedef uint8_t Uph_VelocityDragMode;
enum
{
	UPH_VELOCITY_DRAG_NONE,
	UPH_VELOCITY_DRAG_HANDLE,
	UPH_VELOCITY_DRAG_PAINT
};

typedef struct
{
	Naui_List(Uph_NoteSnapshot) originals;
	Uph_ResourceIndex pattern_index;
	uint32_t grabbed_index;
	int32_t grab_mouse_velocity;
	float last_mouse_x;
	Uph_VelocityDragMode mode;
}
Uph_VelocityDragState;

typedef struct
{
	Uph_VelocityDragState drag;
	float height;
	float resize_start_mouse_y;
	float resize_start_height;
	int32_t hovered_note;
	bool resizing;
	bool visible;
	bool hovered;
	bool splitter_hovered;
}
Uph_VelocityLane;

#pragma endregion

typedef struct
{
	Uph_MidiView view;
	Uph_NoteMap note_map;
	Uph_Piano piano;
	Uph_MidiNoteInput note_input;
	Uph_VelocityLane velocity_lane;
	Leaf_BoundingBox lanes_bounding_box;
	Uph_ActionMode current_action_mode;
	Uph_SnapResolution snap_resolution;
	Naui_Vec2 pan_last_mouse;
	Uph_ResourceIndex playing_pattern;
	double playhead_drag_beat;
	int16_t audition_key;
	bool panning;
	bool playhead_dragging;
	bool lanes_hovered;
	bool panel_hovered;
	bool piano_hovered;
	bool panel_focused;
}
Uph_MidiEditorData;

void uph_midi_editor_init(void);
void uph_midi_editor_update(void);
void uph_midi_editor_reset_interaction(void);
void uph_midi_editor_toggle_playback(void);
void uph_midi_editor_stop_playback(void);
