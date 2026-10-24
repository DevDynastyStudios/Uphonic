#define UPH_ACTION_PATTERN_CREATE "Pattern Create"
#define UPH_ACTION_PATTERN_DELETE "Pattern Delete"
#define UPH_ACTION_PATTERN_RENAME "Pattern Rename"
#define UPH_ACTION_PATTERN_DUPLICATE "Pattern Duplicate"

#define UPH_ACTION_SAMPLE_DELETE "Sample Delete"
#define UPH_ACTION_SAMPLE_RENAME "Sample Rename"
#define UPH_ACTION_SAMPLE_DUPLICATE "Sample Duplicate"

#define UPH_ACTION_RESOURCES_DELETE_GROUP "Resources Delete"
#define UPH_ACTION_RESOURCES_DUPLICATE_GROUP "Resources Duplicate"

#define UPH_ACTION_TRACK_CREATE "Track Create"
#define UPH_ACTION_TRACK_DELETE "Track Delete"
#define UPH_ACTION_TRACKS_DELETE_GROUP "Tracks Delete"
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
#define UPH_ACTION_MIDI_NOTE_VELOCITY "MIDI Note Velocity"
#define UPH_ACTION_MIDI_NOTES_ERASE "MIDI Notes Erase"

#define UPH_ACTION_MIDI_NOTE_CUT_GROUP "MIDI Note Cut"
#define UPH_ACTION_MIDI_NOTES_MOVE_GROUP "MIDI Notes Move"
#define UPH_ACTION_MIDI_NOTES_RESIZE_GROUP "MIDI Notes Resize"
#define UPH_ACTION_MIDI_NOTES_DELETE_GROUP "MIDI Notes Delete"
#define UPH_ACTION_MIDI_NOTES_DUPLICATE_GROUP "MIDI Notes Duplicate"
#define UPH_ACTION_MIDI_NOTES_VELOCITY_GROUP "MIDI Notes Velocity"

#define UPH_ACTION_TRACK_MAX_DEPTH 16

typedef struct
{
	uint32_t path[UPH_ACTION_TRACK_MAX_DEPTH];
	uint8_t depth;
	bool master;
} Uph_ActionTrackRef;

static inline Uph_ActionTrackRef uph_action_track_ref_root(void)
{
	return (Uph_ActionTrackRef){ 0 };
}

static inline Uph_ActionTrackRef uph_action_track_ref_master(void)
{
	return (Uph_ActionTrackRef){ .master = true };
}

static inline bool uph_action_track_ref_is_master(Uph_ActionTrackRef ref)
{
	return ref.master;
}

static inline bool uph_action_track_ref_valid(Uph_ActionTrackRef ref)
{
	return ref.master || ref.depth > 0;
}

static inline uint32_t uph_action_track_ref_index(Uph_ActionTrackRef ref)
{
	return ref.depth > 0 ? ref.path[ref.depth - 1] : 0;
}

static inline Uph_ActionTrackRef uph_action_track_ref_child(Uph_ActionTrackRef owner, uint32_t index)
{
	if (owner.master || owner.depth >= UPH_ACTION_TRACK_MAX_DEPTH)
		return uph_action_track_ref_root();

	owner.path[owner.depth++] = index;
	return owner;
}

static inline Uph_ActionTrackRef uph_action_track_ref_top(uint32_t index)
{
	return uph_action_track_ref_child(uph_action_track_ref_root(), index);
}

static inline Uph_ActionTrackRef uph_action_track_ref_owner(Uph_ActionTrackRef ref)
{
	if (ref.depth > 0)
		ref.depth--;
	return ref;
}

static inline bool uph_action_track_ref_has_prefix(Uph_ActionTrackRef ref, Uph_ActionTrackRef prefix)
{
	if (ref.master || prefix.master || ref.depth < prefix.depth)
		return false;

	for (uint8_t i = 0; i < prefix.depth; i++)
	{
		if (ref.path[i] != prefix.path[i])
			return false;
	}
	return true;
}

static inline bool uph_action_track_ref_equal(Uph_ActionTrackRef a, Uph_ActionTrackRef b)
{
	if (a.master || b.master)
		return a.master == b.master;

	return a.depth == b.depth && uph_action_track_ref_has_prefix(a, b);
}

static inline int uph_action_track_ref_compare(Uph_ActionTrackRef a, Uph_ActionTrackRef b)
{
	const uint8_t shared = a.depth < b.depth ? a.depth : b.depth;
	for (uint8_t i = 0; i < shared; i++)
	{
		if (a.path[i] != b.path[i])
			return a.path[i] < b.path[i] ? -1 : 1;
	}

	if (a.depth != b.depth)
		return a.depth < b.depth ? -1 : 1;
	return 0;
}

static inline Uph_ActionTrackRef uph_action_track_ref_after_insert(Uph_ActionTrackRef ref, Uph_ActionTrackRef inserted)
{
	if (inserted.depth == 0)
		return ref;

	const Uph_ActionTrackRef owner = uph_action_track_ref_owner(inserted);
	if (ref.depth <= owner.depth || !uph_action_track_ref_has_prefix(ref, owner))
		return ref;

	if (ref.path[owner.depth] >= inserted.path[owner.depth])
		ref.path[owner.depth]++;
	return ref;
}

static inline Uph_ActionTrackRef uph_action_track_ref_after_remove(Uph_ActionTrackRef ref, Uph_ActionTrackRef removed)
{
	if (removed.depth == 0)
		return ref;

	const Uph_ActionTrackRef owner = uph_action_track_ref_owner(removed);
	if (ref.depth <= owner.depth || !uph_action_track_ref_has_prefix(ref, owner))
		return ref;

	if (ref.path[owner.depth] == removed.path[owner.depth])
		return uph_action_track_ref_root();
	if (ref.path[owner.depth] > removed.path[owner.depth])
		ref.path[owner.depth]--;
	return ref;
}

static inline Uph_ActionTrackRef uph_action_track_ref_after_move(Uph_ActionTrackRef ref, Uph_ActionTrackRef src, Uph_ActionTrackRef dst)
{
	if (src.depth == 0 || !uph_action_track_ref_valid(ref) || ref.master)
		return ref;

	if (uph_action_track_ref_has_prefix(ref, src))
	{
		Uph_ActionTrackRef moved = dst;
		for (uint8_t i = src.depth; i < ref.depth; i++)
			moved = uph_action_track_ref_child(moved, ref.path[i]);
		return moved;
	}

	return uph_action_track_ref_after_insert(uph_action_track_ref_after_remove(ref, src), dst);
}

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

Uph_ActionTrackRef uph_action_track_ref(const Uph_Track *track);
Uph_Track *uph_action_track_resolve(Uph_ActionTrackRef ref);
Uph_Track *uph_action_track_resolve_in(Naui_List(Uph_Track) tracks, Uph_ActionTrackRef ref);

void uph_action_resource_removed(Uph_ResourceType type, Uph_ResourceIndex index);
void uph_action_resource_restored(Uph_ResourceType type, Uph_ResourceIndex index);

void uph_song_timeline_invalidate_tracks(void);