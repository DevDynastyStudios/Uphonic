#define UPH_PIANO_ACCIDENTAL_SHINE_MIX 0.10f
#define UPH_PIANO_BORDER_DARKEN_MIX 0.50f
#define UPH_PIANO_LABEL_DARKEN_MIX 0.60f
#define UPH_PIANO_LABEL_ALPHA 215
#define UPH_PIANO_LABEL_FONT_SIZE 16.0f
#define UPH_PIANO_HIGHLIGHT_BAR_WIDTH 3.0f

void uph_piano_init(Uph_Piano *piano)
{
	memset(piano, 0, sizeof(*piano));
	piano->style = uph_piano_default_style();
	piano->mouse_key = -1;
}

bool uph_piano_key_pressed(const Uph_Piano *piano, uint8_t key)
{
	return piano->mouse_key == (int16_t)key || uph_key_bitset_get(piano->external_keys, key);
}

void uph_piano_set_external_key(Uph_Piano *piano, uint8_t key, bool pressed)
{
	if (key < UPH_NOTE_MAP_KEY_COUNT)
		uph_key_bitset_set(piano->external_keys, key, pressed);
}

#pragma region Geometry

bool uph_piano_key_rect(const Uph_Piano *piano, const Uph_NoteMap *map, uint8_t key, const Uph_PianoCanvas *canvas, Uph_PianoKeyRect *out)
{
	if (!uph_note_map_in_range(map, key))
		return false;

	const int32_t octave_base = ((int32_t)key / UPH_NOTES_PER_OCTAVE) * UPH_NOTES_PER_OCTAVE;
	const int32_t semitone = (int32_t)key - octave_base;

	int32_t natural_count = 0;
	int32_t naturals_below = 0;
	for (int32_t i = 0; i < UPH_NOTES_PER_OCTAVE; i++)
	{
		if (uph_note_map_kind_at(map, octave_base + i) != UPH_NOTE_KIND_NATURAL)
			continue;

		natural_count++;
		if (i < semitone)
			naturals_below++;
	}

	const float natural_height = (float)UPH_NOTES_PER_OCTAVE / (float)NAUI_MAX(natural_count, 1);
	const float octave_bottom = (float)(uph_note_map_row(map, octave_base) + 1);
	const float boundary = octave_bottom - (float)naturals_below * natural_height;
	const bool accidental = map->notes[key].kind == UPH_NOTE_KIND_ACCIDENTAL;
	const float full_width = canvas->box.width;
	float top, bottom, width;

	if (accidental)
	{
		int32_t run_start = semitone;
		while (run_start > 0 && uph_note_map_kind_at(map, octave_base + run_start - 1) == UPH_NOTE_KIND_ACCIDENTAL)
		{
			run_start--;
		}

		int32_t run_end = semitone;
		while (run_end < UPH_NOTES_PER_OCTAVE - 1 && uph_note_map_kind_at(map, octave_base + run_end + 1) == UPH_NOTE_KIND_ACCIDENTAL)
		{
			run_end++;
		}

		const int32_t run_length = run_end - run_start + 1;
		const float slot = natural_height / (float)run_length;
		const float height = fminf(natural_height * piano->style.black_key_height_ratio, slot);
		const float center = boundary - ((float)(semitone - run_start) - (float)(run_length - 1) * 0.5f) * slot;
		top = center - height * 0.5f;
		bottom = center + height * 0.5f;
		width = full_width * piano->style.black_key_length_ratio;
	}
	else
	{
		top = boundary - natural_height;
		bottom = boundary;
		width = full_width;
	}

	const float row_count = (float)uph_note_map_row_count(map);
	top = fmaxf(top, 0.0f);
	bottom = fminf(bottom, row_count);
	if (bottom <= top)
		return false;

	out->top = top * canvas->lane_height;
	out->bottom = bottom * canvas->lane_height;
	out->width = width;
	out->accidental = accidental;
	return true;
}

int32_t uph_piano_hit_test(const Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas, float mouse_x, float mouse_y)
{
	const Leaf_BoundingBox box = canvas->box;
	if (mouse_x < box.x || mouse_x >= box.x + box.width || mouse_y < box.y || mouse_y >= box.y + box.height)
		return -1;

	const float local_x = mouse_x - box.x;
	const float content_y = mouse_y - box.y + canvas->scroll_y;

	for (int32_t pass = 0; pass < 2; pass++)
	{
		const bool want_accidental = pass == 0;
		for (int32_t key = map->lowest; key <= map->highest; key++)
		{
			Uph_PianoKeyRect rect;
			if (!uph_piano_key_rect(piano, map, (uint8_t)key, canvas, &rect))
				continue;

			if (rect.accidental != want_accidental)
				continue;

			if (local_x < rect.width && content_y >= rect.top && content_y < rect.bottom)
				return key;
		}
	}

	return -1;
}

#pragma endregion

#pragma region Input

Uph_PianoEvents uph_piano_update(Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas, bool hovered)
{
	Uph_PianoEvents events = { -1, -1 };

	if (hovered && naui_mouse_pressed(NAUI_MOUSE_LEFT))
		piano->mouse_active = true;

	if (naui_mouse_released(NAUI_MOUSE_LEFT))
		piano->mouse_active = false;

	int32_t key = -1;
	if (piano->mouse_active)
		key = uph_piano_hit_test(piano, map, canvas, (float)naui_mouse_x(), (float)naui_mouse_y());

	if (piano->mouse_key != -1 && piano->mouse_key != key)
		events.released = piano->mouse_key;

	if (key != -1 && key != piano->mouse_key)
		events.pressed = (int16_t)key;

	piano->mouse_key = (int16_t)key;
	return events;
}

Uph_PianoEvents uph_piano_release_mouse(Uph_Piano *piano)
{
	Uph_PianoEvents events = { -1, piano->mouse_key };
	piano->mouse_key = -1;
	piano->mouse_active = false;
	return events;
}

#pragma endregion

#pragma region Rendering

static void _uph_piano_render_highlight_bar(Uph_NoteHighlight highlight, float right, float top, float bottom)
{
	if (highlight == UPH_NOTE_HIGHLIGHT_NONE)
		return;

	Leaf_Color color = naui_theme_color("naui_widget_accent_color");
	color.a = highlight == UPH_NOTE_HIGHLIGHT_ROOT ? 255 : 140;
	const float bar_width = NAUI_DPI(UPH_PIANO_HIGHLIGHT_BAR_WIDTH);
	naui_fill_rect((Naui_Vec2) { right - bar_width, top + 1.0f }, (Naui_Vec2) { bar_width, fmaxf(bottom - top - 2.0f, 0.0f) }, color, 0.0f, NAUI_CORNER_NONE);
}

static void _uph_piano_render_natural_keys(const Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas)
{
	const Uph_PianoStyle *style = &piano->style;
	const Leaf_BoundingBox box = canvas->box;
	const Leaf_Color natural_color = naui_theme_color("uph_piano_natural_color");
	const Leaf_Color natural_pressed_color = naui_theme_color("uph_piano_natural_pressed_color");
	const Leaf_Color border_color = uph_color_mix(natural_color, LEAF_COLOR_BLACK, UPH_PIANO_BORDER_DARKEN_MIX);

	for (int32_t key = map->lowest; key <= map->highest; key++)
	{
		Uph_PianoKeyRect rect;
		if (!uph_piano_key_rect(piano, map, (uint8_t)key, canvas, &rect) || rect.accidental)
			continue;

		const float top = box.y + rect.top - canvas->scroll_y;
		const float bottom = box.y + rect.bottom - canvas->scroll_y;
		if (top >= box.y + box.height || bottom <= box.y)
			continue;

		naui_fill_rect(
			(Naui_Vec2) { box.x, top },
			(Naui_Vec2) { rect.width, bottom - top },
			uph_piano_key_pressed(piano, (uint8_t)key) ? natural_pressed_color : natural_color,
			0.0f,
			NAUI_CORNER_NONE
		);

		const bool is_octave_start = key % UPH_NOTES_PER_OCTAVE == 0;
		Leaf_Color line_color = border_color;
		line_color.a = (uint8_t)(is_octave_start ? style->octave_border_alpha : style->white_key_border_alpha);
		naui_draw_line(
			(Naui_Vec2) { box.x, bottom },
			(Naui_Vec2) { box.x + rect.width, bottom },
			line_color,
			NAUI_DPI(is_octave_start ? style->octave_border_thick : style->white_key_border_thick)
		);

		_uph_piano_render_highlight_bar(map->notes[key].highlight, box.x + rect.width, top, bottom);
	}
}

static void _uph_piano_render_accidental_keys(const Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas)
{
	const Uph_PianoStyle *style = &piano->style;
	const Leaf_BoundingBox box = canvas->box;
	const float rounding = NAUI_DPI(2.0f);

	for (int32_t key = map->lowest; key <= map->highest; key++)
	{
		Uph_PianoKeyRect rect;
		if (!uph_piano_key_rect(piano, map, (uint8_t)key, canvas, &rect) || !rect.accidental)
			continue;

		const float top = box.y + rect.top - canvas->scroll_y;
		const float bottom = box.y + rect.bottom - canvas->scroll_y;
		if (top >= box.y + box.height || bottom <= box.y)
			continue;

		const bool pressed = uph_piano_key_pressed(piano, (uint8_t)key);
		const Leaf_Color sharp_color = naui_theme_color(pressed ? "uph_piano_sharp_pressed_color" : "uph_piano_sharp_color");
		const Leaf_Color sharp_gradient = naui_theme_color(pressed ? "uph_piano_sharp_pressed_gradient" : "uph_piano_sharp_gradient");

		const Naui_Vec2 position = { box.x, top };
		const Naui_Vec2 size = { rect.width, bottom - top };

		naui_fill_gradient_rect(
			position,
			size,
			(Naui_Gradient) { .color1 = sharp_color, .color2 = sharp_gradient, .percent1 = 0.75f, .percent2 = 1.0f },
			rounding,
			NAUI_CORNER_ALL
		);

		Leaf_Color outline = uph_color_mix(sharp_color, LEAF_COLOR_BLACK, UPH_PIANO_LABEL_DARKEN_MIX);
		outline.a = (uint8_t)style->black_key_border_alpha;
		naui_draw_rect(position, size, outline, NAUI_DPI(style->black_key_border_thick), rounding, NAUI_CORNER_ALL, NAUI_SIDE_ALL);

		if (!pressed && size.y > NAUI_DPI(7.0f))
		{
			naui_fill_rect(
				(Naui_Vec2) { box.x + NAUI_DPI(1.5f), top + NAUI_DPI(1.5f) },
				(Naui_Vec2) { NAUI_DPI(2.0f), size.y - NAUI_DPI(3.0f) },
				uph_color_mix(sharp_color, LEAF_COLOR_WHITE, UPH_PIANO_ACCIDENTAL_SHINE_MIX),
				NAUI_DPI(1.0f),
				NAUI_CORNER_ALL
			);
		}

		_uph_piano_render_highlight_bar(map->notes[key].highlight, box.x + rect.width, top, bottom);
	}
}

static void _uph_piano_render_labels(const Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas)
{
	const Leaf_BoundingBox box = canvas->box;
	const float font_size = NAUI_DPI(UPH_PIANO_LABEL_FONT_SIZE);
	const float label_x = box.x + box.width * piano->style.black_key_length_ratio + NAUI_DPI(24.0f);

	Leaf_Color label_color = uph_color_mix(naui_theme_color("uph_piano_natural_color"), LEAF_COLOR_BLACK, UPH_PIANO_LABEL_DARKEN_MIX);
	label_color.a = UPH_PIANO_LABEL_ALPHA;

	for (int32_t key = map->lowest; key <= map->highest; key++)
	{
		const Uph_NoteDef *def = &map->notes[key];
		if (!def->show_label || def->name[0] == '\0')
			continue;

		Uph_PianoKeyRect rect;
		if (!uph_piano_key_rect(piano, map, (uint8_t)key, canvas, &rect))
			continue;

		const float top = box.y + rect.top - canvas->scroll_y;
		const float bottom = box.y + rect.bottom - canvas->scroll_y;
		if (bottom - top < font_size + NAUI_DPI(3.0f))
			continue;

		const float label_y = top + (bottom - top - font_size) * 0.5f;
		if (label_y < box.y || label_y + font_size > box.y + box.height)
			continue;

		naui_draw_text((Naui_Vec2) { label_x, label_y }, def->name, font_size, 0, label_color);
	}
}

void uph_piano_render(const Uph_Piano *piano, const Uph_NoteMap *map, const Uph_PianoCanvas *canvas)
{
	const Leaf_BoundingBox box = canvas->box;
	naui_push_clip_rect(box.x, box.y, box.width, box.height);
	_uph_piano_render_natural_keys(piano, map, canvas);
	_uph_piano_render_accidental_keys(piano, map, canvas);
	_uph_piano_render_labels(piano, map, canvas);

	naui_draw_line(
		(Naui_Vec2) { box.x + box.width, box.y },
		(Naui_Vec2) { box.x + box.width, box.y + box.height },
		naui_theme_color("uph_timeline_top_ruler_border_color"),
		NAUI_DPI(2.0f)
	);

	naui_pop_clip_rect();
}

#pragma endregion
