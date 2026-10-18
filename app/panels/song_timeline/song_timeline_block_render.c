static void _uph_song_timeline_render_midi_pattern(Naui_Vec2 position, Naui_Vec2 size, Naui_Color color, double start_offset, Leaf_BoundingBox visible_bbox, Uph_MidiPattern* pattern)
{
	const uint32_t note_count = (uint32_t)naui_list_len(pattern->notes);
	if (note_count == 0)
		return;

	uint8_t lowest_key = UINT8_MAX;
	uint8_t highest_key = 0;

	for (uint32_t i = 0; i < note_count; i++)
	{
		const Uph_MidiNote *note = &pattern->notes[i];
		if (note->key_number < lowest_key)
			lowest_key = note->key_number;

		if (note->key_number > highest_key)
			highest_key = note->key_number;
	}

	const uint32_t key_range = (uint32_t)(highest_key - lowest_key) + 1;
	const float slot_height = size.y / (float)key_range;
	const float note_height = fmaxf(slot_height, 1.0f);
	for (uint32_t i = 0; i < note_count; i++)
	{
		const Uph_MidiNote *note = &pattern->notes[i];
		const double note_start_beat = note->start_beat - start_offset;
		if (note_start_beat + note->length_beats < 0.0)
			continue;

		const float x = position.x + (float)(note_start_beat * uph_song_timeline_data.zoom.x);
		const float width = (float)(note->length_beats * uph_song_timeline_data.zoom.x);
		if (x + width < visible_bbox.x || x > visible_bbox.x + visible_bbox.width)
			continue;

		const uint32_t key_offset_from_top = (uint32_t)(highest_key - note->key_number);
		const float y = position.y + (float)key_offset_from_top * slot_height;
		naui_fill_rect(
			(Naui_Vec2){ x, y },
			(Naui_Vec2){ fmaxf(width, 1.0f), note_height },
			color,
			0,
			NAUI_CORNER_NONE
		);
	}
}

static void uph_song_timeline_render_automation(
	Naui_Vec2 position,
	Naui_Vec2 size,
	Naui_Color color,
	double start_offset,
	Leaf_BoundingBox visible_bbox,
	Uph_Automation* automation
)
{
	const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
	const float zoom_x = uph_song_timeline_data.zoom.x;
	if (point_count == 0)
		return;

	const int32_t point_size = NAUI_DPI(6);
	const int32_t half_point_size = point_size / 2;
	const Naui_Color fill_color = {
		color.r, color.g, color.b, (uint8_t)(color.a * 0.25f)
	};

	uph_ui_automation_curve(
		automation,
		position.x - (float)start_offset * zoom_x,
		zoom_x,
		position.y,
		size.y,
		color,
		fill_color,
		1.0f
	);

	for (uint32_t i = 0; i < point_count; i++)
	{
		const Uph_AutomationPoint *point = &automation->points[i];
		const Naui_Vec2 point_position = { position.x + (float)(point->beat - start_offset) * zoom_x, position.y + size.y - point->value * size.y };

		naui_fill_rect(
			(Naui_Vec2) { point_position.x - half_point_size, point_position.y - half_point_size },
			(Naui_Vec2) { point_size, point_size },
			color,
			FLT_MAX,
			NAUI_CORNER_ALL
		);
	}
}

static void _uph_song_timeline_render_timeline_block(Naui_Vec2 position, Naui_Vec2 size, Naui_Color color, float opacity, bool selected, const Uph_TimelineBlock *block, Leaf_BoundingBox visible_bbox)
{
	const float anim_scale = NAUI_DPI(30.0f);
	position.x += size.x * 0.5f;
	position.y += size.y * 0.5f;
	size.x = size.x + (naui_ease_out_elastic(block->visual_lifetime) - 1.0f) * anim_scale + 0.5f;
	size.y = size.y + (naui_ease_out_elastic(block->visual_lifetime) - 1.0f) * anim_scale;
	position.x -= size.x * 0.5f;
	position.y -= size.y * 0.5f;

	const float title_padding = NAUI_DPI(2.0f);
	const float font_size = NAUI_DPI(13.0f);
	const float title_height = font_size + title_padding * 2.0f;
	const float rounding = NAUI_DPI(4.0f);

	naui_push_clip_rect(position.x, position.y, size.x, size.y);

	naui_fill_rect(
		position,
		(Naui_Vec2) { size.x, title_height },
		leaf_rgba(color.r, color.g, color.b, (uint8_t)(200 * opacity)),
		rounding,
		NAUI_CORNER_TL | NAUI_CORNER_TR
	);

	naui_fill_rect(
		(Naui_Vec2) { position.x, position.y + title_height },
		(Naui_Vec2) { size.x, size.y - title_height },
		leaf_rgba(color.r, color.g, color.b, (uint8_t)(60 * opacity)),
		rounding,
		NAUI_CORNER_BL | NAUI_CORNER_BR
	);

	if (block->type == UPH_RESOURCE_SAMPLE)
	{
		naui_draw_text(
			(Naui_Vec2) { position.x + title_padding, position.y + title_padding },
			uph_state.project.samples[block->resource_index].name.data,
			font_size,
			0,
			leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity))
		);

		uph_ui_waveform_zoomable(
			(Naui_Vec2) { position.x, position.y + title_height },
			(Naui_Vec2) { size.x, size.y - title_height },
			leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity)),
			uph_song_timeline_data.zoom.x,
			block->start_offset_beats,
			visible_bbox,
			&uph_state.project.samples[block->resource_index]
		);
	}
	else if (block->type == UPH_RESOURCE_PATTERN)
	{
		naui_draw_text(
			(Naui_Vec2) { position.x + title_padding, position.y + title_padding },
			uph_state.project.midi_patterns[block->resource_index].name.data,
			font_size,
			0,
			leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity))
		);

		_uph_song_timeline_render_midi_pattern(
			(Naui_Vec2) { position.x, position.y + title_height },
			(Naui_Vec2) { size.x, size.y - title_height },
			leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity)),
			block->start_offset_beats,
			visible_bbox,
			&uph_state.project.midi_patterns[block->resource_index]
		);
	}
	else if (block->type == UPH_RESOURCE_AUTOMATION)
	{
		naui_draw_text(
			(Naui_Vec2) { position.x + title_padding, position.y + title_padding },
			uph_state.project.automations[block->resource_index].name.data,
			font_size,
			0,
			leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity))
		);

		uph_song_timeline_render_automation(
			(Naui_Vec2) { position.x, position.y + title_height },
			(Naui_Vec2) { size.x, size.y - title_height },
			leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity)),
			block->start_offset_beats,
			visible_bbox,
			&uph_state.project.automations[block->resource_index]
		);
	}

	naui_pop_clip_rect();

	if (selected)
	{
		const float line_width = NAUI_DPI(2);
		position.x += line_width;
		position.y += line_width;
		size.x -= line_width * 2;
		size.y -= line_width * 2;
		naui_draw_rect(position, size, leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity)), line_width, rounding, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
	}
}

void uph_song_timeline_render_track_timeline_blocks(Leaf_BoundingBox bbox, Uph_Track *track)
{
	Naui_List(Uph_TimelineBlock) blocks = track->blocks;
	const float zoom_x = uph_song_timeline_data.zoom.x;
	const float scroll_x = uph_song_timeline_data.scroll.x;
	const float opacity = uph_track_is_audible(&uph_state.project, track) ? 1.0f : 0.25f;

	Naui_Color color = uph_resources_track_color(track->color_index);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
	{
		if (!uph_song_timeline_block_is_visible(blocks[i].start_beat, blocks[i].length_beats, zoom_x, scroll_x, bbox.width))
			continue;

		blocks[i].visual_lifetime = NAUI_MIN(blocks[i].visual_lifetime + naui_delta_time(), 1.0f);
		_uph_song_timeline_render_timeline_block(
			(Naui_Vec2) { bbox.x + zoom_x * blocks[i].start_beat - scroll_x, bbox.y },
			(Naui_Vec2) { zoom_x * blocks[i].length_beats, bbox.height },
			color,
			opacity,
			blocks[i].selected,
			&blocks[i],
			bbox
		);
	}
}
