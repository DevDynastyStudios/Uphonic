#define UPH_ACTION_PATTERN_CREATE "Pattern Create"
#define UPH_ACTION_PATTERN_DELETE "Pattern Delete"
#define UPH_ACTION_PATTERN_RENAME "Pattern Rename"
#define UPH_ACTION_PATTERN_DUPLICATE "Pattern Duplicate"

#define UPH_ACTION_SAMPLE_DELETE "Sample Delete"
#define UPH_ACTION_SAMPLE_RENAME "Sample Rename"
#define UPH_ACTION_SAMPLE_DUPLICATE "Sample Duplicate"

#define UPH_ACTION_TRACK_CREATE "Track Create"
#define UPH_ACTION_TRACK_DELETE "Track Delete"
#define UPH_ACTION_TRACK_AUTOMATION_CREATE "Automation Track Create"
#define UPH_ACTION_TRACK_RENAME "Track Rename"
#define UPH_ACTION_TRACK_COLOR "Track Color"
#define UPH_ACTION_TRACK_MUTE "Track Mute"
#define UPH_ACTION_TRACK_ARM "Track Arm"
#define UPH_ACTION_TRACK_SOLO "Track Solo"
#define UPH_ACTION_TRACK_VOLUME "Track Volume"
#define UPH_ACTION_TRACK_PAN "Track Pan"

#define UPH_ACTION_AUTOMATION_CREATE "Automation Create"
#define UPH_ACTION_AUTOMATION_DELETE "Automation Delete"
#define UPH_ACTION_AUTOMATION_RENAME "Automation Rename"
#define UPH_ACTION_AUTOMATION_DUPLICATE "Automation Duplicate"
#define UPH_ACTION_AUTOMATION_POINT_CREATE "Automation Point Create"
#define UPH_ACTION_AUTOMATION_POINT_DELETE "Automation Point Delete"
#define UPH_ACTION_AUTOMATION_POINT_MOVE "Automation Point Move"

#define UPH_ACTION_BLOCK_CREATE "Block Create"
#define UPH_ACTION_BLOCK_DELETE "Block Delete"
#define UPH_ACTION_BLOCK_MOVE "Block Move"
#define UPH_ACTION_BLOCK_RESIZE "Block Resize"

#define UPH_ACTION_BLOCK_CUT_GROUP "Block Cut"
#define UPH_ACTION_BLOCKS_MOVE_GROUP "Blocks Move"
#define UPH_ACTION_BLOCKS_RESIZE_GROUP "Blocks Resize"
#define UPH_ACTION_BLOCKS_DELETE_GROUP "Blocks Delete"
#define UPH_ACTION_BLOCKS_DUPLICATE_GROUP "Blocks Duplicate"

#define UPH_ACTION_MIDI_NOTE_CREATE "MIDI Note Create"
#define UPH_ACTION_MIDI_NOTE_DELETE "MIDI Note Delete"
#define UPH_ACTION_MIDI_NOTE_MOVE "MIDI Note Move"
#define UPH_ACTION_MIDI_NOTE_RESIZE "MIDI Note Resize"

#define UPH_ACTION_MIDI_NOTE_CUT_GROUP "MIDI Note Cut"
#define UPH_ACTION_MIDI_NOTES_MOVE_GROUP "MIDI Notes Move"
#define UPH_ACTION_MIDI_NOTES_RESIZE_GROUP "MIDI Notes Resize"
#define UPH_ACTION_MIDI_NOTES_DELETE_GROUP "MIDI Notes Delete"
#define UPH_ACTION_MIDI_NOTES_DUPLICATE_GROUP "MIDI Notes Duplicate"

#define UPH_ACTION_TRACK_INVALID (-2)

typedef struct
{
	int32_t parent;
	uint32_t index;
} Uph_ActionTrackRef;

typedef struct
{
	uint32_t resource_index;
	bool resource_exists;
} Uph_ActionResourceCreate;

typedef struct
{
	Uph_ResourceIndex resource_index;
	Naui_String old_name;
	Naui_String new_name;
} Uph_ActionResourceRename;

void uph_action_initialize(void);
void uph_action_shutdown(void);

void uph_action_resource_removed(Uph_ResourceType type, Uph_ResourceIndex index);
void uph_action_resource_restored(Uph_ResourceType type, Uph_ResourceIndex index);

void uph_song_timeline_invalidate_tracks(void);
