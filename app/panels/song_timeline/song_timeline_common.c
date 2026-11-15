#define UPH_SONG_TIMELINE_HEADER_WIDTH      250
#define UPH_SONG_TIMELINE_HEADER_PADDING    ((Naui_Vec2){ 14.0f, 14.0f })
#define UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH 6
#define UPH_SONG_TIMELINE_DEPTH_INDENT 12
#define UPH_SONG_TIMELINE_FOLDER_ROW_HEIGHT 56
#define UPH_SONG_TIMELINE_BLOCK_TITLE_FONT_SIZE 13
#define UPH_SONG_TIMELINE_BLOCK_TITLE_PADDING 3

typedef uint8_t Uph_ActionMode;
enum
{
	UPH_ACTION_SELECT,
	UPH_ACTION_DRAW,
	UPH_ACTION_CUT
};

typedef struct
{
	double total_beats;
	float grab_offset;
	bool dragging;
	bool hovered;
}
Uph_SongTimelineMinimapState;

typedef struct
{
	Uph_Track *src_track;
	Uph_Track *dst_track;
	uint32_t src_index;
	uint32_t dst_index;
	uint32_t current_index;
}
Uph_GroupBlockMove;

typedef struct
{
	Uph_Track *track;
	Uph_ResourceType type;
}
Uph_TrackTypeBackup;

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
	Naui_List(Uph_GroupBlockMove) group_moves;
	Naui_List(Uph_TrackTypeBackup) type_backups;
	int32_t group_row_offset;
	uint32_t group_grabbed;
	Naui_Vec2 initial_mouse;
	bool active;
	bool creating;
	bool group;
	bool additive;
	bool select_tool;
	bool moved;
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
	const Uph_Track *track;
	uint32_t block_index;
	bool button_hovered;
	bool active;
}
Uph_BlockMenuHover;

typedef struct
{
	Uph_BlockMenuHover hover;
	Uph_BlockMenuHover next_hover;
	Uph_ActionTrackRef target_track;
	uint32_t target_index;
	float view_width;
	float measured_title_height;
	bool title_probe_used;
	bool target_valid;
	bool open_requested;
}
Uph_BlockMenuState;

typedef struct
{
	Uph_ActionTrackRef source;
	Naui_Vec2 press_position;
	bool pending;
	bool active;
	bool locked;
	bool select_on_release;
	bool deselect_record_on_release;
}
Uph_TrackDragState;

typedef struct
{
	bool active;
	double beat;
}
Uph_PlayheadDragState;

typedef struct
{
	Naui_List(Uph_TrackPtr) tracks;
	Uph_Track *dest_group;
	uint32_t dest_index;
}
Uph_TrackPendingMove;

typedef struct
{
	Uph_ActionTrackRef parent;
	Uph_TimelineBlock block;
	bool active;
}
Uph_PendingAutomationDrop;

typedef struct
{
	Naui_Vec2 scroll;
	Naui_Vec2 zoom;
	Uph_DraggingBlockState drag;
	Uph_HoveredBlockState hovered_block;
	Uph_AutomationEditState automation_edit;
	Uph_MarqueeState marquee;
	Uph_BlockMenuState block_menu;
	Uph_LoopRegionDrag loop_drag;
	Uph_PlayheadDragState playhead_drag;
	Uph_SongTimelineMinimapState minimap;
	Uph_TrackDragState track_drag;
	Uph_TrackPendingMove pending_move;
	Uph_PendingAutomationDrop pending_automation;
	Naui_List(Uph_TrackPtr) selected_tracks;
	Uph_Track *selection_anchor;
	Leaf_BoundingBox panel_bounding_box;
	Uph_Track *current_options_track;
	Uph_Track *current_color_options_track;
	Uph_Track *current_hovered_track;
	Uph_Track *frame_hovered_track;
	Uph_Track *hovered_track;
	Uph_Track *rename_track;
	Naui_String rename_old_name;
	struct
	{
		uint32_t row_counter;
		float row_y;
		float settled_row_y;
		float zoom_y;
		bool snap_track_motion;
	}
	visual;
	struct
	{
		float last_mouse_y;
		float anchor_local_y;
		float unsnapped_zoom_y;
		bool active;
	}
	zoom_drag;
	struct
	{
		float anchor_x;
		bool anchored;
	}
	follow;
	uint32_t max_track_depth;
	float content_height;
	float bottom_drop_y;
	Uph_SnapResolution snap_resolution;
	Uph_ActionMode current_action_mode;
	bool panel_focused;
	bool panel_hovered;
	bool tracks_hovered;
	bool track_header_hovered;
	bool bottom_hovered;
	bool disable_space_to_play;
	bool time_display_seconds;
	bool follow_playhead;
	bool record_rolling;
}
Uph_SongTimelineState;

static Uph_SongTimelineState _uph_song_timeline_state;

static inline bool uph_ui_ctrl_down(void)	// Simply because Naui is broken
{
	return naui_key_down(NAUI_KEY_CONTROL) || naui_key_down(NAUI_KEY_LCONTROL) || naui_key_down(NAUI_KEY_RCONTROL);
}

static inline bool uph_ui_shift_down(void)
{
	return naui_key_down(NAUI_KEY_SHIFT) || naui_key_down(NAUI_KEY_LSHIFT) || naui_key_down(NAUI_KEY_RSHIFT);
}

static inline Uph_ActionMode uph_song_timeline_tool(void)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	if (state->current_action_mode == UPH_ACTION_DRAW && uph_ui_ctrl_down())
		return UPH_ACTION_SELECT;

	return state->current_action_mode;
}

static inline Naui_Cursor uph_action_mode_cursor(Uph_ActionMode mode)
{
	if (mode == UPH_ACTION_DRAW)
		return UPH_CURSOR_DRAW;

	if (mode == UPH_ACTION_CUT)
		return UPH_CURSOR_CUT;

	return NAUI_CURSOR_ARROW;
}

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
	return _uph_song_timeline_state.max_track_depth * UPH_SONG_TIMELINE_DEPTH_INDENT;
}

static float uph_song_timeline_header_column_width(void)
{
	return UPH_SONG_TIMELINE_HEADER_WIDTH + UPH_SONG_TIMELINE_HEADER_PADDING.x * 2.0f + uph_song_timeline_header_depth_width();
}

static float uph_song_timeline_max_scroll_y(void)
{
	return fmaxf(0.0f, _uph_song_timeline_state.content_height - _uph_song_timeline_state.zoom.y);
}

static inline float uph_song_timeline_block_title_height(void)
{
	return NAUI_DPI(UPH_SONG_TIMELINE_BLOCK_TITLE_FONT_SIZE) + NAUI_DPI(UPH_SONG_TIMELINE_BLOCK_TITLE_PADDING) * 2.0f;
}

static inline float uph_song_timeline_block_title_bar_height(void)
{
	Uph_SongTimelineState *state = &_uph_song_timeline_state;
	const float measured = state->block_menu.measured_title_height;
	return measured > 0.0f ? measured : uph_song_timeline_block_title_height();
}

static inline bool uph_song_timeline_block_menu_button_rect(float block_left, float block_width, float view_left, float view_right, Naui_Vec4 *out)
{
	const float bar_height = uph_song_timeline_block_title_bar_height();
	const float size = fmaxf(bar_height - NAUI_DPI(2), 1.0f);
	const float inset = (bar_height - size) * 0.5f;
	const float visible_left = fmaxf(block_left, view_left);
	const float visible_right = fminf(block_left + block_width, view_right);
	if (visible_right - visible_left < size * 2.0f + inset)
		return false;

	*out = (Naui_Vec4){ floorf(visible_right - inset - size), floorf((bar_height - size) * 0.5f), floorf(size), floorf(size) };
	return true;
}

static Uph_BlockInteractionMode uph_song_timeline_classify_hover(Naui_Vec4 hover_box, float mouse_x)
{
    if (mouse_x <= hover_box.x + UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_LEFT;

    if (mouse_x >= hover_box.x + hover_box.z - UPH_SONG_TIMELINE_RESIZE_HANDLE_WIDTH)
        return UPH_BLOCK_INTERACTION_RESIZE_RIGHT;

    return UPH_BLOCK_INTERACTION_MOVE;
}
