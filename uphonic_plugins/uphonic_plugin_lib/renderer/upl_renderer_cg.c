#include <CoreGraphics/CoreGraphics.h>

typedef struct
{
	CGContextRef context;
	CGColorSpaceRef color_space;
	float width;
	float height;
	bool clipped;
}
Upl_CgState;

static Upl_CgState upl__cg_state;

#pragma region Helpers

static void upl__cg_fill_color(Leaf_Color color)
{
	CGContextSetRGBFillColor(upl__cg_state.context, (CGFloat)color.r / 255.0, (CGFloat)color.g / 255.0, (CGFloat)color.b / 255.0, (CGFloat)color.a / 255.0);
}

static void upl__cg_stroke_color(Leaf_Color color)
{
	CGContextSetRGBStrokeColor(upl__cg_state.context, (CGFloat)color.r / 255.0, (CGFloat)color.g / 255.0, (CGFloat)color.b / 255.0, (CGFloat)color.a / 255.0);
}

static bool upl__cg_square(const float radii[4])
{
	return radii[0] <= 0.0f && radii[1] <= 0.0f && radii[2] <= 0.0f && radii[3] <= 0.0f;
}

static void upl__cg_round_rect_path(float x, float y, float width, float height, const float radii[4])
{
	CGContextRef context = upl__cg_state.context;
	CGContextBeginPath(context);
	CGContextMoveToPoint(context, x + radii[0], y);
	CGContextAddArcToPoint(context, x + width, y, x + width, y + height, radii[1]);
	CGContextAddArcToPoint(context, x + width, y + height, x, y + height, radii[2]);
	CGContextAddArcToPoint(context, x, y + height, x, y, radii[3]);
	CGContextAddArcToPoint(context, x, y, x + width, y, radii[0]);
	CGContextClosePath(context);
}

static CGImageRef upl__cg_image_create(const uint8_t *pixels, int32_t width, int32_t height)
{
	CFDataRef data = CFDataCreate(kCFAllocatorDefault, pixels, (CFIndex)width * height * 4);
	CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
	CGImageRef image = CGImageCreate(
		(size_t)width, (size_t)height, 8, 32, (size_t)width * 4,
		upl__cg_state.color_space,
		kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedLast,
		provider, NULL, true, kCGRenderingIntentDefault
	);

	CGDataProviderRelease(provider);
	CFRelease(data);
	return image;
}

static CGImageRef upl__cg_glyph_mask(const Upl_Atlas *atlas, const Upl_Glyph *glyph)
{
	const size_t width = (size_t)glyph->width;
	const size_t height = (size_t)glyph->height;
	CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault, (CFIndex)(width * height));
	CFDataSetLength(data, (CFIndex)(width * height));
	uint8_t *bytes = CFDataGetMutableBytePtr(data);

	for (size_t row = 0; row < height; row++)
		memcpy(bytes + (height - 1 - row) * width, atlas->pixels + ((size_t)glyph->atlas_y + row) * UPL_ATLAS_SIZE + (size_t)glyph->atlas_x, width);

	static const CGFloat decode[2] = { 1.0, 0.0 };
	CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
	CGImageRef mask = CGImageMaskCreate(width, height, 8, 8, width, provider, decode, false);
	CGDataProviderRelease(provider);
	CFRelease(data);
	return mask;
}

#pragma endregion

#pragma region Frame

static void upl__cg_begin(CGContextRef context, float scale, int32_t width, int32_t height)
{
	Upl_CgState *state = &upl__cg_state;
	if (!state->color_space)
		state->color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

	state->context = context;
	state->width = (float)width;
	state->height = (float)height;
	state->clipped = false;

	CGContextSaveGState(context);
	CGContextScaleCTM(context, 1.0 / (CGFloat)scale, 1.0 / (CGFloat)scale);
	CGContextSetShouldAntialias(context, true);
}

static void upl__cg_end(void)
{
	Upl_CgState *state = &upl__cg_state;
	if (state->clipped)
		CGContextRestoreGState(state->context);

	CGContextRestoreGState(state->context);
	state->context = NULL;
	state->clipped = false;
}

#pragma endregion

#pragma region Backend

static void upl__gfx_clear(Leaf_Color color)
{
	Upl_CgState *state = &upl__cg_state;
	if (!state->context)
		return;

	color.a = 255;
	upl__cg_fill_color(color);
	CGContextFillRect(state->context, CGRectMake(0.0, 0.0, state->width, state->height));
}

static void upl__gfx_fill_rect(float x, float y, float width, float height, const float radii[4], Leaf_Color color)
{
	CGContextRef context = upl__cg_state.context;
	if (!context)
		return;

	upl__cg_fill_color(color);
	if (upl__cg_square(radii))
	{
		CGContextFillRect(context, CGRectMake(x, y, width, height));
		return;
	}

	upl__cg_round_rect_path(x, y, width, height, radii);
	CGContextFillPath(context);
}

static void upl__gfx_stroke_rect(float x, float y, float width, float height, const float radii[4], float line_width, Leaf_Color color)
{
	CGContextRef context = upl__cg_state.context;
	if (!context)
		return;

	const float half = line_width * 0.5f;
	float inner[4];
	for (uint32_t i = 0; i < 4; i++)
		inner[i] = fmaxf(radii[i] - half, 0.0f);

	upl__cg_stroke_color(color);
	CGContextSetLineWidth(context, line_width);
	upl__cg_round_rect_path(x + half, y + half, width - line_width, height - line_width, inner);
	CGContextStrokePath(context);
}

static void upl__gfx_shadow(float x, float y, float width, float height, const float radii[4], float blur, Leaf_Color color)
{
	upl__shadow_layers(x, y, width, height, radii, blur, color);
}

static void upl__gfx_line(float x0, float y0, float x1, float y1, float width, Leaf_Color color)
{
	CGContextRef context = upl__cg_state.context;
	if (!context)
		return;

	upl__cg_stroke_color(color);
	CGContextSetLineWidth(context, width);
	CGContextSetLineCap(context, kCGLineCapRound);
	CGContextBeginPath(context);
	CGContextMoveToPoint(context, x0, y0);
	CGContextAddLineToPoint(context, x1, y1);
	CGContextStrokePath(context);
}

static void upl__gfx_fill_circle(float cx, float cy, float radius, Leaf_Color color)
{
	CGContextRef context = upl__cg_state.context;
	if (!context)
		return;

	upl__cg_fill_color(color);
	CGContextFillEllipseInRect(context, CGRectMake(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f));
}

static void upl__gfx_stroke_arc(float cx, float cy, float radius, float start_angle, float sweep, float width, Leaf_Color color)
{
	CGContextRef context = upl__cg_state.context;
	if (!context)
		return;

	upl__cg_stroke_color(color);
	CGContextSetLineWidth(context, width);
	CGContextSetLineCap(context, kCGLineCapRound);
	CGContextBeginPath(context);
	CGContextAddArc(context, cx, cy, radius, start_angle, start_angle + sweep, 0);
	CGContextStrokePath(context);
}

static void upl__gfx_glyph(const Upl_Atlas *atlas, Upl_Glyph *glyph, float x, float y, Leaf_Color color)
{
	CGContextRef context = upl__cg_state.context;
	if (!context)
		return;

	if (!glyph->native)
		glyph->native = (void *)upl__cg_glyph_mask(atlas, glyph);
	if (!glyph->native)
		return;

	upl__cg_fill_color(color);
	CGContextSetInterpolationQuality(context, kCGInterpolationNone);
	CGContextDrawImage(context, CGRectMake(x, y, glyph->width, glyph->height), (CGImageRef)glyph->native);
}

static void upl__gfx_image(Upl_Image *image, float x, float y, float width, float height, const float radii[4], Leaf_Color tint)
{
	CGContextRef context = upl__cg_state.context;
	if (!context)
		return;

	const uint32_t key = upl__color_key(tint);
	if (image->native && image->native_tint != key)
	{
		CGImageRelease((CGImageRef)image->native);
		image->native = NULL;
	}

	if (!image->native)
	{
		uint8_t *pixels = upl__image_premultiply(image, tint, false);
		if (!pixels)
			return;

		image->native = (void *)upl__cg_image_create(pixels, image->width, image->height);
		image->native_tint = key;
		free(pixels);
	}

	if (!image->native)
		return;

	CGContextSaveGState(context);
	if (!upl__cg_square(radii))
	{
		upl__cg_round_rect_path(x, y, width, height, radii);
		CGContextClip(context);
	}

	CGContextSetAlpha(context, (CGFloat)tint.a / 255.0);
	CGContextSetInterpolationQuality(context, kCGInterpolationDefault);
	CGContextTranslateCTM(context, x, y + height);
	CGContextScaleCTM(context, 1.0, -1.0);
	CGContextDrawImage(context, CGRectMake(0.0, 0.0, width, height), (CGImageRef)image->native);
	CGContextRestoreGState(context);
}

static void upl__gfx_set_clip(const Upl_Clip *clip)
{
	Upl_CgState *state = &upl__cg_state;
	if (!state->context)
		return;

	if (state->clipped)
		CGContextRestoreGState(state->context);

	state->clipped = clip != NULL;
	if (!clip)
		return;

	CGContextSaveGState(state->context);
	CGContextClipToRect(state->context, CGRectMake(clip->x0, clip->y0, clip->x1 > clip->x0 ? clip->x1 - clip->x0 : 0, clip->y1 > clip->y0 ? clip->y1 - clip->y0 : 0));
}

static void upl__gfx_atlas_reset(void)
{
}

static void upl__gfx_glyph_release(Upl_Glyph *glyph)
{
	CGImageRelease((CGImageRef)glyph->native);
	glyph->native = NULL;
}

static void upl__gfx_image_release(Upl_Image *image)
{
	if (image->native)
		CGImageRelease((CGImageRef)image->native);

	image->native = NULL;
}

#pragma endregion
