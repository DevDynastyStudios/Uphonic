#define UPL_MAX_FONTS			8
#define UPL_MAX_CLIPS			32
#define UPL_GLYPH_CACHE_SIZE	4096
#define UPL_GLYPH_PADDING		1
#define UPL_SHADOW_LAYERS		8

typedef struct
{
	stbtt_fontinfo info;
	float ascent;
	float descent;
	float line_gap;
	bool loaded;
}
Upl_Font;

typedef struct
{
	float opacity;
	Upl_Clip clip;
	Upl_Clip clips[UPL_MAX_CLIPS];
	uint32_t clip_count;
	Upl_Font fonts[UPL_MAX_FONTS];
	uint32_t font_count;
	Upl_Glyph glyphs[UPL_GLYPH_CACHE_SIZE];
	uint32_t glyph_count;
	Upl_Atlas atlas;
	uint64_t image_serial;
}
Upl_RendererState;

static Upl_RendererState upl__renderer_state;

#pragma region Helpers

static inline Leaf_Color upl__fade(Leaf_Color color)
{
	color.a = (uint8_t)((float)color.a * upl__renderer_state.opacity + 0.5f);
	return color;
}

static bool upl__visible(float x0, float y0, float x1, float y1)
{
	const Upl_Clip *clip = &upl__renderer_state.clip;
	return x1 > (float)clip->x0 && y1 > (float)clip->y0 && x0 < (float)clip->x1 && y0 < (float)clip->y1 && clip->x0 < clip->x1 && clip->y0 < clip->y1;
}

static void upl__corner_radii(float width, float height, float rounding, Leaf_Corners corners, float out[4])
{
	const float max_radius = fminf(width, height) * 0.5f;
	const float radius = fmaxf(0.0f, fminf(rounding, max_radius));
	out[0] = (corners & LEAF_CORNER_TL) ? radius : 0.0f;
	out[1] = (corners & LEAF_CORNER_TR) ? radius : 0.0f;
	out[2] = (corners & LEAF_CORNER_BR) ? radius : 0.0f;
	out[3] = (corners & LEAF_CORNER_BL) ? radius : 0.0f;
}

static uint32_t upl__color_key(Leaf_Color color)
{
	return ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | (uint32_t)color.b;
}

static void upl__shadow_layers(float x, float y, float width, float height, const float radii[4], float blur, Leaf_Color color)
{
	const float spread = fmaxf(blur, 0.5f);
	int32_t layers = (int32_t)ceilf(spread);
	layers = layers < 1 ? 1 : (layers > UPL_SHADOW_LAYERS ? UPL_SHADOW_LAYERS : layers);

	const float alpha = 1.0f - powf(1.0f - (float)color.a * (1.0f / 255.0f), 1.0f / (float)layers);
	Leaf_Color layer = color;
	layer.a = (uint8_t)(alpha * 255.0f + 0.5f);
	if (layer.a == 0)
		return;

	for (int32_t i = 0; i < layers; i++)
	{
		const float expand = spread * (1.0f - 2.0f * ((float)i + 0.5f) / (float)layers);
		const float layer_width = width + expand * 2.0f;
		const float layer_height = height + expand * 2.0f;
		if (layer_width <= 0.0f || layer_height <= 0.0f)
			continue;

		float layer_radii[4];
		for (int32_t c = 0; c < 4; c++)
			layer_radii[c] = fminf(fmaxf(radii[c] + expand, 0.0f), fminf(layer_width, layer_height) * 0.5f);

		upl__gfx_fill_rect(x - expand, y - expand, layer_width, layer_height, layer_radii, layer);
	}
}

static uint8_t *upl__image_premultiply(const Upl_Image *image, Leaf_Color tint, bool bgra)
{
	const size_t count = (size_t)image->width * (size_t)image->height;
	uint8_t *out = (uint8_t *)malloc(count * 4);
	if (!out)
		return NULL;

	const uint32_t tr = tint.r, tg = tint.g, tb = tint.b;
	for (size_t i = 0; i < count; i++)
	{
		const uint8_t *src = image->pixels + i * 4;
		const uint32_t a = src[3];
		const uint8_t r = (uint8_t)((uint32_t)src[0] * tr * a / (255u * 255u));
		const uint8_t g = (uint8_t)((uint32_t)src[1] * tg * a / (255u * 255u));
		const uint8_t b = (uint8_t)((uint32_t)src[2] * tb * a / (255u * 255u));
		uint8_t *dst = out + i * 4;
		dst[0] = bgra ? b : r;
		dst[1] = g;
		dst[2] = bgra ? r : b;
		dst[3] = (uint8_t)a;
	}

	return out;
}

#pragma endregion

#pragma region Shapes

static void upl__fill_round_rect(float x, float y, float width, float height, Leaf_Color color, float rounding, Leaf_Corners corners)
{
	if (color.a == 0 || width <= 0.0f || height <= 0.0f || !upl__visible(x, y, x + width, y + height))
		return;

	float radii[4];
	upl__corner_radii(width, height, rounding, corners, radii);
	upl__gfx_fill_rect(x, y, width, height, radii, upl__fade(color));
}

static void upl__stroke_round_rect(float x, float y, float width, float height, Leaf_Color color, float line_width, float rounding, Leaf_Corners corners, Leaf_Sides sides)
{
	if (color.a == 0 || line_width <= 0.0f || width <= 0.0f || height <= 0.0f || !upl__visible(x, y, x + width, y + height))
		return;

	if (sides != LEAF_SIDE_ALL)
	{
		if (sides & LEAF_SIDE_TOP)
			upl__fill_round_rect(x, y, width, line_width, color, 0.0f, LEAF_CORNER_NONE);

		if (sides & LEAF_SIDE_BOTTOM)
			upl__fill_round_rect(x, y + height - line_width, width, line_width, color, 0.0f, LEAF_CORNER_NONE);

		if (sides & LEAF_SIDE_LEFT)
			upl__fill_round_rect(x, y, line_width, height, color, 0.0f, LEAF_CORNER_NONE);

		if (sides & LEAF_SIDE_RIGHT)
			upl__fill_round_rect(x + width - line_width, y, line_width, height, color, 0.0f, LEAF_CORNER_NONE);

		return;
	}

	float radii[4];
	upl__corner_radii(width, height, rounding, corners, radii);
	upl__gfx_stroke_rect(x, y, width, height, radii, fminf(line_width, fminf(width, height) * 0.5f), upl__fade(color));
}

static void upl__fill_shadow(float x, float y, float width, float height, Leaf_Color color, float blur, float rounding, Leaf_Corners corners)
{
	if (color.a == 0 || width <= 0.0f || height <= 0.0f || !upl__visible(x - blur, y - blur, x + width + blur, y + height + blur))
		return;

	float radii[4];
	upl__corner_radii(width, height, rounding, corners, radii);
	upl__gfx_shadow(x, y, width, height, radii, blur, upl__fade(color));
}

#pragma endregion

#pragma region Text

static uint32_t upl__font_add(const void *data)
{
	Upl_RendererState *state = &upl__renderer_state;
	if (state->font_count >= UPL_MAX_FONTS)
		return UPL_DEFAULT_FONT;

	Upl_Font *font = &state->fonts[state->font_count];
	const unsigned char *bytes = (const unsigned char *)data;
	if (!stbtt_InitFont(&font->info, bytes, stbtt_GetFontOffsetForIndex(bytes, 0)))
		return UPL_DEFAULT_FONT;

	int ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
	const float unit = 1.0f / (float)(ascent - descent);
	font->ascent = (float)ascent * unit;
	font->descent = (float)descent * unit;
	font->line_gap = (float)line_gap * unit;
	font->loaded = true;
	return state->font_count++;
}

static Upl_Font *upl__font_get(uint32_t font_id)
{
	Upl_RendererState *state = &upl__renderer_state;
	if (font_id < state->font_count && state->fonts[font_id].loaded)
		return &state->fonts[font_id];

	return state->font_count > 0 ? &state->fonts[UPL_DEFAULT_FONT] : NULL;
}

static void upl__glyph_cache_clear(void)
{
	Upl_RendererState *state = &upl__renderer_state;
	upl__gfx_atlas_reset();

	for (uint32_t i = 0; i < UPL_GLYPH_CACHE_SIZE; i++)
	{
		if (state->glyphs[i].native)
			upl__gfx_glyph_release(&state->glyphs[i]);
	}

	memset(state->glyphs, 0, sizeof(state->glyphs));
	memset(state->atlas.pixels, 0, sizeof(state->atlas.pixels));
	state->glyph_count = 0;
	state->atlas.cursor_x = 0;
	state->atlas.cursor_y = 0;
	state->atlas.row_height = 0;
	state->atlas.generation++;
}

static bool upl__atlas_alloc(Upl_Atlas *atlas, int32_t width, int32_t height, int32_t *x, int32_t *y)
{
	const int32_t padded_width = width + UPL_GLYPH_PADDING;
	const int32_t padded_height = height + UPL_GLYPH_PADDING;
	if (padded_width > UPL_ATLAS_SIZE || padded_height > UPL_ATLAS_SIZE)
		return false;

	if (atlas->cursor_x + padded_width > UPL_ATLAS_SIZE)
	{
		atlas->cursor_x = 0;
		atlas->cursor_y += atlas->row_height;
		atlas->row_height = 0;
	}

	if (atlas->cursor_y + padded_height > UPL_ATLAS_SIZE)
		return false;

	*x = atlas->cursor_x;
	*y = atlas->cursor_y;
	atlas->cursor_x += padded_width;
	atlas->row_height = padded_height > atlas->row_height ? padded_height : atlas->row_height;
	return true;
}

static Upl_Glyph *upl__glyph_get(uint32_t font_id, Upl_Font *font, int32_t size, int32_t codepoint)
{
	Upl_RendererState *state = &upl__renderer_state;
	const uint32_t hash = ((uint32_t)codepoint * 2654435761u) ^ ((uint32_t)size * 40503u) ^ (font_id * 97u);
	uint32_t index = hash % UPL_GLYPH_CACHE_SIZE;

	while (state->glyphs[index].used)
	{
		Upl_Glyph *glyph = &state->glyphs[index];
		if (glyph->codepoint == codepoint && glyph->size == size && glyph->font_id == font_id)
			return glyph;

		index = (index + 1) % UPL_GLYPH_CACHE_SIZE;
	}

	if (state->glyph_count >= UPL_GLYPH_CACHE_SIZE * 3 / 4)
	{
		upl__glyph_cache_clear();
		index = hash % UPL_GLYPH_CACHE_SIZE;
	}

	const float scale = stbtt_ScaleForPixelHeight(&font->info, (float)size);
	int advance, left_bearing, x0, y0, x1, y1;
	stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &left_bearing);
	stbtt_GetCodepointBitmapBox(&font->info, codepoint, scale, scale, &x0, &y0, &x1, &y1);

	int32_t width = x1 - x0;
	int32_t height = y1 - y0;
	int32_t atlas_x = 0;
	int32_t atlas_y = 0;
	if (width > 0 && height > 0 && !upl__atlas_alloc(&state->atlas, width, height, &atlas_x, &atlas_y))
	{
		upl__glyph_cache_clear();
		index = hash % UPL_GLYPH_CACHE_SIZE;
		if (!upl__atlas_alloc(&state->atlas, width, height, &atlas_x, &atlas_y))
			width = height = 0;
	}

	if (width > 0 && height > 0)
	{
		stbtt_MakeCodepointBitmap(&font->info, state->atlas.pixels + atlas_y * UPL_ATLAS_SIZE + atlas_x, width, height, UPL_ATLAS_SIZE, scale, scale, codepoint);
		state->atlas.generation++;
	}

	Upl_Glyph *glyph = &state->glyphs[index];
	*glyph = (Upl_Glyph) {
		.font_id = font_id,
		.size = size,
		.codepoint = codepoint,
		.atlas_x = atlas_x,
		.atlas_y = atlas_y,
		.width = width > 0 && height > 0 ? width : 0,
		.height = width > 0 && height > 0 ? height : 0,
		.x_offset = x0,
		.y_offset = y0,
		.advance = (float)advance * scale,
		.used = true
	};

	state->glyph_count++;
	return glyph;
}

static uint32_t upl__utf8_decode(const char *text, uint32_t length, uint32_t i, int32_t *codepoint)
{
	const uint8_t c = (uint8_t)text[i];
	uint32_t bytes = 1;
	int32_t cp = c;

	if (c >= 0xF0)
	{
		bytes = 4;
		cp = c & 0x07;
	}
	else if (c >= 0xE0)
	{
		bytes = 3;
		cp = c & 0x0F;
	}
	else if (c >= 0xC0)
	{
		bytes = 2;
		cp = c & 0x1F;
	}

	if (i + bytes > length)
	{
		*codepoint = '?';
		return length - i;
	}

	for (uint32_t b = 1; b < bytes; b++)
		cp = (cp << 6) | ((uint8_t)text[i + b] & 0x3F);

	*codepoint = cp;
	return bytes;
}

static void upl__draw_text_n(float x, float y, const char *text, uint32_t length, int32_t font_size, uint32_t font_id, Leaf_Color color)
{
	Upl_RendererState *state = &upl__renderer_state;
	Upl_Font *font = upl__font_get(font_id);
	if (!font || color.a == 0 || font_size <= 0)
		return;

	if (font_id >= state->font_count)
		font_id = UPL_DEFAULT_FONT;

	const Leaf_Color faded = upl__fade(color);
	const float scale = stbtt_ScaleForPixelHeight(&font->info, (float)font_size);
	const int32_t baseline = (int32_t)floorf(y + font->ascent * (float)font_size + 0.5f);
	float pen_x = x;
	int32_t previous = 0;

	for (uint32_t i = 0; i < length;)
	{
		int32_t codepoint;
		i += upl__utf8_decode(text, length, i, &codepoint);

		if (previous)
			pen_x += (float)stbtt_GetCodepointKernAdvance(&font->info, previous, codepoint) * scale;

		Upl_Glyph *glyph = upl__glyph_get(font_id, font, font_size, codepoint);
		const float gx = floorf(pen_x + 0.5f) + (float)glyph->x_offset;
		const float gy = (float)(baseline + glyph->y_offset);
		if (glyph->width > 0 && upl__visible(gx, gy, gx + (float)glyph->width, gy + (float)glyph->height))
			upl__gfx_glyph(&state->atlas, glyph, gx, gy, faded);

		pen_x += glyph->advance;
		previous = codepoint;
	}
}

#pragma endregion

#pragma region Images

static void upl__draw_image(Upl_Image *image, float x, float y, float width, float height, Leaf_Color tint, float rounding, Leaf_Corners corners)
{
	if (!image || tint.a == 0 || width <= 0.0f || height <= 0.0f || !upl__visible(x, y, x + width, y + height))
		return;

	float radii[4];
	upl__corner_radii(width, height, rounding, corners, radii);
	upl__gfx_image(image, x, y, width, height, radii, upl__fade(tint));
}

#pragma endregion

#pragma region Frame

static void upl__renderer_begin(int32_t width, int32_t height, Leaf_Color clear)
{
	Upl_RendererState *state = &upl__renderer_state;
	state->opacity = 1.0f;
	state->clip = (Upl_Clip) { 0, 0, width, height };
	state->clip_count = 0;
	upl__gfx_clear(clear);
}

static void upl__clip_push(int32_t x, int32_t y, int32_t width, int32_t height)
{
	Upl_RendererState *state = &upl__renderer_state;
	if (state->clip_count < UPL_MAX_CLIPS)
		state->clips[state->clip_count++] = state->clip;

	Upl_Clip clip = state->clip;
	clip.x0 = x > clip.x0 ? x : clip.x0;
	clip.y0 = y > clip.y0 ? y : clip.y0;
	clip.x1 = x + width < clip.x1 ? x + width : clip.x1;
	clip.y1 = y + height < clip.y1 ? y + height : clip.y1;
	state->clip = clip;
	upl__gfx_set_clip(&state->clip);
}

static void upl__clip_pop(void)
{
	Upl_RendererState *state = &upl__renderer_state;
	if (state->clip_count == 0)
		return;

	state->clip = state->clips[--state->clip_count];
	upl__gfx_set_clip(state->clip_count > 0 ? &state->clip : NULL);
}

static void upl__render_commands(const Leaf_RenderCmdList *list)
{
	Upl_RendererState *state = &upl__renderer_state;
	for (const Leaf_RenderCmdNode *node = list->first; node; node = node->next)
	{
		const Leaf_RenderCmd *cmd = &node->cmd;
		const Leaf_BoundingBox box = cmd->bounding_box;
		state->opacity = cmd->opacity;

		switch (cmd->type)
		{
			case LEAF_RENDER_CMD_RECT:
				upl__fill_round_rect((float)box.x, (float)box.y, (float)box.width, (float)box.height, cmd->color, (float)cmd->rect.rounding, cmd->rect.rounding_corners);
				break;
			case LEAF_RENDER_CMD_RECT_LINES:
				upl__stroke_round_rect((float)box.x, (float)box.y, (float)box.width, (float)box.height, cmd->color, (float)cmd->rect.line_width, (float)cmd->rect.rounding, cmd->rect.rounding_corners, cmd->rect.sides);
				break;
			case LEAF_RENDER_CMD_TEXT:
				upl__draw_text_n((float)box.x, (float)box.y, cmd->text.text, (uint32_t)strlen(cmd->text.text), cmd->text.font_size, cmd->text.font_id, cmd->color);
				break;
			case LEAF_RENDER_CMD_IMAGE:
				upl__draw_image((Upl_Image *)cmd->image.handle, (float)box.x, (float)box.y, (float)box.width, (float)box.height, cmd->color, (float)cmd->image.rounding, cmd->image.rounding_corners);
				break;
			case LEAF_RENDER_CMD_SCISSOR_PUSH:
				upl__clip_push(box.x, box.y, box.width, box.height);
				break;
			case LEAF_RENDER_CMD_SCISSOR_POP:
				upl__clip_pop();
				break;
			case LEAF_RENDER_CMD_CUSTOM:
				if (cmd->custom.draw)
					cmd->custom.draw(box, cmd->custom.user_data);
				break;
			case LEAF_RENDER_CMD_SHADOW:
			{
				const float x = (float)(box.x + cmd->shadow.offset.x);
				const float y = (float)(box.y + cmd->shadow.offset.y);
				upl__fill_shadow(x, y, (float)box.width, (float)box.height, cmd->color, (float)cmd->shadow.blur_radius, (float)cmd->shadow.rounding, cmd->shadow.rounding_corners);
				break;
			}
		}
	}

	state->opacity = 1.0f;
	while (state->clip_count > 0)
		upl__clip_pop();
}

#pragma endregion

#pragma region Public Drawing

uint32_t upl_font_load(const void *ttf_data, size_t size)
{
	(void)size;
	return upl__font_add(ttf_data);
}

Upl_Image *upl_image_create(const uint8_t *rgba, int32_t width, int32_t height)
{
	if (!rgba || width <= 0 || height <= 0)
		return NULL;

	Upl_Image *image = (Upl_Image *)calloc(1, sizeof(Upl_Image));
	image->pixels = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
	image->width = width;
	image->height = height;
	image->serial = ++upl__renderer_state.image_serial;
	memcpy(image->pixels, rgba, (size_t)width * (size_t)height * 4);
	return image;
}

void upl_image_destroy(Upl_Image *image)
{
	if (!image)
		return;

	upl__gfx_image_release(image);
	free(image->pixels);
	free(image);
}

void upl_fill_rect(float x, float y, float width, float height, Leaf_Color color, float rounding)
{
	upl__fill_round_rect(x, y, width, height, color, rounding, LEAF_CORNER_ALL);
}

void upl_stroke_rect(float x, float y, float width, float height, Leaf_Color color, float line_width, float rounding)
{
	upl__stroke_round_rect(x, y, width, height, color, line_width, rounding, LEAF_CORNER_ALL, LEAF_SIDE_ALL);
}

void upl_draw_line(float x0, float y0, float x1, float y1, Leaf_Color color, float width)
{
	const float half = width * 0.5f + 1.0f;
	if (color.a == 0 || width <= 0.0f || !upl__visible(fminf(x0, x1) - half, fminf(y0, y1) - half, fmaxf(x0, x1) + half, fmaxf(y0, y1) + half))
		return;

	upl__gfx_line(x0, y0, x1, y1, width, upl__fade(color));
}

void upl_fill_circle(float cx, float cy, float radius, Leaf_Color color)
{
	if (color.a == 0 || radius <= 0.0f || !upl__visible(cx - radius - 1.0f, cy - radius - 1.0f, cx + radius + 1.0f, cy + radius + 1.0f))
		return;

	upl__gfx_fill_circle(cx, cy, radius, upl__fade(color));
}

void upl_stroke_arc(float cx, float cy, float radius, float start_angle, float end_angle, Leaf_Color color, float width)
{
	const float extent = radius + width * 0.5f + 1.0f;
	if (color.a == 0 || width <= 0.0f || end_angle <= start_angle || !upl__visible(cx - extent, cy - extent, cx + extent, cy + extent))
		return;

	upl__gfx_stroke_arc(cx, cy, radius, start_angle, fminf(end_angle - start_angle, 6.28318530718f), width, upl__fade(color));
}

void upl_draw_text(float x, float y, const char *text, int32_t font_size, uint32_t font_id, Leaf_Color color)
{
	upl__draw_text_n(x, y, text, (uint32_t)strlen(text), font_size, font_id, color);
}

void upl_draw_image(Upl_Image *image, float x, float y, float width, float height, Leaf_Color tint, float rounding)
{
	upl__draw_image(image, x, y, width, height, tint, rounding, LEAF_CORNER_ALL);
}

Leaf_Dimensions upl_measure_text(const char *text, uint32_t length, int32_t font_size, uint32_t font_id)
{
	Upl_RendererState *state = &upl__renderer_state;
	Upl_Font *font = upl__font_get(font_id);
	if (!font || font_size <= 0)
		return (Leaf_Dimensions) { 0.0f, 0.0f };

	if (font_id >= state->font_count)
		font_id = UPL_DEFAULT_FONT;

	const float scale = stbtt_ScaleForPixelHeight(&font->info, (float)font_size);
	float width = 0.0f;
	int32_t previous = 0;
	for (uint32_t i = 0; i < length;)
	{
		int32_t codepoint;
		i += upl__utf8_decode(text, length, i, &codepoint);

		if (previous)
			width += (float)stbtt_GetCodepointKernAdvance(&font->info, previous, codepoint) * scale;

		int advance, left_bearing;
		stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &left_bearing);
		width += (float)advance * scale;
		previous = codepoint;
	}

	const float height = (font->ascent - font->descent + font->line_gap) * (float)font_size;
	return (Leaf_Dimensions) { ceilf(width), ceilf(height) };
}

#pragma endregion
