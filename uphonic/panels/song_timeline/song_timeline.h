typedef uint8_t Uph_BlockInteractionMode;
enum
{
	UPH_BLOCK_INTERACTION_NONE,
	UPH_BLOCK_INTERACTION_MOVE,
	UPH_BLOCK_INTERACTION_RESIZE_LEFT,
	UPH_BLOCK_INTERACTION_RESIZE_RIGHT
};

void uph_song_timeline_invalidate_tracks(void);

void uph_song_timeline_emit_block_shockwave(
	const Uph_Track *track,
	const Uph_TimelineBlock *block
);

void uph_song_timeline_reset_scroll(void);

void uph_song_timeline_update_record_track(void);

void uph_song_timeline_render_ruler(
	Leaf_BoundingBox bbox,
	float zoom_x,
	float scroll_x
);

void uph_song_timeline_render_track_timeline(
	Uph_Track *track
);

void uph_song_timeline_open_automation_list(
	Uph_Track *track
);

bool uph_song_timeline_can_automate_last_touched(
	Uph_Track *track
);

void uph_song_timeline_automate_last_touched(
	Uph_Track *track
);

Uph_TimelineBlock uph_song_timeline_block_create(
	double start_beat,
	uint32_t resource_index,
	Uph_ResourceType block_type
);

void uph_song_timeline_begin_block_drag(
	Uph_Track *track,
	uint32_t block_index,
	Uph_BlockInteractionMode mode,
	double mouse_beat
);

void uph_song_timeline_apply_group_drag(
	const Uph_TimelineBlock *grabbed
);

void uph_song_timeline_render_track_timeline_blocks(
	Uph_Track *track
);

void uph_song_timeline_request_block_menu(
	Uph_Track *track,
	uint32_t block_index
);
