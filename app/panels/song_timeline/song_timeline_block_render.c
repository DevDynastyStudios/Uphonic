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
			&uph_state.project.samples[block->resource_index],
			block
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

typedef struct
{
	Naui_Color text_color;
	float opacity;
	bool hovered;
}
Uph_BlockMenuButtonDrawData;

static void _uph_song_timeline_render_block_menu_button(Leaf_BoundingBox box, Uph_BlockMenuButtonDrawData *data)
{
	const Naui_Color c = data->text_color;
	const float icon_alpha = (float)c.a;
	if (data->hovered)
	{
		naui_fill_rect(
			(Naui_Vec2){ box.x, box.y },
			(Naui_Vec2){ box.width, box.height },
			leaf_rgba(c.r, c.g, c.b, (uint8_t)(icon_alpha * 0.3f * data->opacity)),
			NAUI_DPI(UPH_UI_FRAME_ROUNDING),
			NAUI_CORNER_ALL
		);
	}

	const float padding = NAUI_DPI(3.0f);
	naui_draw_image(
		naui_asset_image("uph_icon_dots"),
		(Naui_Vec2){ box.x + padding, box.y + padding },
		(Naui_Vec2){ box.width - padding * 2.0f, box.height - padding * 2.0f },
		leaf_rgba(c.r, c.g, c.b, (uint8_t)(icon_alpha * data->opacity)),
		0.0f,
		NAUI_CORNER_NONE
	);
}

static void _uph_song_timeline_render_timeline_block(Naui_Vec2 offset, Naui_Vec2 size, Naui_Color color, float opacity, const Uph_TimelineBlock *block, bool show_menu_button, bool menu_button_hovered)
{
	const float anim_offset = (naui_ease_out_quint(block->visual.lifetime) - 1.0f) * NAUI_DPI(8.0f);
	const Naui_Vec2 anim_size = {
		fmaxf(size.x + anim_offset + 0.5f, 0.0f),
		fmaxf(size.y + anim_offset, 0.0f)
	};

	const float title_padding = NAUI_DPI(UPH_SONG_TIMELINE_BLOCK_TITLE_PADDING);
	const float font_size = NAUI_DPI(UPH_SONG_TIMELINE_BLOCK_TITLE_FONT_SIZE);
	const float title_height = uph_song_timeline_block_title_height();
	const int32_t rounding = (int32_t)NAUI_DPI(UPH_TIMELINE_BLOCK_ROUNDING);
	const Naui_Color text_color = naui_theme_color("uph_timeline_block_text_color");

	const float visible_left = offset.x + (size.x - anim_size.x) * 0.5f;
	const float title_shift = fminf(fmaxf(-visible_left, 0.0f), fmaxf(anim_size.x - title_padding * 2.0f, 0.0f));

	Uph_BlockContentDrawData content_data = {
		.block = *block,
		.color = leaf_rgba(color.r, color.g, color.b, (uint8_t)(255 * opacity))
	};

	leaf({
		.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
		.transparency = 1.0f - naui_ease_out_quint(block->visual.lifetime),
		.floating.offset = { offset.x, offset.y },
		.size = { LEAF_SIZE_FIXED((int32_t)size.x), LEAF_SIZE_FIXED((int32_t)size.y) },
		.child_alignment = { LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER }
	})
	leaf({
		.size = { LEAF_SIZE_FIXED((int32_t)anim_size.x), LEAF_SIZE_FIXED((int32_t)anim_size.y) },
		.clip_children = true
	})
	{
		Leaf_ID title_id = { 0 };
		if (show_menu_button && !uph_song_timeline_data.block_menu.title_probe_used)
		{
			title_id = leaf_id("uph_song_timeline_block_menu_title");
			uph_song_timeline_data.block_menu.title_probe_used = true;
		}

		leaf({
			.id = title_id,
			.size = { LEAF_SIZE_FULL, LEAF_SIZE_FIXED((int32_t)title_height) },
			.padding = { (int32_t)(title_padding + title_shift), (int32_t)title_padding, (int32_t)title_padding, (int32_t)title_padding },
			.color = { leaf_rgba(color.r, color.g, color.b, (uint8_t)(200 * opacity)) },
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_TL | LEAF_CORNER_TR)
		})
		{
			leaf_text(_uph_song_timeline_block_name(block), {
				.font_size = { font_size },
				.color = { leaf_rgba(text_color.r, text_color.g, text_color.b, (uint8_t)(text_color.a * opacity)) }
			});
		}

		leaf({
			.size = { LEAF_SIZE_FULL, LEAF_SIZE_GROW },
			.color = { leaf_rgba(color.r, color.g, color.b, (uint8_t)(60 * opacity)) },
			.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_BL | LEAF_CORNER_BR),
			.custom_draw = (Leaf_CustomDrawFn)_uph_song_timeline_render_block_content,
			.custom_draw_data = LEAF_DATA_SLICE(content_data)
		});

		const float view_width = uph_song_timeline_data.block_menu.view_width > 0.0f ? uph_song_timeline_data.block_menu.view_width : FLT_MAX;
		Naui_Vec4 menu_button;
		if (show_menu_button && uph_song_timeline_block_menu_button_rect(offset.x, size.x, 0.0f, view_width, &menu_button))
		{
			menu_button.x -= visible_left;
			Uph_BlockMenuButtonDrawData button_data = { .text_color = text_color, .opacity = opacity, .hovered = menu_button_hovered };
			leaf({
				.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
				.floating.offset = { menu_button.x, menu_button.y },
				.size = { LEAF_SIZE_FIXED((int32_t)menu_button.z), LEAF_SIZE_FIXED((int32_t)menu_button.w) },
				.custom_draw = (Leaf_CustomDrawFn)_uph_song_timeline_render_block_menu_button,
				.custom_draw_data = LEAF_DATA_SLICE(button_data)
			});
		}

		if (block->selected)
		{
			const int32_t line_width = (int32_t)NAUI_DPI(2);
			const Leaf_Color outline_color = naui_theme_color("uph_selection_outline_color");
			leaf({
				.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
				.floating.offset = { (float)line_width, (float)line_width },
				.size = { LEAF_SIZE_FIXED((int32_t)anim_size.x - line_width * 2), LEAF_SIZE_FIXED((int32_t)anim_size.y - line_width * 2) },
				.border = {
					.width = line_width,
					.color = { leaf_rgba(outline_color.r, outline_color.g, outline_color.b, (uint8_t)(outline_color.a * opacity)) },
					.sides = LEAF_SIDE_ALL
				},
				.rounding = LEAF_ROUNDING_FIXED(rounding, LEAF_CORNER_ALL)
			});
		}
	}
}

void uph_song_timeline_emit_block_shockwave(const Uph_Track *track, const Uph_TimelineBlock *block)
{
	if (!track || naui_panel_closed(uph_state.panels.song_timeline))
		return;

	const Uph_SongTimelineData *data = &uph_song_timeline_data;
	const Leaf_BoundingBox section = data->panel_bounding_box;
	const float timeline_x = section.x + NAUI_DPI(uph_song_timeline_header_column_width());

	uph_particle_emit_shockwave(
		(Naui_Vec2){ timeline_x + data->zoom.x * (float)block->start_beat - data->scroll.x, section.y - data->scroll.y + track->visual.y },
		(Naui_Vec2){ data->zoom.x * (float)block->length_beats, NAUI_DPI(data->zoom.y) },
		uph_color_mix(uph_resources_track_color(track->color_index), leaf_rgba(255, 255, 255, 255), 0.35f),
		NAUI_DPI(UPH_TIMELINE_BLOCK_ROUNDING),
		naui_panel_bounding_box(uph_state.panels.song_timeline)
	);
}

void uph_song_timeline_render_track_timeline_blocks(Uph_Track *track)
{
	Naui_List(Uph_TimelineBlock) blocks = track->blocks;
	const float zoom_x = uph_song_timeline_data.zoom.x;
	const float scroll_x = uph_song_timeline_data.scroll.x;
	const float row_height = NAUI_DPI(uph_song_timeline_data.zoom.y);
	const float viewport_width = uph_song_timeline_data.panel_bounding_box.width;
	const float opacity = uph_track_is_audible(&uph_state.project, track) ? 1.0f : 0.25f;

	const Uph_BlockMenuState *block_menu = &uph_song_timeline_data.block_menu;
	const bool menu_targets_track = block_menu->target_valid && uph_ui_any_menu_open() && uph_action_track_resolve(block_menu->target_track) == track;

	Naui_Color color = uph_resources_track_color(track->color_index);
	for (uint32_t i = 0; i < (uint32_t)naui_list_len(blocks); i++)
	{
		const bool hovered = block_menu->hover.active && block_menu->hover.track == track && block_menu->hover.block_index == i;
		const bool menu_target = menu_targets_track && block_menu->target_index == i;

		if (!uph_song_timeline_block_is_visible(blocks[i].start_beat, blocks[i].length_beats, zoom_x, scroll_x, viewport_width))
			continue;

		blocks[i].visual.lifetime = naui_app_animations_enabled() ? NAUI_MIN(blocks[i].visual.lifetime + naui_delta_time() * NAUI_UI_ANIM_SPEED, 1.0f) : 1.0f;
		_uph_song_timeline_render_timeline_block(
			(Naui_Vec2) { zoom_x * (float)blocks[i].start_beat - scroll_x, 0.0f },
			(Naui_Vec2) { zoom_x * (float)blocks[i].length_beats, row_height },
			color,
			opacity,
			&blocks[i],
			(hovered || menu_target) && !uph_song_timeline_data.drag.active,
			(hovered && block_menu->hover.button_hovered) || menu_target
		);
	}
}