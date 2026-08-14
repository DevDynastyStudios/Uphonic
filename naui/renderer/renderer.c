#define NAUI_RENDERER_MAX_VERTICES (1 << 18)
#define NAUI_RENDERER_MAX_INDICES (NAUI_RENDERER_MAX_VERTICES * 6 / 4)
#define NAUI_RENDERER_CORNER_SEGMENTS 8

#define NAUI_FONT_MAX_SLOTS         4
#define NAUI_FONT_MAX_RANGES        64

#define NAUI_FONT_BAKE_SIZE         32.0f

#define NAUI_CLIP_STACK_MAX 1024

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const struct { int start; int count; } k_unicode_blocks[] =
{
    { 0x0020,   96 },
    { 0x00A0,   96 },
    { 0x0100,  128 },
    { 0x0370,  144 },
    { 0x0400,  128 },
    { 0x3000,   64 },
    { 0x3040,   96 },
    { 0x30A0,   96 },
    { 0x3400, 6592 },
    { 0x4E00,20902 },
    { 0xF900,  512 },
    { 0xFF00,   94 },
};
#define K_NUM_UNICODE_BLOCKS (int)(sizeof(k_unicode_blocks)/sizeof(k_unicode_blocks[0]))

static const int k_atlas_size_candidates[] = { 512, 1024, 2048, 4096 };
#define K_NUM_ATLAS_CANDIDATES (int)(sizeof(k_atlas_size_candidates)/sizeof(k_atlas_size_candidates[0]))

static int naui_choose_initial_atlas(int glyph_count)
{
    if (glyph_count <=  256) return 512;
    if (glyph_count <= 1024) return 1024;
    if (glyph_count <= 8192) return 2048;
    return 4096;
}

typedef struct
{
    Naui_Vec2 position;
    Naui_Vec2 uv;
    uint32_t color;
    int32_t texture_id;
}
Naui_BatchVertex;

typedef struct
{
    float x0, y0, x1, y1;
}
Naui_ClipRect;

typedef struct
{
    int               first_codepoint;
    int               num_chars;
    stbtt_packedchar *chars;
}
Naui_FontRange;

typedef struct
{
    Naui_FontRange *ranges;
    int             range_count;
    mgfx_image      atlas;
    int             atlas_size;
    float           ascent;
    float           descent;
    float           baked_size;
    bool            loaded;
}
Naui_FontBake;

typedef struct
{
    Naui_BatchVertex vertices[NAUI_RENDERER_MAX_VERTICES];
    uint32_t vertex_count;
    uint32_t vertex_offset;

    uint32_t indices[NAUI_RENDERER_MAX_INDICES];
    uint32_t index_count;
    uint32_t index_offset;

    mgfx_pipeline base_pipeline;
    mgfx_buffer batch_vb, batch_ib;

    mgfx_sampler linear_sampler;
    mgfx_sampler nearest_sampler;

    mgfx_image image_atlas;

    Naui_FontBake font[NAUI_FONT_MAX_SLOTS];
    bool          font_loaded[NAUI_FONT_MAX_SLOTS];
    int8_t        font_current_index;

    Naui_ClipRect clip_stack[NAUI_CLIP_STACK_MAX];
    int32_t       clip_stack_depth;

    int32_t width, height;
}
Naui_RendererData;

static Naui_RendererData *rdata;
static float s_cos[NAUI_RENDERER_CORNER_SEGMENTS + 1];
static float s_sin[NAUI_RENDERER_CORNER_SEGMENTS + 1];

static inline float naui_min(float a, float b)
{
    return a < b ? a : b;
}

static inline uint32_t naui_pack_color(Naui_Color c)
{
    return ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.r;
}

static inline Naui_Color naui_lerp_color(Naui_Color a, Naui_Color b, float t)
{
    return (Naui_Color){
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t),
        (uint8_t)(a.a + (b.a - a.a) * t),
    };
}

static const char *naui_utf8_decode(const char *s, int *cp_out)
{
    unsigned char c = (unsigned char)*s++;

    if (c < 0x80)
    {
        *cp_out = c;
    }
    else if (c < 0xC0)
    {
        *cp_out = c;
    }
    else if (c < 0xE0)
    {
        *cp_out = (c & 0x1F) << 6;
        if ((*s & 0xC0) == 0x80) *cp_out |= (*s++ & 0x3F);
    }
    else if (c < 0xF0)
    {
        *cp_out = (c & 0x0F) << 12;
        if ((*s   & 0xC0) == 0x80) *cp_out |= (*s++   & 0x3F) << 6;
        if ((*s   & 0xC0) == 0x80) *cp_out |= (*s++   & 0x3F);
    }
    else
    {
        *cp_out = (c & 0x07) << 18;
        if ((*s   & 0xC0) == 0x80) *cp_out |= (*s++   & 0x3F) << 12;
        if ((*s   & 0xC0) == 0x80) *cp_out |= (*s++   & 0x3F) << 6;
        if ((*s   & 0xC0) == 0x80) *cp_out |= (*s++   & 0x3F);
    }

    return s;
}

static const stbtt_packedchar *naui_bake_lookup(const Naui_FontBake *bake, int cp)
{
    for (int i = 0; i < bake->range_count; i++)
    {
        const Naui_FontRange *r = &bake->ranges[i];
        if (cp >= r->first_codepoint && cp < r->first_codepoint + r->num_chars)
            return &r->chars[cp - r->first_codepoint];
    }
    return NULL;
}

typedef struct
{
    float dx, dy, mn, range;
}
Naui_GradientAxis;

static Naui_GradientAxis naui_gradient_axis(Naui_Vec2 tl, Naui_Vec2 br, float angle_deg)
{
    float rad = angle_deg;
    float dx = cosf(rad), dy = sinf(rad);

    float pts[4][2] = {
        { tl.x, tl.y },
        { br.x, tl.y },
        { br.x, br.y },
        { tl.x, br.y },
    };

    float mn = 1e30f, mx = -1e30f;
    for (int i = 0; i < 4; i++)
    {
        float d = pts[i][0] * dx + pts[i][1] * dy;
        if (d < mn) mn = d;
        if (d > mx) mx = d;
    }

    float range = mx - mn;
    if (range < 1e-6f) range = 1e-6f;

    return (Naui_GradientAxis){ dx, dy, mn, range };
}

static inline float naui_gradient_axis_t(const Naui_GradientAxis *axis, float px, float py)
{
    float t = (px * axis->dx + py * axis->dy - axis->mn) / axis->range;
    return naui_clamp01(t);
}

static inline uint32_t naui_gradient_color_at(const Naui_Gradient *g, const Naui_GradientAxis *axis, float px, float py)
{
    float t = naui_gradient_axis_t(axis, px, py);

    float p1 = g->percent1;
    float p2 = g->percent2;

    if (p2 <= p1)
        t = (t < p1) ? 0.0f : 1.0f;
    else
    {
        t = (t - p1) / (p2 - p1);
        t = naui_clamp01(t);
    }

    return naui_pack_color(naui_lerp_color(g->color1, g->color2, t));
}

static inline Naui_ClipRect naui_current_clip(void)
{
    if (rdata->clip_stack_depth == 0)
        return (Naui_ClipRect){ 0, 0, (float)rdata->width, (float)rdata->height };
    return rdata->clip_stack[rdata->clip_stack_depth - 1];
}

static void naui_apply_scissor(void)
{
    Naui_ClipRect c = naui_current_clip();
    mgfx_scissor((int32_t)c.x0, (int32_t)c.y0, (uint32_t)(c.x1 - c.x0), (uint32_t)(c.y1 - c.y0));
}

static void naui_renderer_flush(void)
{
    if (rdata->index_count == rdata->index_offset) return;

    uint32_t vcount  = rdata->vertex_count - rdata->vertex_offset;
    uint32_t icount  = rdata->index_count - rdata->index_offset;
    size_t   vb_off  = rdata->vertex_offset * sizeof(Naui_BatchVertex);
    size_t   ib_off  = rdata->index_offset * sizeof(uint32_t);

    mgfx_update_buffer(rdata->batch_vb, vb_off, vcount * sizeof(Naui_BatchVertex), rdata->vertices + rdata->vertex_offset);
    mgfx_update_buffer(rdata->batch_ib, ib_off, icount * sizeof(uint32_t), rdata->indices + rdata->index_offset);

    mgfx_draw_indexed(icount, rdata->index_offset, 0);

    rdata->vertex_offset = rdata->vertex_count;
    rdata->index_offset = rdata->index_count;
}

static inline bool naui_reserve(uint32_t vert_needed, uint32_t idx_needed)
{
    if (rdata->vertex_count + vert_needed > NAUI_RENDERER_MAX_VERTICES) return false;
    if (rdata->index_count  + idx_needed  > NAUI_RENDERER_MAX_INDICES)  return false;
    return true;
}

static void naui_push_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d, uint32_t color, int32_t texture_id)
{
    if (!naui_reserve(4, 6))
        return;

    uint32_t base = rdata->vertex_count;
    Naui_BatchVertex *v = &rdata->vertices[base];

    v[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, color, texture_id };
    v[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, color, texture_id };
    v[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, color, texture_id };
    v[3] = (Naui_BatchVertex){ {d.x, d.y}, {0, 0}, color, texture_id };
    rdata->vertex_count += 4;

    uint32_t *idx = &rdata->indices[rdata->index_count];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;
    rdata->index_count += 6;
}

static void naui_push_gradient_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d,
    uint32_t ca, uint32_t cb, uint32_t cc, uint32_t cd, int32_t texture_id)
{
    if (!naui_reserve(4, 6))
        return;

    uint32_t base = rdata->vertex_count;
    Naui_BatchVertex *v = &rdata->vertices[base];

    v[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, ca, texture_id };
    v[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, cb, texture_id };
    v[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, cc, texture_id };
    v[3] = (Naui_BatchVertex){ {d.x, d.y}, {0, 0}, cd, texture_id };
    rdata->vertex_count += 4;

    uint32_t *idx = &rdata->indices[rdata->index_count];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;
    rdata->index_count += 6;
}

static void naui_push_textured_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d, Naui_Vec4 texture_area, uint32_t color, int32_t texture_id)
{
    if (!naui_reserve(4, 6))
        return;

    uint32_t base = rdata->vertex_count;
    Naui_BatchVertex *v = &rdata->vertices[base];

    v[0] = (Naui_BatchVertex){ {a.x, a.y}, {texture_area.x,                  texture_area.y},                  color, texture_id };
    v[1] = (Naui_BatchVertex){ {b.x, b.y}, {texture_area.x + texture_area.z, texture_area.y},                  color, texture_id };
    v[2] = (Naui_BatchVertex){ {c.x, c.y}, {texture_area.x + texture_area.z, texture_area.y + texture_area.w}, color, texture_id };
    v[3] = (Naui_BatchVertex){ {d.x, d.y}, {texture_area.x,                  texture_area.y + texture_area.w}, color, texture_id };
    rdata->vertex_count += 4;

    uint32_t *idx = &rdata->indices[rdata->index_count];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;
    rdata->index_count += 6;
}

static void naui_push_rect(Naui_Vec2 tl, Naui_Vec2 br, uint32_t color, int32_t texture_id)
{
    naui_push_quad4(
        (Naui_Vec2){tl.x, tl.y},
        (Naui_Vec2){br.x, tl.y},
        (Naui_Vec2){br.x, br.y},
        (Naui_Vec2){tl.x, br.y},
        color, texture_id
    );
}

static void naui_push_textured_rect(Naui_Vec2 tl, Naui_Vec2 br, Naui_Vec4 uv, uint32_t color, int32_t texture_id)
{
    naui_push_textured_quad4(
        (Naui_Vec2){tl.x, tl.y},
        (Naui_Vec2){br.x, tl.y},
        (Naui_Vec2){br.x, br.y},
        (Naui_Vec2){tl.x, br.y},
        uv,
        color, texture_id
    );
}

static void naui_push_triangle(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, uint32_t color)
{
    if (!naui_reserve(3, 3))
        return;

    uint32_t base = rdata->vertex_count;
    Naui_BatchVertex *v = &rdata->vertices[base];

    v[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, color, -1 };
    v[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, color, -1 };
    v[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, color, -1 };
    rdata->vertex_count += 3;

    uint32_t *idx = &rdata->indices[rdata->index_count];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    rdata->index_count += 3;
}

static void naui_push_gradient_triangle(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c,
    uint32_t ca, uint32_t cb, uint32_t cc)
{
    if (!naui_reserve(3, 3))
        return;

    uint32_t base = rdata->vertex_count;
    Naui_BatchVertex *v = &rdata->vertices[base];

    v[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, ca, -1 };
    v[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, cb, -1 };
    v[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, cc, -1 };
    rdata->vertex_count += 3;

    uint32_t *idx = &rdata->indices[rdata->index_count];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    rdata->index_count += 3;
}

#define NAUI_GRADIENT_CLIP_MAX_VERTS 8

typedef struct
{
    Naui_Vec2 p[NAUI_GRADIENT_CLIP_MAX_VERTS];
    int count;
}
Naui_GradientPoly;

static Naui_GradientPoly naui_clip_poly_by_t(const Naui_GradientPoly *in, const Naui_GradientAxis *axis, float line_t, int keep_ge)
{
    Naui_GradientPoly out = { .count = 0 };
    if (in->count == 0) return out;

    for (int i = 0; i < in->count; i++)
    {
        Naui_Vec2 cur  = in->p[i];
        Naui_Vec2 next = in->p[(i + 1) % in->count];

        float t_cur  = naui_gradient_axis_t(axis, cur.x, cur.y);
        float t_next = naui_gradient_axis_t(axis, next.x, next.y);

        int cur_in  = keep_ge ? (t_cur  >= line_t) : (t_cur  <= line_t);
        int next_in = keep_ge ? (t_next >= line_t) : (t_next <= line_t);

        if (cur_in && out.count < NAUI_GRADIENT_CLIP_MAX_VERTS)
            out.p[out.count++] = cur;

        if (cur_in != next_in)
        {
            float denom = t_next - t_cur;
            float f = (fabsf(denom) < 1e-8f) ? 0.0f : (line_t - t_cur) / denom;
            f = naui_clamp01(f);

            Naui_Vec2 ix = { cur.x + (next.x - cur.x) * f, cur.y + (next.y - cur.y) * f };
            if (out.count < NAUI_GRADIENT_CLIP_MAX_VERTS)
                out.p[out.count++] = ix;
        }
    }

    return out;
}

static void naui_push_gradient_polygon(const Naui_GradientPoly *poly, const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    if (poly->count < 3) return;

    Naui_Vec2 p0 = poly->p[0];
    uint32_t  c0 = naui_gradient_color_at(g, axis, p0.x, p0.y);

    for (int i = 1; i + 1 < poly->count; i++)
    {
        Naui_Vec2 p1 = poly->p[i];
        Naui_Vec2 p2 = poly->p[i + 1];
        naui_push_gradient_triangle(
            p0, p1, p2,
            c0,
            naui_gradient_color_at(g, axis, p1.x, p1.y),
            naui_gradient_color_at(g, axis, p2.x, p2.y)
        );
    }
}

static void naui_push_flat_polygon(const Naui_GradientPoly *poly, uint32_t color)
{
    if (poly->count < 3) return;

    Naui_Vec2 p0 = poly->p[0];
    for (int i = 1; i + 1 < poly->count; i++)
        naui_push_gradient_triangle(p0, poly->p[i], poly->p[i + 1], color, color, color);
}

static void naui_push_gradient_convex(const Naui_Vec2 *verts, int vert_count, const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    if (vert_count < 3 || vert_count > NAUI_GRADIENT_CLIP_MAX_VERTS) return;

    Naui_GradientPoly src = { .count = vert_count };
    for (int i = 0; i < vert_count; i++) src.p[i] = verts[i];

    float p1 = naui_clamp01(g->percent1);
    float p2 = naui_clamp01(g->percent2);

    uint32_t c1 = naui_pack_color(g->color1);
    uint32_t c2 = naui_pack_color(g->color2);

    if (p2 <= p1)
    {
        Naui_GradientPoly before = naui_clip_poly_by_t(&src, axis, p1, 0);
        Naui_GradientPoly after  = naui_clip_poly_by_t(&src, axis, p1, 1);
        naui_push_flat_polygon(&before, c1);
        naui_push_flat_polygon(&after,  c2);
        return;
    }

    Naui_GradientPoly flat_lo = naui_clip_poly_by_t(&src, axis, p1, 0);
    Naui_GradientPoly rest    = naui_clip_poly_by_t(&src, axis, p1, 1);
    Naui_GradientPoly ramp    = naui_clip_poly_by_t(&rest, axis, p2, 0);
    Naui_GradientPoly flat_hi = naui_clip_poly_by_t(&rest, axis, p2, 1);

    naui_push_flat_polygon(&flat_lo, c1);
    naui_push_gradient_polygon(&ramp, g, axis);
    naui_push_flat_polygon(&flat_hi, c2);
}

static void naui_push_gradient_rect(Naui_Vec2 tl, Naui_Vec2 br, const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    Naui_Vec2 verts[4] = {
        { tl.x, tl.y },
        { br.x, tl.y },
        { br.x, br.y },
        { tl.x, br.y },
    };
    naui_push_gradient_convex(verts, 4, g, axis);
}

static void naui_push_corner_fan(Naui_Vec2 center, float r, float ax, float ay, uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        Naui_Vec2 p0 = { center.x + ax * s_cos[s]     * r, center.y + ay * s_sin[s]     * r };
        Naui_Vec2 p1 = { center.x + ax * s_cos[s + 1] * r, center.y + ay * s_sin[s + 1] * r };
        naui_push_triangle(center, p0, p1, color);
    }
}

static void naui_push_gradient_corner_fan(Naui_Vec2 center, float r, float ax, float ay,
    const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        Naui_Vec2 p0 = { center.x + ax * s_cos[s]     * r, center.y + ay * s_sin[s]     * r };
        Naui_Vec2 p1 = { center.x + ax * s_cos[s + 1] * r, center.y + ay * s_sin[s + 1] * r };
        Naui_Vec2 tri[3] = { center, p0, p1 };
        naui_push_gradient_convex(tri, 3, g, axis);
    }
}

static void naui_push_corner_ring(Naui_Vec2 center, float inner_r, float outer_r, float ax, float ay, uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx = s_cos[s],     cy = s_sin[s];
        float nx = s_cos[s + 1], ny = s_sin[s + 1];

        Naui_Vec2 o0 = { center.x + ax * cx * outer_r, center.y + ay * cy * outer_r };
        Naui_Vec2 o1 = { center.x + ax * nx * outer_r, center.y + ay * ny * outer_r };
        Naui_Vec2 i0 = { center.x + ax * cx * inner_r, center.y + ay * cy * inner_r };
        Naui_Vec2 i1 = { center.x + ax * nx * inner_r, center.y + ay * ny * inner_r };

        naui_push_quad4(o0, o1, i1, i0, color, -1);
    }
}

void naui_renderer_build_atlas(uint32_t width, uint32_t height, void *data)
{
    rdata->image_atlas = mgfx_create_image(&(mgfx_image_create_info){
        .type = MGFX_IMAGE_TYPE_2D,
        .format = MGFX_FORMAT_RGBA8_UNORM,
        .usage = MGFX_IMAGE_USAGE_COLOR_ATTACHMENT,
        .width = width,
        .height = height,
        .data = data
    });
}

void naui_renderer_initialize(void)
{
    rdata = calloc(1, sizeof(Naui_RendererData));

    mgfx_init(&(mgfx_init_info){
        .primary_handle = mg_app_primary_handle(),
        .secondary_handle = mg_app_secondary_handle(),
        .width = mg_app_width(),
        .height = mg_app_height(),
        .vsync = true
    });

    for (int i = 0; i <= NAUI_RENDERER_CORNER_SEGMENTS; i++)
    {
        float a = (float)(M_PI * 0.5) * i / NAUI_RENDERER_CORNER_SEGMENTS;
        s_cos[i] = cosf(a);
        s_sin[i] = sinf(a);
    }

    rdata->batch_vb = mgfx_create_buffer(&(mgfx_buffer_create_info){
        .usage  = MGFX_BUFFER_USAGE_VERTEX,
        .memory = MGFX_MEMORY_SHARED,
        .size   = sizeof(rdata->vertices)
    });

    rdata->batch_ib = mgfx_create_buffer(&(mgfx_buffer_create_info){
        .usage  = MGFX_BUFFER_USAGE_INDEX,
        .memory = MGFX_MEMORY_SHARED,
        .size   = sizeof(rdata->indices)
    });

    rdata->base_pipeline = mgfx_create_pipeline(&(mgfx_pipeline_create_info){
        .shader = get_base_shader(mgfx_get_shader_lang()),
        .vertex_attributes = {
            MGFX_VERTEX_FORMAT_FLOAT2,
            MGFX_VERTEX_FORMAT_FLOAT2,
            MGFX_VERTEX_FORMAT_UBYTE4N,
            MGFX_VERTEX_FORMAT_INT
        },
        .color_blend = {
            .blend_enabled = true,
            .src_color_blend_factor = MGFX_BLEND_FACTOR_SRC_ALPHA,
            .dst_color_blend_factor = MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = MGFX_BLEND_OP_ADD,
            .src_alpha_blend_factor = MGFX_BLEND_FACTOR_ONE,
            .dst_alpha_blend_factor = MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = MGFX_BLEND_OP_ADD
        }
    });

    rdata->linear_sampler = mgfx_create_sampler(&(mgfx_sampler_create_info){
        .min_filter     = MGFX_SAMPLER_FILTER_LINEAR,
        .mag_filter     = MGFX_SAMPLER_FILTER_LINEAR,
        .address_mode_u = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    });

    rdata->nearest_sampler = mgfx_create_sampler(&(mgfx_sampler_create_info){
        .min_filter     = MGFX_SAMPLER_FILTER_NEAREST,
        .mag_filter     = MGFX_SAMPLER_FILTER_NEAREST,
        .address_mode_u = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    });

    rdata->width = mg_app_width();
    rdata->height = mg_app_height();
}

static void naui_free_bake(Naui_FontBake *bake)
{
    if (!bake->loaded) return;

    for (int i = 0; i < bake->range_count; i++)
        free(bake->ranges[i].chars);
    free(bake->ranges);
    bake->ranges = NULL;
    bake->range_count = 0;

    mgfx_destroy_image(bake->atlas);
    bake->loaded = false;
}

void naui_renderer_shutdown(void)
{
    for (int i = 0; i < NAUI_FONT_MAX_SLOTS; i++)
    {
        if (rdata->font_loaded[i])
            naui_free_bake(&rdata->font[i]);
    }
    mgfx_destroy_sampler(rdata->linear_sampler);
    mgfx_destroy_sampler(rdata->nearest_sampler);
    mgfx_destroy_buffer(rdata->batch_vb);
    mgfx_destroy_buffer(rdata->batch_ib);
    mgfx_destroy_pipeline(rdata->base_pipeline);
    mgfx_shutdown();
    free(rdata);
}

void naui_renderer_resize(int32_t width, int32_t height)
{
    mgfx_resize(width, height);
    rdata->width = width;
    rdata->height = height;
}

bool naui_renderer_begin(void)
{
    if (mgfx_begin() != MGFX_RESULT_SUCCESS)
        return false;

    rdata->vertex_count = 0;
    rdata->vertex_offset = 0;
    rdata->index_count = 0;
    rdata->index_offset = 0;
    rdata->clip_stack_depth = 0;
    rdata->font_current_index = -1;

    mgfx_bind_pass(&(mgfx_pass_info){ 0 });
    mgfx_bind_pipeline(rdata->base_pipeline);
    mgfx_bind_vertex_buffer(rdata->batch_vb);
    mgfx_bind_index_buffer(rdata->batch_ib, MGFX_INDEX_TYPE_UINT32);
    mgfx_bind_image(rdata->image_atlas, rdata->linear_sampler, 0);

    struct { Naui_Vec2 u_resolution; } ub_data;
    ub_data.u_resolution = (Naui_Vec2){(float)rdata->width, (float)rdata->height};
    mgfx_bind_uniforms(0, sizeof(ub_data), &ub_data);
    
    return true;
}

void naui_renderer_end(void)
{
    naui_renderer_flush();
    mgfx_end();
}

void naui_push_clip_rect(float x, float y, float width, float height)
{
    if (rdata->clip_stack_depth >= NAUI_CLIP_STACK_MAX) return;

    naui_renderer_flush();

    float x0 = x, y0 = y;
    float x1 = x0 + width, y1 = y0 + height;

    if (rdata->clip_stack_depth > 0)
    {
        Naui_ClipRect *p = &rdata->clip_stack[rdata->clip_stack_depth - 1];
        if (x0 < p->x0) x0 = p->x0;
        if (y0 < p->y0) y0 = p->y0;
        if (x1 > p->x1) x1 = p->x1;
        if (y1 > p->y1) y1 = p->y1;
        if (x1 < x0) x1 = x0;
        if (y1 < y0) y1 = y0;
    }

    rdata->clip_stack[rdata->clip_stack_depth++] = (Naui_ClipRect){ x0, y0, x1, y1 };
    naui_apply_scissor();
}

void naui_pop_clip_rect(void)
{
    if (rdata->clip_stack_depth == 0) return;

    naui_renderer_flush();
    rdata->clip_stack_depth--;
    naui_apply_scissor();
}

static int naui_bake_font_size(
    const uint8_t  *ttf_buf,
    stbtt_fontinfo *info,
    float           bake_size,
    Naui_FontBake  *bake)
{
    int raw_ascent, raw_descent, raw_line_gap;
    stbtt_GetFontVMetrics(info, &raw_ascent, &raw_descent, &raw_line_gap);
    float font_scale = stbtt_ScaleForPixelHeight(info, bake_size);
    bake->ascent  = raw_ascent  * font_scale;
    bake->descent = -raw_descent * font_scale;

    int total_codepoints = 0;
    for (int b = 0; b < K_NUM_UNICODE_BLOCKS; b++)
        total_codepoints += k_unicode_blocks[b].count;

    typedef struct { int first_codepoint; int num_chars; } RangeMeta;
    RangeMeta *meta = malloc(total_codepoints * sizeof(RangeMeta));
    if (!meta) return 0;

    int range_count  = 0;
    int total_glyphs = 0;

    for (int b = 0; b < K_NUM_UNICODE_BLOCKS; b++)
    {
        int block_start = k_unicode_blocks[b].start;
        int block_end   = block_start + k_unicode_blocks[b].count;
        int run_start   = -1;

        for (int cp = block_start; cp <= block_end; cp++)
        {
            int present = (cp < block_end) && (stbtt_FindGlyphIndex(info, cp) != 0);

            if (present && run_start < 0)
            {
                run_start = cp;
            }
            else if (!present && run_start >= 0)
            {
                int num = cp - run_start;
                meta[range_count].first_codepoint = run_start;
                meta[range_count].num_chars       = num;
                range_count++;
                total_glyphs += num;
                run_start = -1;
            }
        }
    }

    if (range_count == 0)
    {
        free(meta);
        return 0;
    }

    stbtt_pack_range *tmp_ranges = malloc(range_count * sizeof(stbtt_pack_range));
    if (!tmp_ranges)
    {
        free(meta);
        return 0;
    }

    for (int i = 0; i < range_count; i++)
    {
        stbtt_packedchar *chars = malloc(meta[i].num_chars * sizeof(stbtt_packedchar));
        if (!chars)
        {
            for (int j = 0; j < i; j++) free(tmp_ranges[j].chardata_for_range);
            free(tmp_ranges); free(meta);
            return 0;
        }
        tmp_ranges[i].font_size                        = bake_size;
        tmp_ranges[i].first_unicode_codepoint_in_range = meta[i].first_codepoint;
        tmp_ranges[i].array_of_unicode_codepoints       = NULL;
        tmp_ranges[i].num_chars                         = meta[i].num_chars;
        tmp_ranges[i].chardata_for_range                = chars;
    }
    free(meta);

    int chosen_atlas_size = 0;
    uint8_t *alpha = NULL;

    int initial = naui_choose_initial_atlas(total_glyphs);

    for (int ci = 0; ci < K_NUM_ATLAS_CANDIDATES; ci++)
    {
        if (k_atlas_size_candidates[ci] < initial) continue;

        int sz = k_atlas_size_candidates[ci];
        uint8_t *buf = calloc(sz * sz, 1);
        if (!buf) continue;

        stbtt_pack_context pc;
        stbtt_PackBegin(&pc, buf, sz, sz, 0, 1, NULL);
        stbtt_PackSetOversampling(&pc, 1, 1);
        int ok = stbtt_PackFontRanges(&pc, ttf_buf, 0, tmp_ranges, range_count);
        stbtt_PackEnd(&pc);

        if (ok)
        {
            chosen_atlas_size = sz;
            alpha = buf;
            break;
        }
        free(buf);
    }

    if (!alpha)
    {
        for (int i = 0; i < range_count; i++) free(tmp_ranges[i].chardata_for_range);
        free(tmp_ranges);
        return 0;
    }

    bake->atlas = mgfx_create_image(&(mgfx_image_create_info){
        .type   = MGFX_IMAGE_TYPE_2D,
        .format = MGFX_FORMAT_R8_UNORM,
        .usage  = MGFX_IMAGE_USAGE_COLOR_ATTACHMENT,
        .width  = (uint32_t)chosen_atlas_size,
        .height = (uint32_t)chosen_atlas_size,
        .data   = alpha,
    });

    free(alpha);

    bake->ranges = malloc(range_count * sizeof(Naui_FontRange));
    if (!bake->ranges)
    {
        for (int i = 0; i < range_count; i++) free(tmp_ranges[i].chardata_for_range);
        free(tmp_ranges);
        return 0;
    }

    bake->atlas_size  = chosen_atlas_size;
    bake->range_count = range_count;
    for (int i = 0; i < range_count; i++)
    {
        bake->ranges[i].first_codepoint = tmp_ranges[i].first_unicode_codepoint_in_range;
        bake->ranges[i].num_chars       = tmp_ranges[i].num_chars;
        bake->ranges[i].chars           = tmp_ranges[i].chardata_for_range;
    }
    free(tmp_ranges);

    bake->baked_size = bake_size;
    bake->loaded     = true;
    return 1;
}

void naui_load_font(uint8_t index, const char *file_name)
{
    if (index >= NAUI_FONT_MAX_SLOTS) return;
    char final_file_name[64];
    strncpy(final_file_name, file_name, strlen(file_name) + 1);
    strncat(final_file_name, ".ttf", sizeof(final_file_name) - 1);

	Naui_Path font_path = NAUI_PATH("Assets/Fonts", final_file_name);
    FILE *f = fopen(font_path.data, "rb");
	
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *ttf_buf = malloc(size);
    if (!ttf_buf) { fclose(f); return; }
    fread(ttf_buf, 1, size, f);
    fclose(f);

    naui_unload_font(index);

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf_buf, 0))
    {
        free(ttf_buf);
        return;
    }

    bool loaded = naui_bake_font_size(ttf_buf, &info, NAUI_FONT_BAKE_SIZE, &rdata->font[index]);

    free(ttf_buf);
    rdata->font_loaded[index] = loaded;
}

void naui_unload_font(uint8_t index)
{
    if (!rdata->font_loaded[index])
        return;

    naui_free_bake(&rdata->font[index]);
    rdata->font_loaded[index] = false;
}

Naui_Vec2 naui_measure_text(const char *text, uint32_t length, float font_size, uint8_t font_index)
{
    if (!text || length == 0) return (Naui_Vec2){0};
    if (font_index >= NAUI_FONT_MAX_SLOTS) return (Naui_Vec2){0};
    if (!rdata->font_loaded[font_index]) return (Naui_Vec2){0};

    const Naui_FontBake *bake = &rdata->font[font_index];

    float scale  = font_size / bake->baked_size;
    float x = 0.0f;
    float max_x  = 0.0f;
    int num_lines = 1;
    int has_glyph = 0;

    const char *p   = text;
    const char *end = text + length;

    while (p < end)
    {
        int cp;
        p = naui_utf8_decode(p, &cp);

        if (cp == '\n')
        {
            if (x > max_x) max_x = x;
            x = 0.0f;
            num_lines++;
            continue;
        }
        if (cp == '\r') continue;

        const stbtt_packedchar *bc = naui_bake_lookup(bake, cp);
        if (!bc) bc = naui_bake_lookup(bake, '?');
        if (!bc)
        {
            x += font_size * 0.25f;
            continue;
        }

        x += bc->xadvance * scale;
        if (x > max_x) max_x = x;
        has_glyph = 1;
    }

    if (!has_glyph) return (Naui_Vec2){0};

    return (Naui_Vec2){ .x = max_x, .y = font_size * num_lines };
}

void naui_draw_text(Naui_Vec2 position, const char *text, float size, uint8_t font_index, Naui_Color color)
{
    if (!text || !*text) return;
    if (font_index >= NAUI_FONT_MAX_SLOTS) return;
    if (!rdata->font_loaded[font_index]) return;

    const Naui_FontBake *bake = &rdata->font[font_index];

    if (rdata->font_current_index != (int8_t)font_index)
    {
        naui_renderer_flush();
        mgfx_bind_image(bake->atlas, rdata->linear_sampler , 1);
        rdata->font_current_index = (int8_t)font_index;
    }

    float scale = size / bake->baked_size;
    float atlas_sz = (float)bake->atlas_size;

    int num_lines = 1;
    for (const char *p = text; *p; p++)
        if (*p == '\n') num_lines++;

    float line_ascent  = bake->ascent  * scale;
    float line_descent = bake->descent * scale;
    float line_height  = line_ascent + line_descent;

    float start_x   = position.x;
    float block_top = position.y;
    float x = start_x;
    float y = block_top + line_ascent;

    uint32_t c = naui_pack_color(color);

    Naui_ClipRect clip = naui_current_clip();

    for (const char *p = text; *p; )
    {
        int cp;
        p = naui_utf8_decode(p, &cp);

        if (cp == '\n')
        {
            x = start_x;
            y += line_height;
            continue;
        }
        if (cp == '\r') continue;

        const stbtt_packedchar *bc = naui_bake_lookup(bake, cp);
        if (!bc) bc = naui_bake_lookup(bake, '?');
        if (!bc)
        {
            x += size * 0.25f;
            continue;
        }

        float gx0 = x + bc->xoff  * scale;
        float gy0 = y + bc->yoff  * scale;
        float gx1 = gx0 + (bc->x1 - bc->x0) * scale;
        float gy1 = gy0 + (bc->y1 - bc->y0) * scale;

        if (gx1 < clip.x0 || gx0 > clip.x1 || gy1 < clip.y0 || gy0 > clip.y1)
        {
            x += bc->xadvance * scale;
            continue;
        }

        if (!naui_reserve(4, 6))
            break;

        float gu0 = bc->x0 / atlas_sz, gv0 = bc->y0 / atlas_sz;
        float gu1 = bc->x1 / atlas_sz, gv1 = bc->y1 / atlas_sz;

        uint32_t base = rdata->vertex_count;
        Naui_BatchVertex *v = &rdata->vertices[base];

        v[0] = (Naui_BatchVertex){ {gx0, gy0}, {gu0, gv0}, c, 1 };
        v[1] = (Naui_BatchVertex){ {gx1, gy0}, {gu1, gv0}, c, 1 };
        v[2] = (Naui_BatchVertex){ {gx1, gy1}, {gu1, gv1}, c, 1 };
        v[3] = (Naui_BatchVertex){ {gx0, gy1}, {gu0, gv1}, c, 1 };
        rdata->vertex_count += 4;

        uint32_t *idx = &rdata->indices[rdata->index_count];
        idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
        idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;
        rdata->index_count += 6;

        x += bc->xadvance * scale;
    }
}

void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding, Naui_CornerFlags corners)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    uint32_t c = naui_pack_color(color);

    if (rounding <= 0.0f || corners == NAUI_CORNER_NONE)
    {
        naui_push_rect(position, (Naui_Vec2){x1, y1}, c, -1);
        return;
    }

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);

    int tl = (corners & NAUI_CORNER_TL) != 0;
    int tr = (corners & NAUI_CORNER_TR) != 0;
    int br = (corners & NAUI_CORNER_BR) != 0;
    int bl = (corners & NAUI_CORNER_BL) != 0;

    naui_push_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, c, -1);
    naui_push_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, c, -1);
    naui_push_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, c, -1);

    if (tl) naui_push_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, c);
    if (tr) naui_push_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, c);
    if (br) naui_push_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, c);
    if (bl) naui_push_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, c);
}

void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding, Naui_CornerFlags corners, Naui_SideFlags sides)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float lw = line_width;
    uint32_t c = naui_pack_color(color);

    int side_top    = (sides & NAUI_SIDE_TOP)       != 0;
    int side_right  = (sides & NAUI_SIDE_RIGHT)     != 0;
    int side_bottom = (sides & NAUI_SIDE_BOTTOM)    != 0;
    int side_left   = (sides & NAUI_SIDE_LEFT)      != 0;

    if (rounding <= 0.0f || corners == NAUI_CORNER_NONE)
    {
        if (side_top)    naui_push_rect((Naui_Vec2){x0-lw, y0-lw}, (Naui_Vec2){x1+lw, y0    }, c, -1);
        if (side_bottom) naui_push_rect((Naui_Vec2){x0-lw, y1    }, (Naui_Vec2){x1+lw, y1+lw}, c, -1);
        if (side_left)   naui_push_rect((Naui_Vec2){x0-lw, y0    }, (Naui_Vec2){x0,    y1    }, c, -1);
        if (side_right)  naui_push_rect((Naui_Vec2){x1,    y0    }, (Naui_Vec2){x1+lw, y1    }, c, -1);
        return;
    }

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    float outer_r = r + lw;

    int tl = (corners & NAUI_CORNER_TL) != 0 && side_top    && side_left;
    int tr = (corners & NAUI_CORNER_TR) != 0 && side_top    && side_right;
    int br = (corners & NAUI_CORNER_BR) != 0 && side_bottom && side_right;
    int bl = (corners & NAUI_CORNER_BL) != 0 && side_bottom && side_left;

    if (side_top)    naui_push_rect((Naui_Vec2){x0 + (tl ? r : -lw), y0-lw}, (Naui_Vec2){x1 - (tr ? r : -lw), y0}, c, -1);
    if (side_bottom) naui_push_rect((Naui_Vec2){x0 + (bl ? r : -lw), y1   }, (Naui_Vec2){x1 - (br ? r : -lw), y1+lw}, c, -1);
    if (side_left)   naui_push_rect((Naui_Vec2){x0-lw, y0 + (tl ? r : 0)}, (Naui_Vec2){x0, y1 - (bl ? r : 0)}, c, -1);
    if (side_right)  naui_push_rect((Naui_Vec2){x1,    y0 + (tr ? r : 0)}, (Naui_Vec2){x1+lw, y1 - (br ? r : 0)}, c, -1);

    if (tl) naui_push_corner_ring((Naui_Vec2){x0+r, y0+r}, r, outer_r, -1, -1, c);
    if (tr) naui_push_corner_ring((Naui_Vec2){x1-r, y0+r}, r, outer_r, +1, -1, c);
    if (br) naui_push_corner_ring((Naui_Vec2){x1-r, y1-r}, r, outer_r, +1, +1, c);
    if (bl) naui_push_corner_ring((Naui_Vec2){x0+r, y1-r}, r, outer_r, -1, +1, c);
}

void naui_fill_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float rounding, Naui_CornerFlags corners)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    Naui_GradientAxis axis = naui_gradient_axis((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, gradient.angle);

    if (rounding <= 0.0f || corners == NAUI_CORNER_NONE)
    {
        naui_push_gradient_rect((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, &gradient, &axis);
        return;
    }

    int tl = (corners & NAUI_CORNER_TL) != 0;
    int tr = (corners & NAUI_CORNER_TR) != 0;
    int br = (corners & NAUI_CORNER_BR) != 0;
    int bl = (corners & NAUI_CORNER_BL) != 0;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);

    naui_push_gradient_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, &gradient, &axis);
    naui_push_gradient_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, &gradient, &axis);
    naui_push_gradient_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, &gradient, &axis);

    if (tl) naui_push_gradient_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, &gradient, &axis);
    if (tr) naui_push_gradient_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, &gradient, &axis);
    if (br) naui_push_gradient_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, &gradient, &axis);
    if (bl) naui_push_gradient_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, &gradient, &axis);
}

void naui_draw_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float line_width, float rounding, Naui_CornerFlags corners, Naui_SideFlags sides)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float lw = line_width;
    Naui_GradientAxis axis = naui_gradient_axis((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, gradient.angle);

    int side_top    = (sides & NAUI_SIDE_TOP)       != 0;
    int side_right  = (sides & NAUI_SIDE_RIGHT)     != 0;
    int side_bottom = (sides & NAUI_SIDE_BOTTOM)    != 0;
    int side_left   = (sides & NAUI_SIDE_LEFT)      != 0;

    if (rounding <= 0.0f || corners == NAUI_CORNER_NONE)
    {
        if (side_top)    naui_push_gradient_rect((Naui_Vec2){x0-lw, y0-lw}, (Naui_Vec2){x1+lw, y0    }, &gradient, &axis);
        if (side_bottom) naui_push_gradient_rect((Naui_Vec2){x0-lw, y1    }, (Naui_Vec2){x1+lw, y1+lw}, &gradient, &axis);
        if (side_left)   naui_push_gradient_rect((Naui_Vec2){x0-lw, y0    }, (Naui_Vec2){x0,    y1    }, &gradient, &axis);
        if (side_right)  naui_push_gradient_rect((Naui_Vec2){x1,    y0    }, (Naui_Vec2){x1+lw, y1    }, &gradient, &axis);
        return;
    }

    int tl = (corners & NAUI_CORNER_TL) != 0 && side_top    && side_left;
    int tr = (corners & NAUI_CORNER_TR) != 0 && side_top    && side_right;
    int br = (corners & NAUI_CORNER_BR) != 0 && side_bottom && side_right;
    int bl = (corners & NAUI_CORNER_BL) != 0 && side_bottom && side_left;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    float outer_r = r + lw;

    if (side_top)    naui_push_gradient_rect((Naui_Vec2){x0 + (tl ? r : -lw), y0-lw}, (Naui_Vec2){x1 - (tr ? r : -lw), y0    }, &gradient, &axis);
    if (side_bottom) naui_push_gradient_rect((Naui_Vec2){x0 + (bl ? r : -lw), y1   }, (Naui_Vec2){x1 - (br ? r : -lw), y1+lw }, &gradient, &axis);
    if (side_left)   naui_push_gradient_rect((Naui_Vec2){x0-lw, y0 + (tl ? r : 0)}, (Naui_Vec2){x0, y1 - (bl ? r : 0)}, &gradient, &axis);
    if (side_right)  naui_push_gradient_rect((Naui_Vec2){x1,    y0 + (tr ? r : 0)}, (Naui_Vec2){x1+lw, y1 - (br ? r : 0)}, &gradient, &axis);

    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx = s_cos[s],     cy = s_sin[s];
        float nx = s_cos[s + 1], ny = s_sin[s + 1];

        float ax_vals[4] = { -1, +1, +1, -1 };
        float ay_vals[4] = { -1, -1, +1, +1 };
        Naui_Vec2 centers[4] = {
            { x0+r, y0+r },
            { x1-r, y0+r },
            { x1-r, y1-r },
            { x0+r, y1-r },
        };
        int corner_flags[4] = { tl, tr, br, bl };

        for (int corner = 0; corner < 4; corner++)
        {
            if (!corner_flags[corner]) continue;

            float ax = ax_vals[corner], ay = ay_vals[corner];
            Naui_Vec2 center = centers[corner];

            Naui_Vec2 o0 = { center.x + ax * cx * outer_r, center.y + ay * cy * outer_r };
            Naui_Vec2 o1 = { center.x + ax * nx * outer_r, center.y + ay * ny * outer_r };
            Naui_Vec2 i0 = { center.x + ax * cx * r,       center.y + ay * cy * r       };
            Naui_Vec2 i1 = { center.x + ax * nx * r,       center.y + ay * ny * r       };

            Naui_Vec2 ring_quad[4] = { o0, o1, i1, i0 };
            naui_push_gradient_convex(ring_quad, 4, &gradient, &axis);
        }
    }
}

void naui_draw_line(Naui_Vec2 a, Naui_Vec2 b, Naui_Color color, float line_width)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len_sq = dx * dx + dy * dy;
    if (len_sq < 1e-20f)
        return;
    float len = sqrtf(len_sq);
    float half_w = line_width * 0.5f / len;
    float nx = -dy * half_w;
    float ny =  dx * half_w;
    naui_push_quad4(
        (Naui_Vec2){ a.x + nx, a.y + ny },
        (Naui_Vec2){ b.x + nx, b.y + ny },
        (Naui_Vec2){ b.x - nx, b.y - ny },
        (Naui_Vec2){ a.x - nx, a.y - ny },
        naui_pack_color(color), -1
    );
}

static inline Naui_Vec4 uv_for_sub(Naui_Vec4 uv, Naui_Vec2 img_tl, Naui_Vec2 img_size, float sx0, float sy0, float sx1, float sy1)
{
    float u0 = uv.x + ((sx0 - img_tl.x) / img_size.x) * uv.z;
    float v0 = uv.y + ((sy0 - img_tl.y) / img_size.y) * uv.w;
    float u1 = ((sx1 - sx0) / img_size.x) * uv.z;
    float v1 = ((sy1 - sy0) / img_size.y) * uv.w;
    return (Naui_Vec4){ u0, v0, u1, v1 };
}

static void naui_push_textured_corner_fan(
    Naui_Vec2 center, float r, float ax, float ay,
    Naui_Vec4 uv_rect,
    Naui_Vec2 img_tl, Naui_Vec2 img_size,
    uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx0 = ax * s_cos[s],     cy0 = ay * s_sin[s];
        float cx1 = ax * s_cos[s + 1], cy1 = ay * s_sin[s + 1];

        Naui_Vec2 p0 = { center.x + cx0 * r, center.y + cy0 * r };
        Naui_Vec2 p1 = { center.x + cx1 * r, center.y + cy1 * r };

        float u_c  = uv_rect.x + ((center.x - img_tl.x) / img_size.x) * uv_rect.z;
        float v_c  = uv_rect.y + ((center.y - img_tl.y) / img_size.y) * uv_rect.w;
        float u_p0 = uv_rect.x + ((p0.x     - img_tl.x) / img_size.x) * uv_rect.z;
        float v_p0 = uv_rect.y + ((p0.y     - img_tl.y) / img_size.y) * uv_rect.w;
        float u_p1 = uv_rect.x + ((p1.x     - img_tl.x) / img_size.x) * uv_rect.z;
        float v_p1 = uv_rect.y + ((p1.y     - img_tl.y) / img_size.y) * uv_rect.w;

        uint32_t base = rdata->vertex_count;
        Naui_BatchVertex *v = &rdata->vertices[base];

        v[0] = (Naui_BatchVertex){ {center.x, center.y}, {u_c,  v_c},  color, 0 };
        v[1] = (Naui_BatchVertex){ {p0.x,     p0.y    }, {u_p0, v_p0}, color, 0 };
        v[2] = (Naui_BatchVertex){ {p1.x,     p1.y    }, {u_p1, v_p1}, color, 0 };
        rdata->vertex_count += 3;

        uint32_t *idx = &rdata->indices[rdata->index_count];
        idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
        rdata->index_count += 3;
    }
}

void naui_draw_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint, float rounding, Naui_CornerFlags corners)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;

    const float s0 = image->texture_area[0], t0 = image->texture_area[1];
    const float s1 = image->texture_area[2], t1 = image->texture_area[3];
    Naui_Vec4 uv = { s0, t0, s1, t1 };
    uint32_t c = naui_pack_color(tint);

    if (rounding <= 0.0f || corners == NAUI_CORNER_NONE)
    {
        naui_push_textured_rect(position, (Naui_Vec2){x1, y1}, uv, c, 0);
        return;
    }

    int tl = (corners & NAUI_CORNER_TL) != 0;
    int tr = (corners & NAUI_CORNER_TR) != 0;
    int br = (corners & NAUI_CORNER_BR) != 0;
    int bl = (corners & NAUI_CORNER_BL) != 0;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    Naui_Vec2 img_tl   = { x0, y0 };
    Naui_Vec2 img_size = { scale.x, scale.y };

    naui_push_textured_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, uv_for_sub(uv, img_tl, img_size, x0+r, y0, x1-r, y1), c, 0);
    naui_push_textured_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x0, y0 + (tl ? r : 0), x0+r, y1 - (bl ? r : 0)), c, 0);
    naui_push_textured_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x1-r, y0 + (tr ? r : 0), x1, y1 - (br ? r : 0)), c, 0);

    if (tl) naui_push_textured_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, uv, img_tl, img_size, c);
    if (tr) naui_push_textured_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, uv, img_tl, img_size, c);
    if (br) naui_push_textured_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, uv, img_tl, img_size, c);
    if (bl) naui_push_textured_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, uv, img_tl, img_size, c);
}

typedef struct
{
    Naui_Vec2 pos;
    Naui_Vec2 uv;
}
Naui_TexGradientVert;

typedef struct
{
    Naui_TexGradientVert v[NAUI_GRADIENT_CLIP_MAX_VERTS];
    int count;
}
Naui_TexGradientPoly;

static Naui_TexGradientPoly naui_tex_clip_poly_by_t(const Naui_TexGradientPoly *in, const Naui_GradientAxis *axis, float line_t, int keep_ge)
{
    Naui_TexGradientPoly out = { .count = 0 };
    if (in->count == 0) return out;

    for (int i = 0; i < in->count; i++)
    {
        Naui_TexGradientVert cur  = in->v[i];
        Naui_TexGradientVert next = in->v[(i + 1) % in->count];

        float t_cur  = naui_gradient_axis_t(axis, cur.pos.x,  cur.pos.y);
        float t_next = naui_gradient_axis_t(axis, next.pos.x, next.pos.y);

        int cur_in  = keep_ge ? (t_cur  >= line_t) : (t_cur  <= line_t);
        int next_in = keep_ge ? (t_next >= line_t) : (t_next <= line_t);

        if (cur_in && out.count < NAUI_GRADIENT_CLIP_MAX_VERTS)
            out.v[out.count++] = cur;

        if (cur_in != next_in)
        {
            float denom = t_next - t_cur;
            float f = (fabsf(denom) < 1e-8f) ? 0.0f : (line_t - t_cur) / denom;
            f = naui_clamp01(f);

            Naui_TexGradientVert ix;
            ix.pos.x = cur.pos.x + (next.pos.x - cur.pos.x) * f;
            ix.pos.y = cur.pos.y + (next.pos.y - cur.pos.y) * f;
            ix.uv.x  = cur.uv.x  + (next.uv.x  - cur.uv.x)  * f;
            ix.uv.y  = cur.uv.y  + (next.uv.y  - cur.uv.y)  * f;

            if (out.count < NAUI_GRADIENT_CLIP_MAX_VERTS)
                out.v[out.count++] = ix;
        }
    }

    return out;
}

static void naui_push_textured_gradient_triangle(
    Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c,
    Naui_Vec2 uv_a, Naui_Vec2 uv_b, Naui_Vec2 uv_c,
    uint32_t ca, uint32_t cb, uint32_t cc,
    int32_t texture_id)
{
    if (!naui_reserve(3, 3))
        return;

    uint32_t base = rdata->vertex_count;
    Naui_BatchVertex *v = &rdata->vertices[base];

    v[0] = (Naui_BatchVertex){ {a.x, a.y}, {uv_a.x, uv_a.y}, ca, texture_id };
    v[1] = (Naui_BatchVertex){ {b.x, b.y}, {uv_b.x, uv_b.y}, cb, texture_id };
    v[2] = (Naui_BatchVertex){ {c.x, c.y}, {uv_c.x, uv_c.y}, cc, texture_id };
    rdata->vertex_count += 3;

    uint32_t *idx = &rdata->indices[rdata->index_count];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    rdata->index_count += 3;
}

static void naui_push_tex_flat_polygon(const Naui_TexGradientPoly *poly, uint32_t color, int32_t texture_id)
{
    if (poly->count < 3) return;

    Naui_TexGradientVert p0 = poly->v[0];
    for (int i = 1; i + 1 < poly->count; i++)
        naui_push_textured_gradient_triangle(
            p0.pos, poly->v[i].pos, poly->v[i + 1].pos,
            p0.uv,  poly->v[i].uv,  poly->v[i + 1].uv,
            color, color, color, texture_id);
}

static void naui_push_tex_gradient_polygon(const Naui_TexGradientPoly *poly, const Naui_Gradient *g, const Naui_GradientAxis *axis, int32_t texture_id)
{
    if (poly->count < 3) return;

    Naui_TexGradientVert p0 = poly->v[0];
    uint32_t c0 = naui_gradient_color_at(g, axis, p0.pos.x, p0.pos.y);

    for (int i = 1; i + 1 < poly->count; i++)
    {
        Naui_TexGradientVert p1 = poly->v[i];
        Naui_TexGradientVert p2 = poly->v[i + 1];
        naui_push_textured_gradient_triangle(
            p0.pos, p1.pos, p2.pos,
            p0.uv,  p1.uv,  p2.uv,
            c0,
            naui_gradient_color_at(g, axis, p1.pos.x, p1.pos.y),
            naui_gradient_color_at(g, axis, p2.pos.x, p2.pos.y),
            texture_id
        );
    }
}

static void naui_push_gradient_convex_textured(const Naui_TexGradientVert *verts, int vert_count, const Naui_Gradient *g, const Naui_GradientAxis *axis, int32_t texture_id)
{
    if (vert_count < 3 || vert_count > NAUI_GRADIENT_CLIP_MAX_VERTS) return;

    Naui_TexGradientPoly src = { .count = vert_count };
    for (int i = 0; i < vert_count; i++) src.v[i] = verts[i];

    float p1 = naui_clamp01(g->percent1);
    float p2 = naui_clamp01(g->percent2);

    uint32_t c1 = naui_pack_color(g->color1);
    uint32_t c2 = naui_pack_color(g->color2);

    if (p2 <= p1)
    {
        Naui_TexGradientPoly before = naui_tex_clip_poly_by_t(&src, axis, p1, 0);
        Naui_TexGradientPoly after  = naui_tex_clip_poly_by_t(&src, axis, p1, 1);
        naui_push_tex_flat_polygon(&before, c1, texture_id);
        naui_push_tex_flat_polygon(&after,  c2, texture_id);
        return;
    }

    Naui_TexGradientPoly flat_lo = naui_tex_clip_poly_by_t(&src, axis, p1, 0);
    Naui_TexGradientPoly rest    = naui_tex_clip_poly_by_t(&src, axis, p1, 1);
    Naui_TexGradientPoly ramp    = naui_tex_clip_poly_by_t(&rest, axis, p2, 0);
    Naui_TexGradientPoly flat_hi = naui_tex_clip_poly_by_t(&rest, axis, p2, 1);

    naui_push_tex_flat_polygon(&flat_lo, c1, texture_id);
    naui_push_tex_gradient_polygon(&ramp, g, axis, texture_id);
    naui_push_tex_flat_polygon(&flat_hi, c2, texture_id);
}

static void naui_push_textured_gradient_corner_fan(
    Naui_Vec2 center, float r, float ax, float ay,
    Naui_Vec4 uv_rect,
    Naui_Vec2 img_tl, Naui_Vec2 img_size,
    const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx0 = ax * s_cos[s],     cy0 = ay * s_sin[s];
        float cx1 = ax * s_cos[s + 1], cy1 = ay * s_sin[s + 1];

        Naui_Vec2 p0 = { center.x + cx0 * r, center.y + cy0 * r };
        Naui_Vec2 p1 = { center.x + cx1 * r, center.y + cy1 * r };

        Naui_Vec2 uv_center = { uv_rect.x + ((center.x - img_tl.x) / img_size.x) * uv_rect.z, uv_rect.y + ((center.y - img_tl.y) / img_size.y) * uv_rect.w };
        Naui_Vec2 uv_p0 = { uv_rect.x + ((p0.x - img_tl.x) / img_size.x) * uv_rect.z, uv_rect.y + ((p0.y - img_tl.y) / img_size.y) * uv_rect.w };
        Naui_Vec2 uv_p1 = { uv_rect.x + ((p1.x - img_tl.x) / img_size.x) * uv_rect.z, uv_rect.y + ((p1.y - img_tl.y) / img_size.y) * uv_rect.w };

        Naui_TexGradientVert tri[3] = {
            { center, uv_center },
            { p0, uv_p0 },
            { p1, uv_p1 },
        };

        naui_push_gradient_convex_textured(tri, 3, g, axis, 0);
    }
}

static void naui_push_textured_gradient_rect(Naui_Vec2 tl, Naui_Vec2 br,
    Naui_Vec4 texture_area, const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    Naui_TexGradientVert verts[4] = {
        { {tl.x, tl.y}, {texture_area.x, texture_area.y } },
        { {br.x, tl.y}, {texture_area.x + texture_area.z, texture_area.y } },
        { {br.x, br.y}, {texture_area.x + texture_area.z, texture_area.y + texture_area.w} },
        { {tl.x, br.y}, {texture_area.x, texture_area.y + texture_area.w} },
    };
    naui_push_gradient_convex_textured(verts, 4, g, axis, 0);
}

void naui_draw_gradient_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient tint, float rounding, Naui_CornerFlags corners)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;

    const float s0 = image->texture_area[0], t0 = image->texture_area[1];
    const float s1 = image->texture_area[2], t1 = image->texture_area[3];
    Naui_Vec4 uv = { s0, t0, s1, t1 };

    Naui_GradientAxis axis = naui_gradient_axis((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, tint.angle);

    if (rounding <= 0.0f || corners == NAUI_CORNER_NONE)
    {
        naui_push_textured_gradient_rect((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, uv, &tint, &axis);
        return;
    }

    int tl = (corners & NAUI_CORNER_TL) != 0;
    int tr = (corners & NAUI_CORNER_TR) != 0;
    int br = (corners & NAUI_CORNER_BR) != 0;
    int bl = (corners & NAUI_CORNER_BL) != 0;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    Naui_Vec2 img_tl   = { x0, y0 };
    Naui_Vec2 img_size = { scale.x, scale.y };

    naui_push_textured_gradient_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, uv_for_sub(uv, img_tl, img_size, x0+r, y0, x1-r, y1), &tint, &axis);
    naui_push_textured_gradient_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x0, y0 + (tl ? r : 0), x0+r, y1 - (bl ? r : 0)), &tint, &axis);
    naui_push_textured_gradient_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x1-r, y0 + (tr ? r : 0), x1, y1 - (br ? r : 0)), &tint, &axis);

    if (tl) naui_push_textured_gradient_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, uv, img_tl, img_size, &tint, &axis);
    if (tr) naui_push_textured_gradient_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, uv, img_tl, img_size, &tint, &axis);
    if (br) naui_push_textured_gradient_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, uv, img_tl, img_size, &tint, &axis);
    if (bl) naui_push_textured_gradient_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, uv, img_tl, img_size, &tint, &axis);
}

void naui_draw_shadow(Naui_Vec2 position, Naui_Vec2 scale, float blur_radius, Naui_Color color, float rounding, Naui_CornerFlags corners)
{
    if (blur_radius <= 0.0f)
        return;

    float r  = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    float br = blur_radius;

    float x0 = position.x, y0 = position.y;
    float x1 = position.x + scale.x, y1 = position.y + scale.y;

    uint32_t c_full = naui_pack_color(color);
    uint32_t c_zero = naui_pack_color((Naui_Color){ color.r, color.g, color.b, 0 });

    int tl = (corners & NAUI_CORNER_TL) != 0;
    int tr = (corners & NAUI_CORNER_TR) != 0;
    int br_flag = (corners & NAUI_CORNER_BR) != 0;
    int bl = (corners & NAUI_CORNER_BL) != 0;

    float cx_tl = x0 + (tl ? r : 0), cy_tl = y0 + (tl ? r : 0);
    float cx_tr = x1 - (tr ? r : 0), cy_tr = y0 + (tr ? r : 0);
    float cx_br = x1 - (br_flag ? r : 0), cy_br = y1 - (br_flag ? r : 0);
    float cx_bl = x0 + (bl ? r : 0), cy_bl = y1 - (bl ? r : 0);

    naui_push_gradient_quad4(
        (Naui_Vec2){cx_tl, y0 - br}, (Naui_Vec2){cx_tr, y0 - br},
        (Naui_Vec2){cx_tr, y0     }, (Naui_Vec2){cx_tl, y0     },
        c_zero, c_zero, c_full, c_full, -1);

    naui_push_gradient_quad4(
        (Naui_Vec2){cx_bl, y1     }, (Naui_Vec2){cx_br, y1     },
        (Naui_Vec2){cx_br, y1 + br}, (Naui_Vec2){cx_bl, y1 + br},
        c_full, c_full, c_zero, c_zero, -1);

    naui_push_gradient_quad4(
        (Naui_Vec2){x0 - br, cy_tl}, (Naui_Vec2){x0, cy_tl},
        (Naui_Vec2){x0, cy_bl}, (Naui_Vec2){x0 - br, cy_bl},
        c_zero, c_full, c_full, c_zero, -1);

    naui_push_gradient_quad4(
        (Naui_Vec2){x1,      cy_tr}, (Naui_Vec2){x1 + br, cy_tr},
        (Naui_Vec2){x1 + br, cy_br}, (Naui_Vec2){x1,      cy_br},
        c_full, c_zero, c_zero, c_full, -1);

    struct {
        float cx, cy;
        float ax, ay;
        int enabled;
    } fan[4] = {
        { cx_tl, cy_tl, -1, -1, tl      },
        { cx_tr, cy_tr, +1, -1, tr      },
        { cx_br, cy_br, +1, +1, br_flag },
        { cx_bl, cy_bl, -1, +1, bl      },
    };

    for (int f = 0; f < 4; f++)
    {
        if (!fan[f].enabled)
        {
            float cx = fan[f].cx, cy = fan[f].cy;
            float ax = fan[f].ax, ay = fan[f].ay;

            naui_push_gradient_quad4(
                (Naui_Vec2){cx, cy },
                (Naui_Vec2){cx + ax*br, cy },
                (Naui_Vec2){cx + ax*br, cy + ay*br},
                (Naui_Vec2){cx, cy + ay*br},
                c_full, c_zero, c_zero, c_zero, -1);
            continue;
        }

        float cx = fan[f].cx, cy = fan[f].cy;
        float ax = fan[f].ax, ay = fan[f].ay;

        for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
        {
            float c0 = s_cos[s], s0 = s_sin[s];
            float c1 = s_cos[s + 1], s1 = s_sin[s + 1];

            Naui_Vec2 i0 = { cx + ax * c0 * r, cy + ay * s0 * r };
            Naui_Vec2 i1 = { cx + ax * c1 * r, cy + ay * s1 * r };

            Naui_Vec2 o0 = { cx + ax * c0 * (r + br), cy + ay * s0 * (r + br) };
            Naui_Vec2 o1 = { cx + ax * c1 * (r + br), cy + ay * s1 * (r + br) };

            naui_push_gradient_quad4(i0, i1, o1, o0,
                c_full, c_full, c_zero, c_zero, -1);
        }
    }
}

void naui_fill_polygon(const Naui_Vec2 *points, int point_count, Naui_Color color)
{
    if (!points || point_count < 3)
        return;

    uint32_t c = naui_pack_color(color);

    Naui_Vec2 p0 = points[0];
    for (int i = 1; i + 1 < point_count; i++)
        naui_push_triangle(p0, points[i], points[i + 1], c);
}

Naui_Vec4 naui_current_clip_rect(void)
{
    Naui_ClipRect c = naui_current_clip();
    return (Naui_Vec4){ c.x0, c.y0, c.x1 - c.x0, c.y1 - c.y0 };
}