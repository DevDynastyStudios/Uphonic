#define UPL_MAX_FONTS			8
#define UPL_MAX_CLIPS			32
#define UPL_GLYPH_CACHE_SIZE	4096

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
	uint8_t *bitmap;
	uint32_t font_id;
	int32_t size;
	int32_t codepoint;
	int32_t width;
	int32_t height;
	int32_t x_offset;
	int32_t y_offset;
	float advance;
	bool used;
}
Upl_Glyph;

typedef struct
{
	int32_t x0;
	int32_t y0;
	int32_t x1;
	int32_t y1;
}
Upl_Clip;

struct Upl_Image
{
	uint8_t *pixels;
	int32_t width;
	int32_t height;
};

typedef struct
{
	uint32_t *pixels;
	int32_t width;
	int32_t height;
	float opacity;
	Upl_Clip clip;
	Upl_Clip clips[UPL_MAX_CLIPS];
	uint32_t clip_count;
	Upl_Font fonts[UPL_MAX_FONTS];
	uint32_t font_count;
	Upl_Glyph glyphs[UPL_GLYPH_CACHE_SIZE];
	uint32_t glyph_count;
}
Upl_RendererState;

static Upl_RendererState upl__renderer_state;

#pragma region Pixels

static inline float upl__clamp01(float v)
{
	return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static inline void upl__blend(uint32_t *dst, uint8_t r, uint8_t g, uint8_t b, float alpha)
{
	if (alpha <= 0.0f)
		return;

	if (alpha >= 1.0f)
	{
		*dst = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
		return;
	}

	const uint32_t d = *dst;
	const int32_t a = (int32_t)(alpha * 256.0f);
	const int32_t dr = (int32_t)((d >> 16) & 0xFF);
	const int32_t dg = (int32_t)((d >> 8) & 0xFF);
	const int32_t db = (int32_t)(d & 0xFF);
	const uint32_t nr = (uint32_t)(dr + ((((int32_t)r - dr) * a) >> 8));
	const uint32_t ng = (uint32_t)(dg + ((((int32_t)g - dg) * a) >> 8));
	const uint32_t nb = (uint32_t)(db + ((((int32_t)b - db) * a) >> 8));
	*dst = (nr << 16) | (ng << 8) | nb;
}

static inline void upl__plot(int32_t x, int32_t y, Leaf_Color color, float coverage)
{
	Upl_RendererState *state = &upl__renderer_state;
	upl__blend(&state->pixels[y * state->width + x], color.r, color.g, color.b, coverage * (float)color.a * (1.0f / 255.0f) * state->opacity);
}

static bool upl__bounds(float x0, float y0, float x1, float y1, Upl_Clip *out)
{
	Upl_RendererState *state = &upl__renderer_state;
	out->x0 = (int32_t)floorf(x0);
	out->y0 = (int32_t)floorf(y0);
	out->x1 = (int32_t)ceilf(x1);
	out->y1 = (int32_t)ceilf(y1);
	out->x0 = out->x0 > state->clip.x0 ? out->x0 : state->clip.x0;
	out->y0 = out->y0 > state->clip.y0 ? out->y0 : state->clip.y0;
	out->x1 = out->x1 < state->clip.x1 ? out->x1 : state->clip.x1;
	out->y1 = out->y1 < state->clip.y1 ? out->y1 : state->clip.y1;
	return out->x0 < out->x1 && out->y0 < out->y1;
}

#pragma endregion

#pragma region Shapes

static void upl__corner_radii(float width, float height, float rounding, Leaf_Corners corners, float out[4])
{
	const float max_radius = fminf(width, height) * 0.5f;
	const float radius = fmaxf(0.0f, fminf(rounding, max_radius));
	out[0] = (corners & LEAF_CORNER_TL) ? radius : 0.0f;
	out[1] = (corners & LEAF_CORNER_TR) ? radius : 0.0f;
	out[2] = (corners & LEAF_CORNER_BR) ? radius : 0.0f;
	out[3] = (corners & LEAF_CORNER_BL) ? radius : 0.0f;
}

static inline float upl__sd_round_box(float px, float py, float cx, float cy, float hx, float hy, const float radii[4])
{
	px -= cx;
	py -= cy;
	const float radius = px < 0.0f ? (py < 0.0f ? radii[0] : radii[3]) : (py < 0.0f ? radii[1] : radii[2]);
	const float qx = fabsf(px) - hx + radius;
	const float qy = fabsf(py) - hy + radius;
	const float ox = fmaxf(qx, 0.0f);
	const float oy = fmaxf(qy, 0.0f);
	return fminf(fmaxf(qx, qy), 0.0f) + sqrtf(ox * ox + oy * oy) - radius;
}

static void upl__fill_round_rect(float x, float y, float width, float height, Leaf_Color color, float rounding, Leaf_Corners corners)
{
	Upl_Clip b;
	if (color.a == 0 || width <= 0.0f || height <= 0.0f || !upl__bounds(x, y, x + width, y + height, &b))
		return;

	float radii[4];
	upl__corner_radii(width, height, rounding, corners, radii);
	const float max_radius = fmaxf(fmaxf(radii[0], radii[1]), fmaxf(radii[2], radii[3]));
	const float hx = width * 0.5f;
	const float hy = height * 0.5f;
	const float cx = x + hx;
	const float cy = y + hy;

	for (int32_t py = b.y0; py < b.y1; py++)
	{
		const float fy = (float)py + 0.5f;
		const bool row_interior = fy >= y + max_radius && fy <= y + height - max_radius;
		for (int32_t px = b.x0; px < b.x1; px++)
		{
			const float fx = (float)px + 0.5f;
			if (row_interior || (fx >= x + max_radius && fx <= x + width - max_radius))
			{
				upl__plot(px, py, color, 1.0f);
				continue;
			}

			upl__plot(px, py, color, upl__clamp01(0.5f - upl__sd_round_box(fx, fy, cx, cy, hx, hy, radii)));
		}
	}
}

static void upl__stroke_round_rect(float x, float y, float width, float height, Leaf_Color color, float line_width, float rounding, Leaf_Corners corners, Leaf_Sides sides)
{
	if (color.a == 0 || line_width <= 0.0f || width <= 0.0f || height <= 0.0f)
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

	Upl_Clip b;
	if (!upl__bounds(x, y, x + width, y + height, &b))
		return;

	float outer[4];
	float inner[4];
	upl__corner_radii(width, height, rounding, corners, outer);
	for (int32_t i = 0; i < 4; i++)
		inner[i] = fmaxf(outer[i] - line_width, 0.0f);

	const float hx = width * 0.5f;
	const float hy = height * 0.5f;
	const float cx = x + hx;
	const float cy = y + hy;
	const float inner_hx = fmaxf(hx - line_width, 0.0f);
	const float inner_hy = fmaxf(hy - line_width, 0.0f);

	for (int32_t py = b.y0; py < b.y1; py++)
	{
		const float fy = (float)py + 0.5f;
		for (int32_t px = b.x0; px < b.x1; px++)
		{
			const float fx = (float)px + 0.5f;
			const float outside = upl__clamp01(0.5f - upl__sd_round_box(fx, fy, cx, cy, hx, hy, outer));
			const float hole = (inner_hx > 0.0f && inner_hy > 0.0f) ? upl__clamp01(0.5f - upl__sd_round_box(fx, fy, cx, cy, inner_hx, inner_hy, inner)) : 0.0f;
			upl__plot(px, py, color, outside * (1.0f - hole));
		}
	}
}

static void upl__fill_shadow(float x, float y, float width, float height, Leaf_Color color, float blur, float rounding, Leaf_Corners corners)
{
	Upl_Clip b;
	if (color.a == 0 || !upl__bounds(x - blur, y - blur, x + width + blur, y + height + blur, &b))
		return;

	float radii[4];
	upl__corner_radii(width, height, rounding, corners, radii);
	const float hx = width * 0.5f;
	const float hy = height * 0.5f;
	const float cx = x + hx;
	const float cy = y + hy;
	const float spread = fmaxf(blur, 0.5f);

	for (int32_t py = b.y0; py < b.y1; py++)
	{
		const float fy = (float)py + 0.5f;
		for (int32_t px = b.x0; px < b.x1; px++)
		{
			const float fx = (float)px + 0.5f;
			const float t = upl__clamp01((upl__sd_round_box(fx, fy, cx, cy, hx, hy, radii) + spread) / (2.0f * spread));
			upl__plot(px, py, color, 1.0f - t * t * (3.0f - 2.0f * t));
		}
	}
}

static inline float upl__sd_segment(float px, float py, float x0, float y0, float x1, float y1)
{
	const float pax = px - x0;
	const float pay = py - y0;
	const float bax = x1 - x0;
	const float bay = y1 - y0;
	const float length_sq = bax * bax + bay * bay;
	const float h = length_sq > 0.0f ? upl__clamp01((pax * bax + pay * bay) / length_sq) : 0.0f;
	const float dx = pax - bax * h;
	const float dy = pay - bay * h;
	return sqrtf(dx * dx + dy * dy);
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
	for (uint32_t i = 0; i < UPL_GLYPH_CACHE_SIZE; i++)
	{
		if (state->glyphs[i].bitmap)
			stbtt_FreeBitmap(state->glyphs[i].bitmap, NULL);
	}

	memset(state->glyphs, 0, sizeof(state->glyphs));
	state->glyph_count = 0;
}

static const Upl_Glyph *upl__glyph_get(uint32_t font_id, Upl_Font *font, int32_t size, int32_t codepoint)
{
	Upl_RendererState *state = &upl__renderer_state;
	const uint32_t hash = ((uint32_t)codepoint * 2654435761u) ^ ((uint32_t)size * 40503u) ^ (font_id * 97u);
	uint32_t index = hash % UPL_GLYPH_CACHE_SIZE;

	while (state->glyphs[index].used)
	{
		const Upl_Glyph *glyph = &state->glyphs[index];
		if (glyph->codepoint == codepoint && glyph->size == size && glyph->font_id == font_id)
			return glyph;

		index = (index + 1) % UPL_GLYPH_CACHE_SIZE;
	}

	if (state->glyph_count >= UPL_GLYPH_CACHE_SIZE * 3 / 4)
	{
		upl__glyph_cache_clear();
		index = hash % UPL_GLYPH_CACHE_SIZE;
	}

	Upl_Glyph *glyph = &state->glyphs[index];
	const float scale = stbtt_ScaleForPixelHeight(&font->info, (float)size);
	int advance, left_bearing;
	stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &left_bearing);

	glyph->bitmap = stbtt_GetCodepointBitmap(&font->info, 0.0f, scale, codepoint, &glyph->width, &glyph->height, &glyph->x_offset, &glyph->y_offset);
	glyph->font_id = font_id;
	glyph->size = size;
	glyph->codepoint = codepoint;
	glyph->advance = (float)advance * scale;
	glyph->used = true;
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

		const Upl_Glyph *glyph = upl__glyph_get(font_id, font, font_size, codepoint);
		const int32_t gx = (int32_t)floorf(pen_x + 0.5f) + glyph->x_offset;
		const int32_t gy = baseline + glyph->y_offset;
		Upl_Clip b;
		if (glyph->bitmap && upl__bounds((float)gx, (float)gy, (float)(gx + glyph->width), (float)(gy + glyph->height), &b))
		{
			for (int32_t py = b.y0; py < b.y1; py++)
			{
				const uint8_t *row = glyph->bitmap + (py - gy) * glyph->width;
				for (int32_t px = b.x0; px < b.x1; px++)
				{
					const uint8_t coverage = row[px - gx];
					if (coverage)
						upl__plot(px, py, color, (float)coverage * (1.0f / 255.0f));
				}
			}
		}

		pen_x += glyph->advance;
		previous = codepoint;
	}
}

#pragma endregion

#pragma region Images

static inline void upl__image_sample(const Upl_Image *image, float u, float v, float out[4])
{
	const float fx = fminf(fmaxf(u * (float)image->width - 0.5f, 0.0f), (float)(image->width - 1));
	const float fy = fminf(fmaxf(v * (float)image->height - 0.5f, 0.0f), (float)(image->height - 1));
	const int32_t x0 = (int32_t)fx;
	const int32_t y0 = (int32_t)fy;
	const int32_t x1 = x0 + 1 < image->width ? x0 + 1 : x0;
	const int32_t y1 = y0 + 1 < image->height ? y0 + 1 : y0;
	const float tx = fx - (float)x0;
	const float ty = fy - (float)y0;
	const uint8_t *p00 = image->pixels + (y0 * image->width + x0) * 4;
	const uint8_t *p10 = image->pixels + (y0 * image->width + x1) * 4;
	const uint8_t *p01 = image->pixels + (y1 * image->width + x0) * 4;
	const uint8_t *p11 = image->pixels + (y1 * image->width + x1) * 4;

	for (int32_t c = 0; c < 4; c++)
	{
		const float top = (float)p00[c] + ((float)p10[c] - (float)p00[c]) * tx;
		const float bottom = (float)p01[c] + ((float)p11[c] - (float)p01[c]) * tx;
		out[c] = top + (bottom - top) * ty;
	}
}

static void upl__draw_image(const Upl_Image *image, float x, float y, float width, float height, Leaf_Color tint, float rounding, Leaf_Corners corners)
{
	Upl_RendererState *state = &upl__renderer_state;
	Upl_Clip b;
	if (!image || tint.a == 0 || width <= 0.0f || height <= 0.0f || !upl__bounds(x, y, x + width, y + height, &b))
		return;

	float radii[4];
	upl__corner_radii(width, height, rounding, corners, radii);
	const bool rounded = radii[0] > 0.0f || radii[1] > 0.0f || radii[2] > 0.0f || radii[3] > 0.0f;
	const float hx = width * 0.5f;
	const float hy = height * 0.5f;

	for (int32_t py = b.y0; py < b.y1; py++)
	{
		const float fy = (float)py + 0.5f;
		for (int32_t px = b.x0; px < b.x1; px++)
		{
			const float fx = (float)px + 0.5f;
			float sample[4];
			upl__image_sample(image, (fx - x) / width, (fy - y) / height, sample);

			float alpha = sample[3] * (1.0f / 255.0f) * (float)tint.a * (1.0f / 255.0f) * state->opacity;
			if (rounded)
				alpha *= upl__clamp01(0.5f - upl__sd_round_box(fx, fy, x + hx, y + hy, hx, hy, radii));

			upl__blend(
				&state->pixels[py * state->width + px],
				(uint8_t)(sample[0] * (float)tint.r * (1.0f / 255.0f)),
				(uint8_t)(sample[1] * (float)tint.g * (1.0f / 255.0f)),
				(uint8_t)(sample[2] * (float)tint.b * (1.0f / 255.0f)),
				alpha
			);
		}
	}
}

#pragma endregion

#pragma region Frame

static void upl__renderer_begin(uint32_t *pixels, int32_t width, int32_t height, Leaf_Color clear)
{
	Upl_RendererState *state = &upl__renderer_state;
	state->pixels = pixels;
	state->width = width;
	state->height = height;
	state->opacity = 1.0f;
	state->clip = (Upl_Clip) { 0, 0, width, height };
	state->clip_count = 0;

	const uint32_t value = ((uint32_t)clear.r << 16) | ((uint32_t)clear.g << 8) | (uint32_t)clear.b;
	for (int32_t i = 0; i < width * height; i++)
		pixels[i] = value;
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
}

static void upl__clip_pop(void)
{
	Upl_RendererState *state = &upl__renderer_state;
	if (state->clip_count > 0)
		state->clip = state->clips[--state->clip_count];
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
				upl__draw_image((const Upl_Image *)cmd->image.handle, (float)box.x, (float)box.y, (float)box.width, (float)box.height, cmd->color, (float)cmd->image.rounding, cmd->image.rounding_corners);
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

	Upl_Image *image = (Upl_Image *)malloc(sizeof(Upl_Image));
	image->pixels = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
	image->width = width;
	image->height = height;
	memcpy(image->pixels, rgba, (size_t)width * (size_t)height * 4);
	return image;
}

void upl_image_destroy(Upl_Image *image)
{
	if (!image)
		return;

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
	const float half = width * 0.5f;
	Upl_Clip b;
	if (color.a == 0 || !upl__bounds(fminf(x0, x1) - half - 1.0f, fminf(y0, y1) - half - 1.0f, fmaxf(x0, x1) + half + 1.0f, fmaxf(y0, y1) + half + 1.0f, &b))
		return;

	for (int32_t py = b.y0; py < b.y1; py++)
	{
		for (int32_t px = b.x0; px < b.x1; px++)
			upl__plot(px, py, color, upl__clamp01(half + 0.5f - upl__sd_segment((float)px + 0.5f, (float)py + 0.5f, x0, y0, x1, y1)));
	}
}

void upl_fill_circle(float cx, float cy, float radius, Leaf_Color color)
{
	Upl_Clip b;
	if (color.a == 0 || !upl__bounds(cx - radius - 1.0f, cy - radius - 1.0f, cx + radius + 1.0f, cy + radius + 1.0f, &b))
		return;

	for (int32_t py = b.y0; py < b.y1; py++)
	{
		const float dy = (float)py + 0.5f - cy;
		for (int32_t px = b.x0; px < b.x1; px++)
		{
			const float dx = (float)px + 0.5f - cx;
			upl__plot(px, py, color, upl__clamp01(radius + 0.5f - sqrtf(dx * dx + dy * dy)));
		}
	}
}

void upl_stroke_arc(float cx, float cy, float radius, float start_angle, float end_angle, Leaf_Color color, float width)
{
	const float half = width * 0.5f;
	const float extent = radius + half + 1.0f;
	Upl_Clip b;
	if (color.a == 0 || end_angle <= start_angle || !upl__bounds(cx - extent, cy - extent, cx + extent, cy + extent, &b))
		return;

	const float two_pi = 6.28318530718f;
	const float sweep = fminf(end_angle - start_angle, two_pi);
	const float sx = cx + cosf(start_angle) * radius;
	const float sy = cy + sinf(start_angle) * radius;
	const float ex = cx + cosf(start_angle + sweep) * radius;
	const float ey = cy + sinf(start_angle + sweep) * radius;

	for (int32_t py = b.y0; py < b.y1; py++)
	{
		const float fy = (float)py + 0.5f;
		for (int32_t px = b.x0; px < b.x1; px++)
		{
			const float fx = (float)px + 0.5f;
			const float dx = fx - cx;
			const float dy = fy - cy;
			float angle = atan2f(dy, dx) - start_angle;
			angle = fmodf(angle, two_pi);
			if (angle < 0.0f)
				angle += two_pi;

			float distance;
			if (angle <= sweep)
			{
				distance = fabsf(sqrtf(dx * dx + dy * dy) - radius);
			}
			else
			{
				const float ds = sqrtf((fx - sx) * (fx - sx) + (fy - sy) * (fy - sy));
				const float de = sqrtf((fx - ex) * (fx - ex) + (fy - ey) * (fy - ey));
				distance = fminf(ds, de);
			}

			upl__plot(px, py, color, upl__clamp01(half + 0.5f - distance));
		}
	}
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
