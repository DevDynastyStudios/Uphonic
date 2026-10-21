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

typedef struct
{
	Uph_TimelineBlock block;
	Naui_Color color;
}
Uph_BlockContentDrawData;

static void _uph_song_timeline_render_block_content(Leaf_BoundingBox box, Uph_BlockContentDrawData *data)
{
	const Uph_TimelineBlock *block = &data->block;
	const Naui_Vec2 position = { box.x, box.y };
	const Naui_Vec2 size = { box.width, box.height };
	const Leaf_BoundingBox visible_bbox = uph_song_timeline_data.panel_bounding_box;

	if (block->type == UPH_RESOURCE_SAMPLE)
	{
		uph_ui_waveform_zoomable(
			position,
			size,
			data->color,
			uph_song_timeline_data.zoom.x,
			block->start_offset_beats,
			visible_bbox,
			&uph_state.project.samples[block->resource_index]
		);
	}
	else if (block->type == UPH_RESOURCE_PATTERN)
	{
		_uph_song_timeline_render_midi_pattern(
			position,
			size,
			data->color,
			block->start_offset_beats,
			visible_bbox,
			&uph_state.project.midi_patterns[block->resource_index]
		);
	}
	else if (block->type == UPH_RESOURCE_AUTOMATION)
	{
		uph_song_timeline_render_automation(
			position,
			size,
			data->color,
			block->start_offset_beats,
			visible_bbox,
			&uph_state.project.automations[block->resource_index]
		);
	}
}

static const char *_uph_song_timeline_block_name(const Uph_TimelineBlock *block)
{
	if (block->type == UPH_RESOURCE_SAMPLE)
		return uph_state.project.samples[block->resource_index].name.data;

	if (block->type == UPH_RESOURCE_PATTERN)
		return uph_state.project.midi_patterns[block->resource_index].name.data;

	if (block->type == UPH_RESOURCE_AUTOMATION)
		return uph_state.project.automations[block->resource_index].name.data;

	return "";
}

static void _uph_song_timeline_render_timeline_block(Naui_Vec2 offset, Naui_Vec2 size, Naui_Color color, float opacity, const Uph_TimelineBlock *block)
{
	const float anim_offset = (naui_ease_out_elastic(block->visual_lifetime) - 1.0f) * NAUI_DPI(30.0f);
	const Naui_Vec2 anim_size = {
		fmaxf(size.x + anim_offset + 0.5f, 0.0f),
		fmaxf(size.y + anim_offset, 0.0f)
	};

	const float title_padding = NAUI_DPI(2.0f);
	const float font_size = NAUI_DPI(13.0f);
	const float title_height = font_size + title_padding * 2.0f;
	const int32_t rounding = (int32_t)NAUI_DPI(4.0f);

	Uph_BlockContentDrawData content_data = {
		.block = *block,
		.color = leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity))
	};

	leaf({
		.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
		.floating.offset = { offset.x, offset.y },
		.size = { LEAF_SIZE_FIXED((int32_t)size.x), LEAF_SIZE_FIXED((int32_t)size.y) },
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
	})
	leaf({
		.size = { LEAF_SIZE_FIXED((int32_t)anim_size.x), LEAF_SIZE_FIXED((int32_t)anim_size.y) },
		.clip_children = true
	})
	{
		leaf({
			.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIXED((int32_t)title_height) },
			.padding = LEAF_PADDING_ALL((int32_t)title_padding),
			.color = { leaf_rgba(color.r, color.g, color.b, (uint8_t)(200 * opacity)) },
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_TL | LEAF_CORNER_TR)
		})
		{
			leaf_text(_uph_song_timeline_block_name(block), {
				.font_size = { font_size },
				.color = { leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity)) }
			});
		}

		leaf({
			.size = { LEAF_SIZE_FULL, LEAF_SIZE_GROW },
			.color = { leaf_rgba(color.r, color.g, color.b, (uint8_t)(60 * opacity)) },
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_BL | LEAF_CORNER_BR),
			.custom_draw = (Leaf_CustomDrawFn)_uph_song_timeline_render_block_content,
			.custom_draw_data = LEAF_DATA_SLICE(content_data)
		});

		if (block->selected)
		{
			const int32_t line_width = (int32_t)NAUI_DPI(2);
			leaf({
				.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
				.floating.offset = { (float)line_width, (float)line_width },
				.size = { LEAF_SIZE_FIXED((int32_t)anim_size.x - line_width * 2), LEAF_SIZE_FIXED((int32_t)anim_size.y - line_width * 2) },
				.border = {
					.width = line_width,
					.color = { leaf_rgba(255, 255, 255, (uint8_t)(255 * opacity)) },
					.sides = LEAF_SIDE_ALL
				},
				.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL)
			});
		}
	}
}

void uph_song_timeline_render_track_timeline_blocks(Uph_Track *track)
{
	Naui_List(Uph_TimelineBlock) blocks = track->blocks;
	const float zoom_x = uph_song_timeline_data.zoom.x;
	const float scroll_x = uph_song_timeline_data.scroll.x;
	const float row_height = NAUI_DPI(uph_song_timeline_data.zoom.y);
	const float viewport_width = uph_song_timeline_data.panel_bounding_box.width;
	const float opacity = uph_track_is_audible(&uph_state.project, track) ? 1.0f : 0.25f;

	Naui_Color color = uph_resources_track_color(track->color_index);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
	{
		if (!uph_song_timeline_block_is_visible(blocks[i].start_beat, blocks[i].length_beats, zoom_x, scroll_x, viewport_width))
			continue;

		blocks[i].visual_lifetime = NAUI_MIN(blocks[i].visual_lifetime + naui_delta_time(), 1.0f);
		_uph_song_timeline_render_timeline_block(
			(Naui_Vec2) { zoom_x * (float)blocks[i].start_beat - scroll_x, 0.0f },
			(Naui_Vec2) { zoom_x * (float)blocks[i].length_beats, row_height },
			color,
			opacity,
			&blocks[i]
		);
	}
}