#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <d2d1.h>

#define UPL_D2D_IMAGE_SLOTS		16
#define UPL_D2D_FULL_TURN		6.28318530718f
#define UPL_D2D_RECREATE_TARGET	((HRESULT)0x8899000CL)

typedef struct Upl_D2DUnknown Upl_D2DUnknown;
typedef struct Upl_D2DFactory Upl_D2DFactory;
typedef struct Upl_D2DTarget Upl_D2DTarget;
typedef struct Upl_D2DBrush Upl_D2DBrush;
typedef struct Upl_D2DSolidBrush Upl_D2DSolidBrush;
typedef struct Upl_D2DBitmap Upl_D2DBitmap;
typedef struct Upl_D2DPath Upl_D2DPath;
typedef struct Upl_D2DSink Upl_D2DSink;
typedef struct Upl_D2DStrokeStyle Upl_D2DStrokeStyle;

struct Upl_D2DUnknown
{
	const struct
	{
		void *query_interface;
		void *add_ref;
		ULONG (STDMETHODCALLTYPE *Release)(Upl_D2DUnknown *self);
	}
	*lpVtbl;
};

struct Upl_D2DFactory
{
	const struct
	{
		void *unused_0_9[10];
		HRESULT (STDMETHODCALLTYPE *CreatePathGeometry)(Upl_D2DFactory *self, Upl_D2DPath **path);
		HRESULT (STDMETHODCALLTYPE *CreateStrokeStyle)(Upl_D2DFactory *self, const D2D1_STROKE_STYLE_PROPERTIES *properties, const FLOAT *dashes, UINT32 dash_count, Upl_D2DStrokeStyle **style);
		void *unused_12_13[2];
		HRESULT (STDMETHODCALLTYPE *CreateHwndRenderTarget)(Upl_D2DFactory *self, const D2D1_RENDER_TARGET_PROPERTIES *properties, const D2D1_HWND_RENDER_TARGET_PROPERTIES *hwnd_properties, Upl_D2DTarget **target);
	}
	*lpVtbl;
};

struct Upl_D2DTarget
{
	const struct
	{
		void *unused_0_3[4];
		HRESULT (STDMETHODCALLTYPE *CreateBitmap)(Upl_D2DTarget *self, D2D1_SIZE_U size, const void *data, UINT32 pitch, const D2D1_BITMAP_PROPERTIES *properties, Upl_D2DBitmap **bitmap);
		void *unused_5_6[2];
		HRESULT (STDMETHODCALLTYPE *CreateBitmapBrush)(Upl_D2DTarget *self, Upl_D2DBitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES *properties, const D2D1_BRUSH_PROPERTIES *brush_properties, Upl_D2DBrush **brush);
		HRESULT (STDMETHODCALLTYPE *CreateSolidColorBrush)(Upl_D2DTarget *self, const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *properties, Upl_D2DSolidBrush **brush);
		void *unused_9_14[6];
		void (STDMETHODCALLTYPE *DrawLine)(Upl_D2DTarget *self, D2D1_POINT_2F p0, D2D1_POINT_2F p1, Upl_D2DBrush *brush, FLOAT width, Upl_D2DStrokeStyle *style);
		void (STDMETHODCALLTYPE *DrawRectangle)(Upl_D2DTarget *self, const D2D1_RECT_F *rect, Upl_D2DBrush *brush, FLOAT width, Upl_D2DStrokeStyle *style);
		void (STDMETHODCALLTYPE *FillRectangle)(Upl_D2DTarget *self, const D2D1_RECT_F *rect, Upl_D2DBrush *brush);
		void (STDMETHODCALLTYPE *DrawRoundedRectangle)(Upl_D2DTarget *self, const D2D1_ROUNDED_RECT *rect, Upl_D2DBrush *brush, FLOAT width, Upl_D2DStrokeStyle *style);
		void (STDMETHODCALLTYPE *FillRoundedRectangle)(Upl_D2DTarget *self, const D2D1_ROUNDED_RECT *rect, Upl_D2DBrush *brush);
		void (STDMETHODCALLTYPE *DrawEllipse)(Upl_D2DTarget *self, const D2D1_ELLIPSE *ellipse, Upl_D2DBrush *brush, FLOAT width, Upl_D2DStrokeStyle *style);
		void (STDMETHODCALLTYPE *FillEllipse)(Upl_D2DTarget *self, const D2D1_ELLIPSE *ellipse, Upl_D2DBrush *brush);
		void (STDMETHODCALLTYPE *DrawGeometry)(Upl_D2DTarget *self, Upl_D2DPath *geometry, Upl_D2DBrush *brush, FLOAT width, Upl_D2DStrokeStyle *style);
		void (STDMETHODCALLTYPE *FillGeometry)(Upl_D2DTarget *self, Upl_D2DPath *geometry, Upl_D2DBrush *brush, Upl_D2DBrush *opacity_brush);
		void *unused_24;
		void (STDMETHODCALLTYPE *FillOpacityMask)(Upl_D2DTarget *self, Upl_D2DBitmap *mask, Upl_D2DBrush *brush, D2D1_OPACITY_MASK_CONTENT content, const D2D1_RECT_F *destination, const D2D1_RECT_F *source);
		void *unused_26_31[6];
		void (STDMETHODCALLTYPE *SetAntialiasMode)(Upl_D2DTarget *self, D2D1_ANTIALIAS_MODE mode);
		void *unused_33_41[9];
		HRESULT (STDMETHODCALLTYPE *Flush)(Upl_D2DTarget *self, UINT64 *tag1, UINT64 *tag2);
		void *unused_43_44[2];
		void (STDMETHODCALLTYPE *PushAxisAlignedClip)(Upl_D2DTarget *self, const D2D1_RECT_F *rect, D2D1_ANTIALIAS_MODE mode);
		void (STDMETHODCALLTYPE *PopAxisAlignedClip)(Upl_D2DTarget *self);
		void (STDMETHODCALLTYPE *Clear)(Upl_D2DTarget *self, const D2D1_COLOR_F *color);
		void (STDMETHODCALLTYPE *BeginDraw)(Upl_D2DTarget *self);
		HRESULT (STDMETHODCALLTYPE *EndDraw)(Upl_D2DTarget *self, UINT64 *tag1, UINT64 *tag2);
		void *unused_50_56[7];
		D2D1_WINDOW_STATE (STDMETHODCALLTYPE *CheckWindowState)(Upl_D2DTarget *self);
		HRESULT (STDMETHODCALLTYPE *Resize)(Upl_D2DTarget *self, const D2D1_SIZE_U *size);
	}
	*lpVtbl;
};

struct Upl_D2DBrush
{
	const struct
	{
		void *unused_0_3[4];
		void (STDMETHODCALLTYPE *SetOpacity)(Upl_D2DBrush *self, FLOAT opacity);
		void (STDMETHODCALLTYPE *SetTransform)(Upl_D2DBrush *self, const float *matrix);
	}
	*lpVtbl;
};

struct Upl_D2DSolidBrush
{
	const struct
	{
		void *unused_0_7[8];
		void (STDMETHODCALLTYPE *SetColor)(Upl_D2DSolidBrush *self, const D2D1_COLOR_F *color);
	}
	*lpVtbl;
};

struct Upl_D2DBitmap
{
	const struct
	{
		void *unused_0_9[10];
		HRESULT (STDMETHODCALLTYPE *CopyFromMemory)(Upl_D2DBitmap *self, const void *rect, const void *data, UINT32 pitch);
	}
	*lpVtbl;
};

struct Upl_D2DPath
{
	const struct
	{
		void *unused_0_16[17];
		HRESULT (STDMETHODCALLTYPE *Open)(Upl_D2DPath *self, Upl_D2DSink **sink);
	}
	*lpVtbl;
};

struct Upl_D2DSink
{
	const struct
	{
		void *unused_0_4[5];
		void (STDMETHODCALLTYPE *BeginFigure)(Upl_D2DSink *self, D2D1_POINT_2F start, D2D1_FIGURE_BEGIN begin);
		void *unused_6_7[2];
		void (STDMETHODCALLTYPE *EndFigure)(Upl_D2DSink *self, D2D1_FIGURE_END end);
		HRESULT (STDMETHODCALLTYPE *Close)(Upl_D2DSink *self);
		void (STDMETHODCALLTYPE *AddLine)(Upl_D2DSink *self, D2D1_POINT_2F point);
		void *unused_11_13[3];
		void (STDMETHODCALLTYPE *AddArc)(Upl_D2DSink *self, const D2D1_ARC_SEGMENT *arc);
	}
	*lpVtbl;
};

typedef struct
{
	uint64_t serial;
	uint64_t last_used;
	uint32_t tint;
	Upl_D2DBitmap *bitmap;
	Upl_D2DBrush *brush;
}
Upl_D2DImageSlot;

typedef struct
{
	HWND hwnd;
	Upl_D2DTarget *hwnd_target;
	Upl_D2DSolidBrush *brush;
	Upl_D2DBitmap *atlas;
	uint32_t atlas_generation;
	uint32_t width;
	uint32_t height;
	uint64_t frame;
	Upl_D2DImageSlot images[UPL_D2D_IMAGE_SLOTS];
}
Upl_D2DSurface;

typedef struct
{
	Upl_D2DFactory *factory;
	Upl_D2DStrokeStyle *round_stroke;
	uint32_t surface_count;
	Upl_D2DSurface *current;
	Upl_D2DTarget *target;
	D2D1_ANTIALIAS_MODE antialias;
	bool clipped;
	bool drawn;
}
Upl_D2DState;

static Upl_D2DState upl__d2d_state;

static const GUID upl__d2d_iid_factory = { 0x06152247, 0x6f50, 0x465a, { 0x92, 0x45, 0x11, 0x8b, 0xfd, 0x3b, 0x60, 0x07 } };

#pragma region Helpers

static void upl__d2d_release(void *object)
{
	if (object)
		((Upl_D2DUnknown *)object)->lpVtbl->Release((Upl_D2DUnknown *)object);
}

static D2D1_COLOR_F upl__d2d_color(Leaf_Color color)
{
	return (D2D1_COLOR_F) { (float)color.r / 255.0f, (float)color.g / 255.0f, (float)color.b / 255.0f, (float)color.a / 255.0f };
}

static Upl_D2DBrush *upl__d2d_solid(Leaf_Color color)
{
	Upl_D2DSolidBrush *brush = upl__d2d_state.current->brush;
	const D2D1_COLOR_F value = upl__d2d_color(color);
	brush->lpVtbl->SetColor(brush, &value);
	return (Upl_D2DBrush *)brush;
}

static void upl__d2d_antialias(D2D1_ANTIALIAS_MODE mode)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (state->antialias == mode)
		return;

	state->target->lpVtbl->SetAntialiasMode(state->target, mode);
	state->antialias = mode;
}

static bool upl__d2d_ready(void)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (!state->current)
		return false;

	upl__d2d_antialias(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	state->drawn = true;
	return true;
}

static bool upl__d2d_uniform(const float radii[4])
{
	return radii[0] == radii[1] && radii[1] == radii[2] && radii[2] == radii[3];
}

static Upl_D2DPath *upl__d2d_round_rect_path(float x, float y, float width, float height, const float radii[4])
{
	Upl_D2DFactory *factory = upl__d2d_state.factory;
	Upl_D2DPath *path = NULL;
	Upl_D2DSink *sink = NULL;
	if (FAILED(factory->lpVtbl->CreatePathGeometry(factory, &path)))
		return NULL;

	if (FAILED(path->lpVtbl->Open(path, &sink)))
	{
		upl__d2d_release(path);
		return NULL;
	}

	const D2D1_POINT_2F corners[4][2] = {
		{ { x + width - radii[1], y }, { x + width, y + radii[1] } },
		{ { x + width, y + height - radii[2] }, { x + width - radii[2], y + height } },
		{ { x + radii[3], y + height }, { x, y + height - radii[3] } },
		{ { x, y + radii[0] }, { x + radii[0], y } }
	};
	const float corner_radii[4] = { radii[1], radii[2], radii[3], radii[0] };

		sink->lpVtbl->BeginFigure(sink, (D2D1_POINT_2F) { x + radii[0], y }, D2D1_FIGURE_BEGIN_FILLED);
	for (uint32_t i = 0; i < 4; i++)
	{
		sink->lpVtbl->AddLine(sink, corners[i][0]);
		if (corner_radii[i] <= 0.0f)
			continue;

		const D2D1_ARC_SEGMENT arc = { corners[i][1], { corner_radii[i], corner_radii[i] }, 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL };
		sink->lpVtbl->AddArc(sink, &arc);
	}

	sink->lpVtbl->EndFigure(sink, D2D1_FIGURE_END_CLOSED);
	sink->lpVtbl->Close(sink);
	upl__d2d_release(sink);
	return path;
}

static void upl__d2d_fill_shape(float x, float y, float width, float height, const float radii[4], Upl_D2DBrush *brush)
{
	Upl_D2DTarget *target = upl__d2d_state.target;
	const D2D1_RECT_F rect = { x, y, x + width, y + height };
	if (!upl__d2d_uniform(radii))
	{
		Upl_D2DPath *path = upl__d2d_round_rect_path(x, y, width, height, radii);
		if (path)
			target->lpVtbl->FillGeometry(target, path, brush, NULL);
		upl__d2d_release(path);
	}
	else if (radii[0] > 0.0f)
	{
		const D2D1_ROUNDED_RECT rounded = { rect, radii[0], radii[0] };
		target->lpVtbl->FillRoundedRectangle(target, &rounded, brush);
	}
	else
	{
		target->lpVtbl->FillRectangle(target, &rect, brush);
	}
}

#pragma endregion

#pragma region Surface

static void upl__d2d_release_target(Upl_D2DSurface *surface)
{
	for (uint32_t i = 0; i < UPL_D2D_IMAGE_SLOTS; i++)
	{
		upl__d2d_release(surface->images[i].brush);
		upl__d2d_release(surface->images[i].bitmap);
		surface->images[i] = (Upl_D2DImageSlot) { 0 };
	}

	upl__d2d_release(surface->atlas);
	upl__d2d_release(surface->brush);
	upl__d2d_release(surface->hwnd_target);
	surface->atlas = NULL;
	surface->brush = NULL;
	surface->hwnd_target = NULL;
}

static bool upl__d2d_create_target(Upl_D2DSurface *surface)
{
	Upl_D2DFactory *factory = upl__d2d_state.factory;
	const D2D1_RENDER_TARGET_PROPERTIES properties = {
		.type = D2D1_RENDER_TARGET_TYPE_DEFAULT,
		.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
		.dpiX = 96.0f,
		.dpiY = 96.0f,
		.usage = D2D1_RENDER_TARGET_USAGE_NONE,
		.minLevel = D2D1_FEATURE_LEVEL_DEFAULT
	};
	const D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_properties = {
		.hwnd = surface->hwnd,
		.pixelSize = { surface->width, surface->height },
		.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY
	};

	if (FAILED(factory->lpVtbl->CreateHwndRenderTarget(factory, &properties, &hwnd_properties, &surface->hwnd_target)))
		return false;

	Upl_D2DTarget *target = surface->hwnd_target;
	const D2D1_COLOR_F white = { 1.0f, 1.0f, 1.0f, 1.0f };
	const D2D1_BITMAP_PROPERTIES atlas_properties = { { DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, 96.0f, 96.0f };
	const D2D1_SIZE_U atlas_size = { UPL_ATLAS_SIZE, UPL_ATLAS_SIZE };

	if (FAILED(target->lpVtbl->CreateSolidColorBrush(target, &white, NULL, &surface->brush)) ||
		FAILED(target->lpVtbl->CreateBitmap(target, atlas_size, NULL, 0, &atlas_properties, &surface->atlas)))
	{
		upl__d2d_release_target(surface);
		return false;
	}

	surface->atlas_generation = UINT32_MAX;
	return true;
}

static Upl_D2DSurface *upl__d2d_surface_create(HWND hwnd, uint32_t width, uint32_t height)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (!state->factory)
	{
		if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &upl__d2d_iid_factory, NULL, (void **)&state->factory)))
			return NULL;

		const D2D1_STROKE_STYLE_PROPERTIES stroke = { D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f };
		state->factory->lpVtbl->CreateStrokeStyle(state->factory, &stroke, NULL, 0, &state->round_stroke);
	}

	Upl_D2DSurface *surface = (Upl_D2DSurface *)calloc(1, sizeof(Upl_D2DSurface));
	surface->hwnd = hwnd;
	surface->width = width;
	surface->height = height;
	state->surface_count++;
	return surface;
}

static void upl__d2d_surface_destroy(Upl_D2DSurface *surface)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (!surface)
		return;

	upl__d2d_release_target(surface);
	free(surface);

	if (--state->surface_count > 0)
		return;

	upl__d2d_release(state->round_stroke);
	upl__d2d_release(state->factory);
	state->round_stroke = NULL;
	state->factory = NULL;
}

static void upl__d2d_surface_resize(Upl_D2DSurface *surface, uint32_t width, uint32_t height)
{
	surface->width = width;
	surface->height = height;
	if (!surface->hwnd_target)
		return;

	const D2D1_SIZE_U size = { width, height };
	surface->hwnd_target->lpVtbl->Resize(surface->hwnd_target, &size);
}

static bool upl__d2d_begin(Upl_D2DSurface *surface)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (!surface->hwnd_target && !upl__d2d_create_target(surface))
		return false;

	if (surface->hwnd_target->lpVtbl->CheckWindowState(surface->hwnd_target) & D2D1_WINDOW_STATE_OCCLUDED)
		return false;

	state->current = surface;
	state->target = surface->hwnd_target;
	state->antialias = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
	state->clipped = false;
	state->drawn = false;
	surface->frame++;

	state->target->lpVtbl->BeginDraw(state->target);
	state->target->lpVtbl->SetAntialiasMode(state->target, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	return true;
}

static void upl__d2d_end(Upl_D2DSurface *surface)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (state->clipped)
		state->target->lpVtbl->PopAxisAlignedClip(state->target);

	const HRESULT result = state->target->lpVtbl->EndDraw(state->target, NULL, NULL);
	state->current = NULL;
	state->target = NULL;
	state->clipped = false;

	if (result == UPL_D2D_RECREATE_TARGET)
		upl__d2d_release_target(surface);
}

static Upl_D2DImageSlot *upl__d2d_image_slot(Upl_D2DSurface *surface, Upl_Image *image, Leaf_Color tint)
{
	const uint32_t key = upl__color_key(tint);
	Upl_D2DImageSlot *slot = &surface->images[0];
	for (uint32_t i = 0; i < UPL_D2D_IMAGE_SLOTS; i++)
	{
		Upl_D2DImageSlot *candidate = &surface->images[i];
		if (candidate->serial == image->serial && candidate->tint == key && candidate->brush)
		{
			candidate->last_used = surface->frame;
			return candidate;
		}

		if (candidate->last_used < slot->last_used)
			slot = candidate;
	}

	upl__d2d_release(slot->brush);
	upl__d2d_release(slot->bitmap);
	*slot = (Upl_D2DImageSlot) { 0 };

	uint8_t *pixels = upl__image_premultiply(image, tint, true);
	if (!pixels)
		return NULL;

	Upl_D2DTarget *target = upl__d2d_state.target;
	const D2D1_BITMAP_PROPERTIES properties = { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, 96.0f, 96.0f };
	const D2D1_SIZE_U size = { (UINT32)image->width, (UINT32)image->height };
	const D2D1_BITMAP_BRUSH_PROPERTIES brush_properties = { D2D1_EXTEND_MODE_CLAMP, D2D1_EXTEND_MODE_CLAMP, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR };
	const HRESULT result = target->lpVtbl->CreateBitmap(target, size, pixels, (UINT32)image->width * 4, &properties, &slot->bitmap);
	free(pixels);

	if (FAILED(result) || FAILED(target->lpVtbl->CreateBitmapBrush(target, slot->bitmap, &brush_properties, NULL, &slot->brush)))
	{
		upl__d2d_release(slot->bitmap);
		*slot = (Upl_D2DImageSlot) { 0 };
		return NULL;
	}

	slot->serial = image->serial;
	slot->tint = key;
	slot->last_used = surface->frame;
	return slot;
}

#pragma endregion

#pragma region Backend

static void upl__gfx_clear(Leaf_Color color)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (!state->current)
		return;

	const D2D1_COLOR_F value = upl__d2d_color(color);
	state->target->lpVtbl->Clear(state->target, &value);
}

static void upl__gfx_fill_rect(float x, float y, float width, float height, const float radii[4], Leaf_Color color)
{
	if (upl__d2d_ready())
		upl__d2d_fill_shape(x, y, width, height, radii, upl__d2d_solid(color));
}

static void upl__gfx_stroke_rect(float x, float y, float width, float height, const float radii[4], float line_width, Leaf_Color color)
{
	if (!upl__d2d_ready())
		return;

	Upl_D2DTarget *target = upl__d2d_state.target;
	Upl_D2DBrush *brush = upl__d2d_solid(color);
	const float half = line_width * 0.5f;
	const D2D1_RECT_F rect = { x + half, y + half, x + width - half, y + height - half };
	float inner[4];
	for (uint32_t i = 0; i < 4; i++)
		inner[i] = fmaxf(radii[i] - half, 0.0f);

	if (!upl__d2d_uniform(inner))
	{
		Upl_D2DPath *path = upl__d2d_round_rect_path(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, inner);
		if (path)
			target->lpVtbl->DrawGeometry(target, path, brush, line_width, NULL);
		upl__d2d_release(path);
	}
	else if (inner[0] > 0.0f)
	{
		const D2D1_ROUNDED_RECT rounded = { rect, inner[0], inner[0] };
		target->lpVtbl->DrawRoundedRectangle(target, &rounded, brush, line_width, NULL);
	}
	else
	{
		target->lpVtbl->DrawRectangle(target, &rect, brush, line_width, NULL);
	}
}

static void upl__gfx_shadow(float x, float y, float width, float height, const float radii[4], float blur, Leaf_Color color)
{
	upl__shadow_layers(x, y, width, height, radii, blur, color);
}

static void upl__gfx_line(float x0, float y0, float x1, float y1, float width, Leaf_Color color)
{
	if (!upl__d2d_ready())
		return;

	Upl_D2DTarget *target = upl__d2d_state.target;
	target->lpVtbl->DrawLine(target, (D2D1_POINT_2F) { x0, y0 }, (D2D1_POINT_2F) { x1, y1 }, upl__d2d_solid(color), width, upl__d2d_state.round_stroke);
}

static void upl__gfx_fill_circle(float cx, float cy, float radius, Leaf_Color color)
{
	if (!upl__d2d_ready())
		return;

	Upl_D2DTarget *target = upl__d2d_state.target;
	const D2D1_ELLIPSE ellipse = { { cx, cy }, radius, radius };
	target->lpVtbl->FillEllipse(target, &ellipse, upl__d2d_solid(color));
}

static void upl__gfx_stroke_arc(float cx, float cy, float radius, float start_angle, float sweep, float width, Leaf_Color color)
{
	if (!upl__d2d_ready())
		return;

	Upl_D2DState *state = &upl__d2d_state;
	Upl_D2DTarget *target = state->target;
	Upl_D2DBrush *brush = upl__d2d_solid(color);
	if (sweep >= UPL_D2D_FULL_TURN - 1e-4f)
	{
		const D2D1_ELLIPSE ellipse = { { cx, cy }, radius, radius };
		target->lpVtbl->DrawEllipse(target, &ellipse, brush, width, NULL);
		return;
	}

	Upl_D2DPath *path = NULL;
	Upl_D2DSink *sink = NULL;
	if (FAILED(state->factory->lpVtbl->CreatePathGeometry(state->factory, &path)))
		return;

	if (SUCCEEDED(path->lpVtbl->Open(path, &sink)))
	{
		const float end_angle = start_angle + sweep;
		const D2D1_ARC_SEGMENT arc = {
			{ cx + cosf(end_angle) * radius, cy + sinf(end_angle) * radius },
			{ radius, radius },
			0.0f,
			D2D1_SWEEP_DIRECTION_CLOCKWISE,
			sweep > UPL_D2D_FULL_TURN * 0.5f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
		};

				sink->lpVtbl->BeginFigure(sink, (D2D1_POINT_2F) { cx + cosf(start_angle) * radius, cy + sinf(start_angle) * radius }, D2D1_FIGURE_BEGIN_HOLLOW);
		sink->lpVtbl->AddArc(sink, &arc);
		sink->lpVtbl->EndFigure(sink, D2D1_FIGURE_END_OPEN);
		sink->lpVtbl->Close(sink);
		upl__d2d_release(sink);
		target->lpVtbl->DrawGeometry(target, path, brush, width, state->round_stroke);
	}

	upl__d2d_release(path);
}

static void upl__gfx_glyph(const Upl_Atlas *atlas, Upl_Glyph *glyph, float x, float y, Leaf_Color color)
{
	Upl_D2DState *state = &upl__d2d_state;
	Upl_D2DSurface *surface = state->current;
	if (!surface)
		return;

	Upl_D2DTarget *target = state->target;
	if (surface->atlas_generation != atlas->generation)
	{
		if (state->drawn)
			target->lpVtbl->Flush(target, NULL, NULL);

		surface->atlas->lpVtbl->CopyFromMemory(surface->atlas, NULL, atlas->pixels, UPL_ATLAS_SIZE);
		surface->atlas_generation = atlas->generation;
	}

	upl__d2d_antialias(D2D1_ANTIALIAS_MODE_ALIASED);
	state->drawn = true;

	const D2D1_RECT_F destination = { x, y, x + (float)glyph->width, y + (float)glyph->height };
	const D2D1_RECT_F source = {
		(float)glyph->atlas_x, (float)glyph->atlas_y,
		(float)(glyph->atlas_x + glyph->width), (float)(glyph->atlas_y + glyph->height)
	};
	target->lpVtbl->FillOpacityMask(target, surface->atlas, upl__d2d_solid(color), D2D1_OPACITY_MASK_CONTENT_GRAPHICS, &destination, &source);
}

static void upl__gfx_image(Upl_Image *image, float x, float y, float width, float height, const float radii[4], Leaf_Color tint)
{
	if (!upl__d2d_ready())
		return;

	Upl_D2DImageSlot *slot = upl__d2d_image_slot(upl__d2d_state.current, image, tint);
	if (!slot)
		return;

	const float transform[6] = { width / (float)image->width, 0.0f, 0.0f, height / (float)image->height, x, y };

	Upl_D2DBrush *brush = slot->brush;
	brush->lpVtbl->SetTransform(brush, transform);
	brush->lpVtbl->SetOpacity(brush, (float)tint.a / 255.0f);
	upl__d2d_fill_shape(x, y, width, height, radii, brush);
}

static void upl__gfx_set_clip(const Upl_Clip *clip)
{
	Upl_D2DState *state = &upl__d2d_state;
	if (!state->current)
		return;

	if (state->clipped)
		state->target->lpVtbl->PopAxisAlignedClip(state->target);

	state->clipped = clip != NULL;
	if (!clip)
		return;

	const D2D1_RECT_F rect = { (float)clip->x0, (float)clip->y0, (float)(clip->x1 > clip->x0 ? clip->x1 : clip->x0), (float)(clip->y1 > clip->y0 ? clip->y1 : clip->y0) };
	state->target->lpVtbl->PushAxisAlignedClip(state->target, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
}

static void upl__gfx_atlas_reset(void)
{
}

static void upl__gfx_glyph_release(Upl_Glyph *glyph)
{
	glyph->native = NULL;
}

static void upl__gfx_image_release(Upl_Image *image)
{
	image->native = NULL;
}

#pragma endregion
