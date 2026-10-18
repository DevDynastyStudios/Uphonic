#define UPH_SONG_TIMELINE_ZOOM_X_MIN 8.0f
#define UPH_SONG_TIMELINE_ZOOM_X_MAX 256.0f
#define UPH_SONG_TIMELINE_PAN_SPEED 1.0f
#define UPH_SONG_TIMELINE_SCROLL_Y_SPEED 40.0f
#define UPH_SONG_TIMELINE_ZOOM_SPEED 0.1f
#define UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH 6.0f
#define UPH_SONG_TIMELINE_DEPTH_INDENT 12.0f

typedef uint8_t Uph_BlockInteractionMode;
enum
{
	UPH_BLOCK_INTERACTION_NONE,
	UPH_BLOCK_INTERACTION_MOVE,
	UPH_BLOCK_INTERACTION_RESIZE_LEFT,
	UPH_BLOCK_INTERACTION_RESIZE_RIGHT
};

typedef struct
{
	double initial_drag_beat_offset;
	double initial_start_beat;
	double initial_length_beats;
	double initial_start_offset_beats;
	uint32_t block_index;
	Uph_Track *track;
	Uph_ActionTrackRef source_track;
	uint32_t source_index;
	Naui_List(Uph_ActionBlockSnapshot) group_blocks;
	bool active;
	bool creating;
	bool group;
	bool additive;
	Uph_BlockInteractionMode mode;
}
Uph_DraggingBlockState;

typedef struct
{
	uint32_t block_index;
	Uph_Track *track;
	int32_t dragging_point_index;
	Uph_ResourceIndex automation_index;
	Uph_AutomationPoint initial_point;
	bool creating;
	bool active;
}
Uph_AutomationEditState;

typedef struct
{
	uint32_t block_index;
	Uph_Track *track;
	bool active;
}
Uph_HoveredBlockState;

typedef struct
{
	double start_beat;
	float start_content_y;
	bool active;
}
Uph_MarqueeState;

typedef struct
{
	Uph_ActionTrackRef source;
	Naui_Vec2 press_position;
	bool pending;
	bool active;
	bool select_on_release;
}
Uph_TrackDragState;

typedef struct
{
	Naui_List(Uph_TrackPtr) tracks;
	Uph_Track *dest_group;
	uint32_t dest_index;
}
Uph_TrackPendingMove;

typedef struct
{
	Naui_Vec2 scroll;
	Naui_Vec2 zoom;
	Uph_DraggingBlockState drag;
	Uph_HoveredBlockState hovered_block;
	Uph_AutomationEditState automation_edit;
	Uph_MarqueeState marquee;
	Uph_TrackDragState track_drag;
	Uph_TrackPendingMove pending_move;
	Naui_List(Uph_TrackPtr) selected_tracks;
	Uph_Track *selection_anchor;
	Leaf_BoundingBox panel_bounding_box;
	Uph_Track *current_options_track;
	Uph_Track *current_color_options_track;
	Uph_Track *current_hovered_track;
	Uph_Track *rename_track;
	Naui_String rename_old_name;
	uint32_t visual_row_counter;
	uint32_t max_track_depth;
	float content_height;
	Uph_SnapResolution snap_resolution;
	Uph_ActionMode current_action_mode;
	bool panel_focused;
	bool panel_hovered;
	bool tracks_hovered;
	bool track_header_hovered;
	bool disable_space_to_play;
}
Uph_SongTimelineData;

static Uph_SongTimelineData uph_song_timeline_data;

#pragma region Temp Helpers
static inline bool uph_ui_ctrl_down(void)	// Simply because Naui is broken
{
	return naui_key_down(NAUI_KEY_CONTROL) || naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL);
}

static inline bool uph_ui_shift_down(void)
{
	return naui_key_down(NAUI_KEY_SHIFT) || naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT);
}
#pragma endregion

#pragma region Shared Helpers

static inline bool uph_song_timeline_vec4_contains_vec2(const Naui_Vec4 rect, const Naui_Vec2 point)
{
	return point.x >= rect.x && point.x < rect.x + rect.z && point.y >= rect.y && point.y < rect.y + rect.w;
}

static inline bool uph_song_timeline_block_is_visible(double start_beat, double length_beats, float zoom_x, float scroll_x, float viewport_width)
{
	const double left = start_beat * zoom_x;
	const double right = (start_beat + length_beats) * zoom_x;

	if (right < scroll_x)
		return false;

	if (left > scroll_x + viewport_width)
		return false;

	return true;
}

static uint32_t uph_song_timeline_track_depth(const Uph_Track *track)
{
	uint32_t max_depth = 0;
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(track->subtracks); i++)
	{
		const uint32_t depth = uph_song_timeline_track_depth(&track->subtracks[i]) + 1;
		if (depth > max_depth)
			max_depth = depth;
	}
	return max_depth;
}

static float uph_song_timeline_header_depth_width(void)
{
	return uph_song_timeline_data.max_track_depth * UPH_SONG_TIMELINE_DEPTH_INDENT;
}

static float uph_song_timeline_header_column_width(void)
{
	return naui_theme_float("uph_song_timeline_header_width") + naui_theme_vec2("uph_song_timeline_header_padding").x * 2.0f + uph_song_timeline_header_depth_width();
}

static float uph_song_timeline_max_scroll_y(void)
{
	return uph_song_timeline_data.content_height - uph_song_timeline_data.zoom.y;
}

static Uph_BlockInteractionMode uph_song_timeline_classify_hover(Naui_Vec4 hover_box, float mouse_x)
{
    if (mouse_x <= hover_box.x + UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_LEFT;

    if (mouse_x >= hover_box.x + hover_box.z - UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_RIGHT;

    return UPH_BLOCK_INTERACTION_MOVE;
}

#pragma endregion

#pragma region Public API
void uph_song_timeline_update_automation_point_drag(Naui_Vec2 position, Naui_Vec2 size, Uph_Track *track, uint32_t block_index, double start_offset, Uph_ResourceIndex automation_index);

void uph_song_timeline_update_input(void);
void uph_song_timeline_update_selection_keys(void);
void uph_song_timeline_update_empty_space_marquee(void);

void uph_song_timeline_render_ruler(Leaf_BoundingBox bbox, float zoom_x, float scroll_x);
void uph_song_timeline_render_playhead_overlay(Leaf_BoundingBox bbox, void *data);
void uph_song_timeline_render_top_bar(void);

void uph_song_timeline_render_track(Uph_Track *track, uint32_t depth, Uph_UIMenuID options_menu, Uph_UIMenuID color_menu);
void uph_song_timeline_render_track_options_menu(Uph_UIMenuID track_options_context_menu);
void uph_song_timeline_render_track_plus(void);
void uph_song_timeline_update_track_drag(void);
void uph_song_timeline_apply_pending_move(void);

void uph_song_timeline_render_track_timeline(Uph_Track *track);

Uph_TimelineBlock uph_song_timeline_block_create(double start_beat, uint32_t resource_index, Uph_ResourceType block_type);

void uph_song_timeline_update_drag_track_switch(void);
void uph_song_timeline_begin_block_drag(Uph_Track *track, uint32_t block_index, Uph_BlockInteractionMode mode, double mouse_beat);
void uph_song_timeline_apply_group_drag(const Uph_TimelineBlock *grabbed);
void uph_song_timeline_finish_block_drag(void);

void uph_song_timeline_render_track_timeline_blocks(Leaf_BoundingBox bbox, Uph_Track *track);

void uph_song_timeline_render_toolbox(void);
#pragma endregion