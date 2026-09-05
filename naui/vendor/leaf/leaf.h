#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define LEAF_FEATURE_SHADOWS 1

#ifdef __cplusplus
extern "C" {
#endif
    
#define LEAF_COLOR_WHITE (Leaf_Color) { 255, 255, 255, 255 }
#define LEAF_COLOR_BLACK (Leaf_Color) { 0, 0, 0, 255 }
#define LEAF_COLOR_TRANSPARENT (Leaf_Color) { 0 }
    
#define LEAF_API // TODO(box): implement proper dll/wasm exporting
    
    typedef struct
    {
        uint64_t value;
        const char *label;
    }
    Leaf_ID;
    
    typedef struct
    {
        float x, y;
    }
    Leaf_Vec2;
    
    typedef struct
    {
        float width, height;
    }
    Leaf_Dimensions;
    
    typedef struct
    {
        uint8_t r, g, b, a;
    }
    Leaf_Color;
    
    typedef uint8_t Leaf_ColorFillType;
    enum
    {
        LEAF_SOLID_COLOR_FILL = 0,
        LEAF_GRADIENT_LINEAR_COLOR_FILL = 1
    };
    
    typedef struct
    {
        Leaf_Color color1;
        Leaf_Color color2;
        float percent1;
        float percent2;
        float angle;
        Leaf_ColorFillType type;
    }
    Leaf_ColorFill;
    
#define LEAF_GRADIENT(c1, c2, angle) (Leaf_ColorFill){ (c1), (c2), 0.0f, 1.0f, angle, LEAF_GRADIENT_LINEAR_COLOR_FILL }
#define LEAF_GRADIENT_PERCENT(c1, p1, c2, p2, angle) (Leaf_ColorFill){ (c1), (c2), (p1), (p2), angle, LEAF_GRADIENT_LINEAR_COLOR_FILL }
    
#define LEAF_DEG(v) ((v) * 0.0174532925f)
#define LEAF_RAD(v) (v)
    
    static inline Leaf_Color leaf_rgb(uint8_t r, uint8_t g, uint8_t b)
    {
        return (Leaf_Color){ r, g, b, 255 };
    }
    
    static inline Leaf_Color leaf_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return (Leaf_Color){ r, g, b, a };
    }
    
    static inline Leaf_Color leaf_hex(uint32_t hex)
    {
        return (Leaf_Color){
            (uint8_t)((hex >> 16) & 0xFF),
            (uint8_t)((hex >>  8) & 0xFF),
            (uint8_t)((hex >>  0) & 0xFF),
            255
        };
    }
    
    static inline Leaf_Color leaf_hexa(uint32_t hex)
    {
        return (Leaf_Color){
            (uint8_t)((hex >> 24) & 0xFF),
            (uint8_t)((hex >> 16) & 0xFF),
            (uint8_t)((hex >>  8) & 0xFF),
            (uint8_t)((hex >>  0) & 0xFF)
        };
    }

    static inline bool leaf_id_valid(Leaf_ID id)
    {
        return id.value != 0;
    }
    
    typedef struct
    {
        float x, y, width, height;
    }
    Leaf_BoundingBox;
    
    typedef uint8_t Leaf_Positioning;
    enum
    {
        LEAF_POSITIONING_RELATIVE,
        LEAF_POSITIONING_FLOATING_TO_PARENT,
        LEAF_POSITIONING_FLOATING_TO_ROOT
    };
    
    typedef uint8_t Leaf_LayoutDirection;
    enum
    {
        LEAF_DIRECTION_VERTICAL,
        LEAF_DIRECTION_HORIZONTAL
    };
    
    typedef uint8_t Leaf_LayoutAlignmentX;
    enum
    {
        LEAF_ALIGN_X_LEFT,
        LEAF_ALIGN_X_RIGHT,
        LEAF_ALIGN_X_CENTER
    };
    
    typedef uint8_t Leaf_LayoutAlignmentY;
    enum
    {
        LEAF_ALIGN_Y_TOP,
        LEAF_ALIGN_Y_BOTTOM,
        LEAF_ALIGN_Y_CENTER
    };
    
    typedef struct
    {
        Leaf_LayoutAlignmentX x;
        Leaf_LayoutAlignmentY y;
    }
    Leaf_Alignment;
    
    typedef uint8_t Leaf_SizeType;
    enum
    {
        LEAF_SIZE_TYPE_FIT,
        LEAF_SIZE_TYPE_GROW,
        LEAF_SIZE_TYPE_PERCENT,
        LEAF_SIZE_TYPE_FIXED,
        LEAF_SIZE_TYPE_DERIVED
    };
    
    typedef struct
    {
        struct
        {
            struct { float min, max; } min_max;
            float percent;
        }
        size;
        Leaf_SizeType type;
    }
    Leaf_SizeAxis;
    
    typedef struct
    {
        Leaf_SizeAxis width;
        Leaf_SizeAxis height;
    }
    Leaf_Size;
    
    typedef struct
    {
        float left, right, top, bottom;
    }
    Leaf_Padding;
#define LEAF_PADDING_ALL(v) (Leaf_Padding) { v, v, v, v }
#define LEAF_PADDING_AXES(h, v) (Leaf_Padding) { h, h, v, v }
    
    typedef uint8_t Leaf_Sides;
    enum
    {
        LEAF_SIDE_NONE = 0,
        LEAF_SIDE_TOP = 1 << 0,
        LEAF_SIDE_RIGHT = 1 << 1,
        LEAF_SIDE_BOTTOM = 1 << 2,
        LEAF_SIDE_LEFT = 1 << 3,
        LEAF_SIDE_ALL = LEAF_SIDE_TOP | LEAF_SIDE_RIGHT | LEAF_SIDE_BOTTOM | LEAF_SIDE_LEFT
    };
    
    typedef struct
    {
        float width;
        Leaf_Sides sides;
        Leaf_ColorFill color;
    }
    Leaf_Border;
    
    typedef void *Leaf_Image;
    
    typedef uint8_t Leaf_Corners;
    enum
    {
        LEAF_CORNER_NONE = 0,
        LEAF_CORNER_TL = 1 << 0,
        LEAF_CORNER_TR = 1 << 1,
        LEAF_CORNER_BR = 1 << 2,
        LEAF_CORNER_BL = 1 << 3,
        LEAF_CORNER_ALL = LEAF_CORNER_TL | LEAF_CORNER_TR | LEAF_CORNER_BR | LEAF_CORNER_BL,
    };
    
    typedef uint8_t Leaf_RoundingType;
    enum
    {
        LEAF_ROUNDING_TYPE_FIXED,
        LEAF_ROUNDING_TYPE_PERCENT
    };
    
    typedef struct
    {
        float value;
        Leaf_Corners corners;
        Leaf_RoundingType type;
    }
    Leaf_Rounding;
#define LEAF_ROUNDING_FIXED(v, c) (Leaf_Rounding) {.value = (v), .corners = (c), .type = LEAF_ROUNDING_TYPE_FIXED}
#define LEAF_ROUNDING_PERCENT(v, c) (Leaf_Rounding) {.value = (v), .corners = (c), .type = LEAF_ROUNDING_TYPE_PERCENT}
#define LEAF_ROUNDING_FULL(c) (Leaf_Rounding) {.value = 1.0f, .corners = (c), .type = LEAF_ROUNDING_TYPE_PERCENT}
    
    typedef void (*Leaf_CustomDrawFn)(Leaf_BoundingBox box, void *user_data);
    
    typedef struct
    {
        void *data;
        size_t size;
    }
    Leaf_DataSlice;
#define LEAF_DATA_SLICE(x) (Leaf_DataSlice) { .data = &(x), .size = sizeof(x) }
    
#ifdef LEAF_FEATURE_SHADOWS
    typedef struct
    {
        float blur_radius;
        Leaf_Vec2 offset;
        Leaf_Color color;
    }
    Leaf_Shadow;
#endif
    
    typedef struct
    {
        Leaf_Alignment parent_alignment;
        Leaf_Alignment self_alignment;
        Leaf_Vec2 offset;
    }
    Leaf_Floating;

    typedef uint8_t Leaf_UniformSizing;
    enum
    {
        LEAF_UNIFORM_SIZING_NONE,
        LEAF_UNIFORM_SIZING_WIDTH,
        LEAF_UNIFORM_SIZING_HEIGHT,
        LEAF_UNIFORM_SIZING_BOTH
    };
    
    typedef struct
    {
        Leaf_ID id;
        Leaf_Image image;
        
        Leaf_CustomDrawFn custom_draw;
        Leaf_DataSlice custom_draw_data;
        
        Leaf_Size size;
        Leaf_Padding padding;
        Leaf_Border border;
        Leaf_ColorFill color;
        Leaf_Vec2 child_offset;
        Leaf_Alignment child_alignment;
        
        Leaf_Floating floating;
        
#ifdef LEAF_FEATURE_SHADOWS
        Leaf_Shadow shadow;
#endif
        Leaf_Rounding rounding;
        
        float child_gap;
        float child_cross_gap;
        float aspect_ratio;
        
        Leaf_UniformSizing uniform_children;
        Leaf_LayoutDirection direction;
        Leaf_Positioning positioning;
        
        bool clip_children;
        bool wrap_children;
    }
    Leaf_ElementConfig;
    
    typedef struct
    {
        Leaf_BoundingBox bounding_box;
        uint64_t id;
        uint32_t frame;
    }
    Leaf_LayoutFrameEntry;
    
    typedef uint8_t Leaf_TextAlignment;
    enum
    {
        LEAF_TEXT_ALIGN_LEFT,
        LEAF_TEXT_ALIGN_CENTER,
        LEAF_TEXT_ALIGN_RIGHT
    };
    
    typedef uint8_t Leaf_TextWrapMode;
    enum
    {
        LEAF_TEXT_WRAP_MODE_NONE,
        LEAF_TEXT_WRAP_MODE_CHAR,
        LEAF_TEXT_WRAP_MODE_WORD
    };
    
    typedef struct
    {
        Leaf_ColorFill color;
        Leaf_SizeAxis font_size;
        uint32_t font_id;
        Leaf_TextAlignment alignment;
        Leaf_TextWrapMode wrap_mode;
    }
    Leaf_TextConfig;
    
    typedef uint8_t Leaf_RenderCmdType;
    enum
    {
        LEAF_RENDER_CMD_RECT,
        LEAF_RENDER_CMD_RECT_LINES,
        LEAF_RENDER_CMD_TEXT,
        LEAF_RENDER_CMD_IMAGE,
        LEAF_RENDER_CMD_SCISSOR_PUSH,
        LEAF_RENDER_CMD_SCISSOR_POP,
        LEAF_RENDER_CMD_CUSTOM,
#ifdef LEAF_FEATURE_SHADOWS
        LEAF_RENDER_CMD_SHADOW
#endif
    };
    
    typedef struct
    {
        union
        {
            struct
            {
                float line_width;
                float rounding;
                Leaf_Corners rounding_corners;
                Leaf_Sides sides;
            }
            rect;
            
            struct
            {
                const char *text;
                float font_size;
                uint32_t font_id;
            }
            text;
            
            struct
            {
                void *handle;
                float rounding;
                Leaf_Corners rounding_corners;
            }
            image;
            
#ifdef LEAF_FEATURE_SHADOWS
            struct
            {
                Leaf_Vec2 offset;
                float blur_radius;
                float rounding;
                Leaf_Corners rounding_corners;
            }
            shadow;
#endif
            
            struct
            {
                Leaf_CustomDrawFn draw;
                void *user_data;
            }
            custom;
        };
        
        Leaf_RenderCmdType type;
        Leaf_BoundingBox bounding_box;
        Leaf_ColorFill color;
    }
    Leaf_RenderCmd;
    
    typedef struct Leaf_RenderCmdNode Leaf_RenderCmdNode;
    struct Leaf_RenderCmdNode
    {
        Leaf_RenderCmd cmd;
        Leaf_RenderCmdNode *next;
    };
    
    typedef struct
    {
        Leaf_RenderCmdNode *first;
        Leaf_RenderCmdNode *last;
        uint32_t count;
    }
    Leaf_RenderCmdList;
    
    typedef Leaf_Dimensions (*Leaf_MeasureTextFn)(const char *text, uint32_t length, float resolved_font_size, const Leaf_TextConfig *config);
    
    // All the avaliable space in the parent (IMPORTANT: Does NOT apply to floating elements. Consider using LEAF_SIZE_FULL)
#define LEAF_SIZE_GROW                  (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_GROW }
#define LEAF_SIZE_GROW_MIN(v)           (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_GROW, .size.min_max.min = (v) }
#define LEAF_SIZE_GROW_MAX(v)           (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_GROW, .size.min_max.max = (v) }
#define LEAF_SIZE_GROW_MIN_MAX(mn, mx)  (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_GROW, .size.min_max.min = (mn), .size.min_max.max = (mx) }
#define LEAF_SIZE_FIT                   (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_FIT  }
#define LEAF_SIZE_FIT_MIN(v)            (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_FIT,     .size.min_max.min = (v) }
#define LEAF_SIZE_FIXED(v)              (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_FIXED,   .size.min_max.min = (v) }
#define LEAF_SIZE_PERCENT(v)            (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_PERCENT, .size.percent     = (v) }
    // Full size of the parent (Regardless of how much free space it has)
#define LEAF_SIZE_FULL                  (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_PERCENT, .size.percent     = (1.0f) }
#define LEAF_SIZE_PERCENT_MIN(p, m)     (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_PERCENT, .size.percent     = (p), .size.min_max.min = (m) }
#define LEAF_SIZE_PERCENT_MAX(p, m)     (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_PERCENT, .size.percent = (p), .size.min_max.max = (m) }
#define LEAF_SIZE_PERCENT_MIN_MAX(p, mn, mx) (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_PERCENT, .size.percent = (p), .size.min_max.min = (mn), .size.min_max.max = (mx) }
#define LEAF_SIZE_DERIVED               (Leaf_SizeAxis){ .type = LEAF_SIZE_TYPE_DERIVED }
    
    typedef struct { Leaf_ElementConfig wrapped; } Leaf_ElementConfigWrapper;
#define leaf(...) \
for (int leaf__i_ = (leaf_begin_element( \
(Leaf_ElementConfigWrapper){ __VA_ARGS__ }.wrapped), 1); \
leaf__i_; \
leaf__i_ = (leaf_end_element(), 0))
    
    LEAF_API Leaf_ID leaf_id(const char *label);
    LEAF_API Leaf_ID leaf_id_indexed(const char *label, uint64_t index);
    
    LEAF_API void leaf_init(void);
    LEAF_API void leaf_shutdown(void);
    
    LEAF_API void leaf_set_measure_text(Leaf_MeasureTextFn fn);
    LEAF_API void leaf_set_pointer_pos(float x, float y);
    
    LEAF_API void leaf_begin_element(Leaf_ElementConfig config);
    LEAF_API void leaf_end_element(void);
    
    LEAF_API bool leaf_hovered(Leaf_ID id);
    LEAF_API Leaf_BoundingBox leaf_get_bounding_box(Leaf_ID id);
    
    LEAF_API void leaf_begin_frame(int32_t width, int32_t height);
    LEAF_API Leaf_RenderCmdList leaf_end_frame(void);
    
    typedef struct { Leaf_TextConfig wrapped; } Leaf_TextConfigWrapper;
#define leaf_text(text, ...) \
__leaf_text(text, (Leaf_TextConfigWrapper){ __VA_ARGS__ }.wrapped)
    LEAF_API void __leaf_text(const char *text, Leaf_TextConfig config);
    
#ifdef LEAF_IMPLEMENTATION
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
    
    typedef uint8_t Leaf_NodeType;
    enum
    {
        LEAF_NODE_TYPE_ELEMENT,
        LEAF_NODE_TYPE_TEXT
    };
    
    typedef struct Leaf_Node Leaf_Node;
    struct Leaf_Node
    {
        union
        {
            struct
            {
                Leaf_ElementConfig config;
                void *custom_data;
                int32_t child_count;
                int32_t relative_child_count;
            }
            element;
            
            struct
            {
                Leaf_TextConfig config;
                const char *text;
                uint32_t size;
                float resolved_font_size;
            }
            text;
        };
        
        Leaf_NodeType type;
        Leaf_BoundingBox bounding_box;
        
        Leaf_Node *parent;
        Leaf_Node *first_child;
        Leaf_Node *last_child;
        Leaf_Node *next_sibling;
        
        float row_cross_offset;
        float row_cross_size;
    };
    
#define LEAF_ASSERT(x, msg) if (!(x)) {fprintf(stderr, "[Leaf]: %s", msg); return;}
#define LEAF_ASSERT_NULL(x, msg) if (!(x)) {fprintf(stderr, "[Leaf]: %s", msg); return NULL;}
    
#define LEAF_MAX(a, b) ((a) > (b) ? (a) : (b))
#define LEAF_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
    
#ifndef LEAF_CONFIG_MIN_MEMORY_SIZE
#define LEAF_CONFIG_MIN_MEMORY_SIZE (1 << 20)
#endif
    
#ifndef LEAF_CONFIG_MAX_STACK
#define LEAF_CONFIG_MAX_STACK 256
#endif
    
#ifndef LEAF_CONFIG_MAX_HASH_ENTRIES
#define LEAF_CONFIG_MAX_HASH_ENTRIES (1 << 14)
#endif
    
    typedef struct Leaf_ArenaChunk Leaf_ArenaChunk;
    struct Leaf_ArenaChunk
    {
        Leaf_ArenaChunk *next;
        size_t pos;
        size_t size;
        uint8_t data[];
    };
    
    typedef struct
    {
        Leaf_ArenaChunk *first;
        Leaf_ArenaChunk *current;
        size_t chunk_size;
    }
    Leaf_Arena;
    
    static inline Leaf_ArenaChunk *leaf_arena_new_chunk(size_t size)
    {
        Leaf_ArenaChunk *chunk = (Leaf_ArenaChunk*)malloc(sizeof(Leaf_ArenaChunk) + size);
        chunk->next = NULL;
        chunk->pos = 0;
        chunk->size = size;
        return chunk;
    }
    
    static inline void leaf_arena_init(Leaf_Arena *arena, size_t size)
    {
        arena->first = leaf_arena_new_chunk(size);
        arena->current = arena->first;
        arena->chunk_size = size;
    }
    
    static inline void leaf_arena_free(Leaf_Arena *arena)
    {
        Leaf_ArenaChunk *chunk = arena->first;
        while (chunk)
        {
            Leaf_ArenaChunk *next = chunk->next;
            free(chunk);
            chunk = next;
        }
    }
    
    static inline void leaf_arena_reset(Leaf_Arena *arena)
    {
        for (Leaf_ArenaChunk *chunk = arena->first; chunk; chunk = chunk->next)
            chunk->pos = 0;
        arena->current = arena->first;
    }
    
    static inline void *leaf_arena_alloc(Leaf_Arena *arena, size_t size)
    {
        Leaf_ArenaChunk *chunk = arena->current;
        
        if (chunk->pos + size > chunk->size)
        {
            if (chunk->next && size <= chunk->next->size)
            {
                chunk->next->pos = 0;
                arena->current = chunk->next;
            }
            else
            {
                size_t new_size = LEAF_MAX(arena->chunk_size, size);
                Leaf_ArenaChunk *new_chunk = leaf_arena_new_chunk(new_size);
                if (!new_chunk) return NULL;
                chunk->next = new_chunk;
                arena->current = new_chunk;
            }
            chunk = arena->current;
        }
        
        void *ptr = chunk->data + chunk->pos;
        chunk->pos += size;
        return ptr;
    }
    
    typedef struct
    {
        Leaf_LayoutFrameEntry layout_entires[LEAF_CONFIG_MAX_HASH_ENTRIES];
        
        Leaf_Node *stack[LEAF_CONFIG_MAX_STACK];
        uint32_t stack_top;
        
        Leaf_RenderCmdList render_cmds;
        Leaf_Arena arena;
        
        Leaf_Vec2 pointer_pos;
        Leaf_MeasureTextFn measure_text;
        
        uint32_t frame;
    }
    Leaf_Context;
    
    static Leaf_Context *leaf_ctx;
    
    static uint64_t leaf_murmur(const void *key, int len, uint64_t seed)
    {
        const uint8_t *data = (const uint8_t *)key;
        uint64_t h = seed;
        for (int i = 0; i < len; i++)
        {
            uint64_t k = data[i];
            k *= 0x87c37b91114253d5ULL; k = (k << 31) | (k >> 33); k *= 0x4cf5ad432745937fULL;
            h ^= k; h = (h << 27) | (h >> 37); h = h * 5 + 0x52dce729ULL;
        }
        h ^= (uint64_t)len;
        h ^= h >> 33; h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        return h ? h : 1;
    }
    
    Leaf_ID leaf_id(const char *label)
    {
        return (Leaf_ID){leaf_murmur(label, (int)strlen(label), 0), label};
    }
    
    Leaf_ID leaf_id_indexed(const char *label, uint64_t index)
    {
        uint64_t base = leaf_murmur(label, (int)strlen(label), 0);
        return (Leaf_ID){leaf_murmur(label, (int)strlen(label), base + index), label};
    }
    
    void leaf_init(void)
    {
        leaf_ctx = (Leaf_Context*)calloc(1, sizeof(Leaf_Context));
        leaf_arena_init(&leaf_ctx->arena, LEAF_CONFIG_MIN_MEMORY_SIZE);
    }
    
    void leaf_shutdown(void)
    {
        leaf_arena_free(&leaf_ctx->arena);
        free(leaf_ctx);
    }
    
    void leaf_set_measure_text(Leaf_MeasureTextFn fn)
    {
        leaf_ctx->measure_text = fn;
    }
    
    void leaf_set_pointer_pos(float x, float y)
    {
        leaf_ctx->pointer_pos.x = x;
        leaf_ctx->pointer_pos.y = y;
    }
    
    static void leaf_set_layout_entry(Leaf_ID id, Leaf_LayoutFrameEntry entry)
    {
        entry.id = id.value;
        entry.frame = leaf_ctx->frame;
        uint32_t i = id.value % LEAF_CONFIG_MAX_HASH_ENTRIES;
        while (leaf_ctx->layout_entires[i].id != 0 && leaf_ctx->layout_entires[i].id != id.value)
            i = (i + 1) % LEAF_CONFIG_MAX_HASH_ENTRIES;
        leaf_ctx->layout_entires[i] = entry;
    }
    
    static Leaf_LayoutFrameEntry leaf_get_layout_entry(Leaf_ID id)
    {
        uint32_t i = id.value % LEAF_CONFIG_MAX_HASH_ENTRIES;
        while (leaf_ctx->layout_entires[i].id != 0 && leaf_ctx->layout_entires[i].id != id.value)
            i = (i + 1) % LEAF_CONFIG_MAX_HASH_ENTRIES;
        
        Leaf_LayoutFrameEntry *entry = &leaf_ctx->layout_entires[i];
        if (entry->frame != leaf_ctx->frame - 1)
            return (Leaf_LayoutFrameEntry){0};
        return *entry;
    }
    
    static const char *leaf_cache_str(const char *src, uint32_t size)
    {
        char *dst = (char*)leaf_arena_alloc(&leaf_ctx->arena, size + 1);
        memcpy(dst, src, size);
        dst[size] = '\0';
        return dst;
    }
    
    static inline bool leaf_point_in_box(float px, float py, Leaf_BoundingBox bb)
    {
        return px > bb.x && px <= bb.x + bb.width &&
            py > bb.y && py <= bb.y + bb.height;
    }
    
    bool leaf_hovered(Leaf_ID id)
    {
        Leaf_BoundingBox bounding_box = leaf_get_layout_entry(id).bounding_box;
        return leaf_point_in_box(leaf_ctx->pointer_pos.x, leaf_ctx->pointer_pos.y, bounding_box);
    }
    
    Leaf_BoundingBox leaf_get_bounding_box(Leaf_ID id)
    {
        return leaf_get_layout_entry(id).bounding_box;
    }
    
    static inline Leaf_Node *leaf_alloc_node(void)
    {
        Leaf_Node *node = (Leaf_Node*)leaf_arena_alloc(&leaf_ctx->arena, sizeof(Leaf_Node));
        memset(node, 0, sizeof(Leaf_Node));
        return node;
    }
    
    static inline void leaf_stack_push(Leaf_Node *node)
    {
        LEAF_ASSERT(leaf_ctx->stack_top < LEAF_CONFIG_MAX_STACK,
                    "Stack limit exceeded. Increase LEAF_CONFIG_MAX_STACK.");
        leaf_ctx->stack[leaf_ctx->stack_top++] = node;
    }
    
    static inline void leaf_stack_pop(void)
    {
        leaf_ctx->stack_top--;
    }
    
    static inline Leaf_Node *leaf_stack_top(void)
    {
        return leaf_ctx->stack[leaf_ctx->stack_top - 1];
    }
    
    static inline void leaf_accumulate_fit(Leaf_Node *parent, Leaf_Node *child)
    {
        const Leaf_ElementConfig *cfg = &parent->element.config;
        bool fit_w = cfg->size.width.type  == LEAF_SIZE_TYPE_FIT;
        bool fit_h = cfg->size.height.type == LEAF_SIZE_TYPE_FIT;
        if (!fit_w && !fit_h) return;
        
        if (child->type == LEAF_NODE_TYPE_ELEMENT &&
            child->element.config.positioning != LEAF_POSITIONING_RELATIVE)
            return;
        
        float cw = child->bounding_box.width;
        float ch = child->bounding_box.height;
        
        if (fit_w)
        {
            if (cfg->direction == LEAF_DIRECTION_HORIZONTAL)
                parent->bounding_box.width += cw;
            else
                parent->bounding_box.width = LEAF_MAX(
                                                      cw + cfg->padding.left + cfg->padding.right,
                                                      parent->bounding_box.width);
        }
        if (fit_h)
        {
            if (cfg->direction == LEAF_DIRECTION_HORIZONTAL)
                parent->bounding_box.height = LEAF_MAX(
                                                       ch + cfg->padding.top + cfg->padding.bottom,
                                                       parent->bounding_box.height);
            else
                parent->bounding_box.height += ch;
        }
    }
    
    static inline void leaf_append_child(Leaf_Node *parent, Leaf_Node *child)
    {
        child->parent = parent;
        child->next_sibling = NULL;
        parent->element.child_count++;
        
        if (child->type == LEAF_NODE_TYPE_TEXT ||
            child->element.config.positioning == LEAF_POSITIONING_RELATIVE)
            parent->element.relative_child_count++;
        
        if (!parent->first_child)
        {
            parent->first_child = child;
            parent->last_child = child;
        }
        else
        {
            parent->last_child->next_sibling = child;
            parent->last_child = child;
        }
        
        leaf_accumulate_fit(parent, child);
    }
    
    static float leaf_resolve_font_size(const Leaf_SizeAxis *axis, Leaf_Node *parent, const char *text, uint32_t text_len, const Leaf_TextConfig *cfg)
    {
        switch (axis->type)
        {
            case LEAF_SIZE_TYPE_FIXED:
            return axis->size.min_max.min;
            
            case LEAF_SIZE_TYPE_FIT:
            default:
            return axis->size.min_max.min;
            
            case LEAF_SIZE_TYPE_PERCENT:
            case LEAF_SIZE_TYPE_GROW:
            {
                float parent_w = 0.0f;
                float parent_h = 0.0f;
                if (parent && parent->type == LEAF_NODE_TYPE_ELEMENT)
                {
                    const Leaf_ElementConfig *pc = &parent->element.config;
                    parent_w = parent->bounding_box.width  - pc->padding.left - pc->padding.right;
                    parent_h = parent->bounding_box.height - pc->padding.top  - pc->padding.bottom;
                    if (parent_w < 0.0f) parent_w = 0.0f;
                    if (parent_h < 0.0f) parent_h = 0.0f;
                }
                
                float parent_smaller = parent_w < parent_h ? parent_w : parent_h;
                float target = (axis->type == LEAF_SIZE_TYPE_PERCENT)
                    ? parent_smaller * axis->size.percent
                    : parent_smaller;
                
                float ref_size = 16.0f;
                Leaf_Dimensions ref_dim = leaf_ctx->measure_text(text, text_len, ref_size, cfg);
                float text_larger = ref_dim.width > ref_dim.height ? ref_dim.width : ref_dim.height;
                
                float resolved = (text_larger > 0.0f)
                    ? ref_size * (target / text_larger)
                    : target;
                
                if (axis->size.min_max.min > 0.0f && resolved < axis->size.min_max.min)
                    resolved = axis->size.min_max.min;
                if (axis->size.min_max.max > 0.0f && resolved > axis->size.min_max.max)
                    resolved = axis->size.min_max.max;
                return resolved;
            }
        }
    }
    
    void __leaf_text(const char *text, Leaf_TextConfig config)
    {
        Leaf_Node *parent = leaf_stack_top();
        Leaf_Node *node = leaf_alloc_node();
        
        uint32_t len = (uint32_t)strlen(text);
        
        node->type = LEAF_NODE_TYPE_TEXT;
        node->text.config = config;
        node->text.text = leaf_cache_str(text, len);
        node->text.size = len;
        node->text.resolved_font_size = leaf_resolve_font_size(&config.font_size, parent, text, node->text.size, &config);
        
        Leaf_Dimensions size = leaf_ctx->measure_text(text, node->text.size, node->text.resolved_font_size, &config);
        node->bounding_box.width  = size.width;
        node->bounding_box.height = size.height;
        
        leaf_append_child(parent, node);
    }
    
    void leaf_begin_element(Leaf_ElementConfig config)
    {
        Leaf_Node *node = leaf_alloc_node();
        
        node->type = LEAF_NODE_TYPE_ELEMENT;
        node->element.config = config;
        
        if (config.size.width.type == LEAF_SIZE_TYPE_FIXED)
            node->bounding_box.width = config.size.width.size.min_max.min;
        if (config.size.height.type == LEAF_SIZE_TYPE_FIXED)
            node->bounding_box.height = config.size.height.size.min_max.min;
        
        if (config.custom_draw_data.size)
        {
            node->element.custom_data = leaf_arena_alloc(&leaf_ctx->arena, config.custom_draw_data.size);
            memcpy(node->element.custom_data, config.custom_draw_data.data, config.custom_draw_data.size);
        }
        
        node->parent = leaf_stack_top();
        leaf_stack_push(node);
    }
    
    void leaf_end_element(void)
    {
        Leaf_Node *node = leaf_stack_top();
        const Leaf_ElementConfig *config = &node->element.config;
        
        if (config->size.width.type != LEAF_SIZE_TYPE_FIT)
            node->bounding_box.width += config->padding.left + config->padding.right;
        if (config->size.height.type != LEAF_SIZE_TYPE_FIT)
            node->bounding_box.height += config->padding.top + config->padding.bottom;
        
        const float child_gap = LEAF_MAX(node->element.relative_child_count - 1, 0) * config->child_gap;
        if (config->size.width.type == LEAF_SIZE_TYPE_FIT && config->direction == LEAF_DIRECTION_HORIZONTAL)
            node->bounding_box.width += child_gap;
        else if (config->size.height.type == LEAF_SIZE_TYPE_FIT && config->direction == LEAF_DIRECTION_VERTICAL)
            node->bounding_box.height += child_gap;
        
        if (node->parent)
            leaf_append_child(node->parent, node);
        
        leaf_stack_pop();
    }
    
#define LEAF_FOREACH_CHILD(x, _parent)\
for (Leaf_Node *x = _parent->first_child; x != NULL; x = x->next_sibling)
    
    static inline void leaf_push_render_cmd(Leaf_RenderCmd cmd)
    {
        Leaf_RenderCmdNode *node = (Leaf_RenderCmdNode*)leaf_arena_alloc(&leaf_ctx->arena, sizeof(Leaf_RenderCmdNode));
        if (!node)
            return;
        node->cmd = cmd;
        node->next = NULL;
        
        if (leaf_ctx->render_cmds.last)
            leaf_ctx->render_cmds.last->next = node;
        else
            leaf_ctx->render_cmds.first = node;
        
        leaf_ctx->render_cmds.last = node;
        leaf_ctx->render_cmds.count++;
    }
    
    static inline void leaf_element_clamp_min_max(Leaf_Node *node, const Leaf_ElementConfig *config)
    {
        float mn = config->size.width.size.min_max.min;
        float mx = config->size.width.size.min_max.max;
        if (mn > 0 && node->bounding_box.width < mn)
            node->bounding_box.width = mn;
        if (mx > 0 && node->bounding_box.width > mx)
            node->bounding_box.width = mx;
        
        mn = config->size.height.size.min_max.min;
        mx = config->size.height.size.min_max.max;
        if (mn > 0 && node->bounding_box.height < mn)
            node->bounding_box.height = mn;
        if (mx > 0 && node->bounding_box.height > mx)
            node->bounding_box.height = mx;
    }
    
    static inline bool leaf_is_color_fill_empty(Leaf_ColorFill fill)
    {
        return fill.color1.a == 0 && fill.color2.a == 0;
    }
    
    static inline bool leaf_box_offscreen(Leaf_BoundingBox box)
    {
        Leaf_BoundingBox screen = leaf_ctx->stack[0]->bounding_box;
        return box.x + box.width  <= screen.x ||
            box.y + box.height <= screen.y ||
            box.x >= screen.x + screen.width ||
            box.y >= screen.y + screen.height;
    }
    
    static inline float leaf_resolve_rounding(const Leaf_Node *node, const Leaf_ElementConfig *config)
    {
        float smallest_side = node->bounding_box.width < node->bounding_box.height
            ? node->bounding_box.width
            : node->bounding_box.height;
        float max_radius = smallest_side * 0.5f;
        
        float radius = (config->rounding.type == LEAF_ROUNDING_TYPE_FIXED)
            ? config->rounding.value
            : max_radius * config->rounding.value;
        
        return LEAF_CLAMP(radius, 0.0f, max_radius);
    }
    
    static void leaf_render_node(Leaf_Node *node)
    {
        if (node->bounding_box.width <= 0 ||
            node->bounding_box.height <= 0 ||
            leaf_box_offscreen(node->bounding_box))
            return;
        
        switch (node->type)
        {
            case LEAF_NODE_TYPE_ELEMENT:
            {
                const Leaf_ElementConfig *config = &node->element.config;
                
#ifdef LEAF_FEATURE_SHADOWS
                if (config->shadow.color.a)
                {
                    Leaf_BoundingBox shadow_box = {
                        .x = node->bounding_box.x,
                        .y = node->bounding_box.y,
                        .width  = node->bounding_box.width,
                        .height = node->bounding_box.height
                    };
                    leaf_push_render_cmd((Leaf_RenderCmd){
                                             .type = LEAF_RENDER_CMD_SHADOW,
                                             .color = (Leaf_ColorFill){
                                                 .color1 = config->shadow.color,
                                                 .type = LEAF_SOLID_COLOR_FILL
                                             },
                                             .bounding_box = shadow_box,
                                             .shadow.offset = config->shadow.offset,
                                             .shadow.blur_radius = config->shadow.blur_radius,
                                             .shadow.rounding = leaf_resolve_rounding(node, config),
                                             .shadow.rounding_corners = config->rounding.corners
                                         });
                }
#endif
                
                if (!leaf_is_color_fill_empty(config->color))
                {
                    if (config->image)
                    {
                        leaf_push_render_cmd((Leaf_RenderCmd){
                                                 .type = LEAF_RENDER_CMD_IMAGE,
                                                 .color = config->color,
                                                 .bounding_box = node->bounding_box,
                                                 .image.handle = config->image,
                                                 .image.rounding = leaf_resolve_rounding(node, config),
                                                 .image.rounding_corners = config->rounding.corners
                                             });
                    }
                    else
                    {
                        leaf_push_render_cmd((Leaf_RenderCmd){
                                                 .type = LEAF_RENDER_CMD_RECT,
                                                 .color = config->color,
                                                 .bounding_box = node->bounding_box,
                                                 .rect.rounding = leaf_resolve_rounding(node, config),
                                                 .rect.rounding_corners = config->rounding.corners
                                             });
                    }
                }
                
                if (config->custom_draw)
                {
                    leaf_push_render_cmd((Leaf_RenderCmd){
                                             .type = LEAF_RENDER_CMD_CUSTOM,
                                             .bounding_box = node->bounding_box,
                                             .custom.draw = config->custom_draw,
                                             .custom.user_data = node->element.custom_data
                                         });
                }
                
                if (!leaf_is_color_fill_empty(config->border.color))
                    leaf_push_render_cmd((Leaf_RenderCmd){
                                             .type = LEAF_RENDER_CMD_RECT_LINES,
                                             .color = config->border.color,
                                             .bounding_box = node->bounding_box,
                                             .rect.rounding = leaf_resolve_rounding(node, config),
                                             .rect.rounding_corners = config->rounding.corners,
                                             .rect.line_width = config->border.width,
                                             .rect.sides = config->border.sides
                                         });
                break;
            }
            case LEAF_NODE_TYPE_TEXT:
            {
                const Leaf_TextConfig *config = &node->text.config;
                if (!leaf_is_color_fill_empty(config->color))
                    leaf_push_render_cmd((Leaf_RenderCmd){
                                             .type = LEAF_RENDER_CMD_TEXT,
                                             .color = config->color,
                                             .bounding_box = node->bounding_box,
                                             .text.text = node->text.text,
                                             .text.font_size = node->text.resolved_font_size,
                                             .text.font_id = config->font_id
                                         });
                break;
            }
        }
    }
    
    static inline void leaf_char_from_utf8(const char *str, uint32_t i, int *out_cp, int *out_bytes)
    {
        unsigned char c = (unsigned char)str[i];
        if      (c < 0x80) { *out_cp = c;        *out_bytes = 1; }
        else if (c < 0xE0) { *out_cp = c & 0x1F; *out_bytes = 2; }
        else if (c < 0xF0) { *out_cp = c & 0x0F; *out_bytes = 3; }
        else               { *out_cp = c & 0x07; *out_bytes = 4; }
        for (int b = 1; b < *out_bytes; b++)
            *out_cp = (*out_cp << 6) | ((unsigned char)str[i + b] & 0x3F);
    }
    
    static Leaf_Node *leaf_wrap_text_node(Leaf_Node *parent, Leaf_Node *node, float avail_width)
    {
        const Leaf_TextConfig *cfg  = &node->text.config;
        const char *text = node->text.text;
        uint32_t len = node->text.size;
        float resolved_font_size = node->text.resolved_font_size;
        bool word_mode = cfg->wrap_mode == LEAF_TEXT_WRAP_MODE_WORD;
        
        Leaf_Node *last_inserted = node;
        uint32_t line_start = 0;
        
        while (line_start < len)
        {
            uint32_t line_end = line_start;
            uint32_t last_word_break = line_start;
            uint32_t i = line_start;
            
            while (i <= len)
            {
                int cp = 0, cp_bytes = 1;
                if (i < len)
                    leaf_char_from_utf8(text, i, &cp, &cp_bytes);
                
                if (word_mode && (i == len || cp == ' '))
                    last_word_break = i;
                
                Leaf_Dimensions d = leaf_ctx->measure_text(text + line_start, i - line_start, resolved_font_size, cfg);
                
                if (d.width > avail_width)
                {
                    if (word_mode && last_word_break > line_start)
                        line_end = last_word_break;
                    else
                        line_end = i;
                    break;
                }
                
                line_end = i;
                if (i >= len) break;
                i += cp_bytes;
            }
            
            uint32_t seg_start = line_start;
            if (word_mode && seg_start > 0 && text[seg_start] == ' ')
                seg_start++;
            
            uint32_t seg_len = (line_end > seg_start) ? line_end - seg_start : 0;
            const char *seg  = leaf_cache_str(text + seg_start, seg_len);
            if (!seg) return last_inserted;
            
            Leaf_Dimensions d = leaf_ctx->measure_text(seg, seg_len, resolved_font_size, cfg);
            
            if (line_start == 0)
            {
                node->text.text   = seg;
                node->text.size = seg_len;
                node->bounding_box.width  = d.width;
                node->bounding_box.height = d.height;
            }
            else
            {
                Leaf_Node *line_node = leaf_alloc_node();
                line_node->type = LEAF_NODE_TYPE_TEXT;
                line_node->text.config = *cfg;
                line_node->text.text   = seg;
                line_node->text.size = seg_len;
                line_node->text.resolved_font_size = resolved_font_size;
                line_node->bounding_box.width  = d.width;
                line_node->bounding_box.height = d.height;
                line_node->parent = parent;
                
                line_node->next_sibling = last_inserted->next_sibling;
                last_inserted->next_sibling = line_node;
                if (parent->last_child == last_inserted)
                    parent->last_child = line_node;
                parent->element.child_count++;
                parent->element.relative_child_count++;
                
                last_inserted = line_node;
            }
            
            uint32_t next_start = line_end;
            if (word_mode && next_start < len && text[next_start] == ' ')
                next_start++;
            
            if (next_start <= line_start)
            {
                int cp = 0, cp_bytes = 1;
                if (line_start < len)
                    leaf_char_from_utf8(text, line_start, &cp, &cp_bytes);
                next_start = line_start + cp_bytes;
            }
            
            if (next_start >= len) break;
            line_start = next_start;
        }
        
        return last_inserted;
    }
    
    
    static void leaf_wrap_text_children(Leaf_Node *parent)
    {
        if (parent->type != LEAF_NODE_TYPE_ELEMENT) return;
        
        const Leaf_ElementConfig *cfg = &parent->element.config;
        float avail = parent->bounding_box.width - cfg->padding.left - cfg->padding.right;
        
        Leaf_Node *child = parent->first_child;
        while (child)
        {
            if (child->type == LEAF_NODE_TYPE_TEXT &&
                child->text.config.wrap_mode != LEAF_TEXT_WRAP_MODE_NONE &&
                child->bounding_box.width > avail)
                child = leaf_wrap_text_node(parent, child, avail);
            
            child = child->next_sibling;
        }
    }
    
    static void leaf_resolve_aspect_ratio(Leaf_Node *node)
    {
        if (node->type != LEAF_NODE_TYPE_ELEMENT) return;
        const Leaf_ElementConfig *cfg = &node->element.config;
        if (cfg->aspect_ratio <= 0.0f) return;
        
        bool ar_w = cfg->size.width.type  == LEAF_SIZE_TYPE_DERIVED;
        bool ar_h = cfg->size.height.type == LEAF_SIZE_TYPE_DERIVED;
        
        if (ar_w && !ar_h)
            node->bounding_box.width  = node->bounding_box.height * cfg->aspect_ratio;
        else if (ar_h && !ar_w)
            node->bounding_box.height = node->bounding_box.width  / cfg->aspect_ratio;
    }
    
#define LEAF_MAIN(horizontal, a, b)   ((horizontal) ? (a) : (b))
#define LEAF_CROSS(horizontal, a, b)  ((horizontal) ? (b) : (a))
#define LEAF_ALIGN_OFFSET(align, end_val, center_val, free) \
((align) == (end_val)    ? (free) :                     \
(align) == (center_val) ? (free) * 0.5f : 0.0f)
    
    static void leaf_recompute_fit(Leaf_Node *parent)
    {
        if (parent->type != LEAF_NODE_TYPE_ELEMENT)
            return;
        
        const Leaf_ElementConfig *cfg = &parent->element.config;
        bool fit_w = cfg->size.width.type  == LEAF_SIZE_TYPE_FIT;
        bool fit_h = cfg->size.height.type == LEAF_SIZE_TYPE_FIT;
        if (!fit_w && !fit_h) return;
        
        bool h = cfg->direction == LEAF_DIRECTION_HORIZONTAL;
        
        if (fit_w) parent->bounding_box.width  = cfg->padding.left + cfg->padding.right;
        if (fit_h) parent->bounding_box.height = cfg->padding.top  + cfg->padding.bottom;
        
        if (cfg->wrap_children)
        {
            float avail_main =
                LEAF_MAIN(h, parent->bounding_box.width,  parent->bounding_box.height) -
                LEAF_MAIN(h, cfg->padding.left + cfg->padding.right, cfg->padding.top + cfg->padding.bottom);
            
            float cross_total = 0.0f;
            float row_main = 0.0f, row_cross = 0.0f;
            bool in_row = false;
            
            LEAF_FOREACH_CHILD(child, parent)
            {
                if (child->type == LEAF_NODE_TYPE_ELEMENT &&
                    child->element.config.positioning != LEAF_POSITIONING_RELATIVE)
                    continue;
                
                float child_main  = LEAF_MAIN (h, child->bounding_box.width, child->bounding_box.height);
                float child_cross = LEAF_CROSS(h, child->bounding_box.width, child->bounding_box.height);
                float gap = in_row ? cfg->child_gap : 0.0f;
                
                if (in_row && row_main + gap + child_main > avail_main)
                {
                    cross_total += row_cross + cfg->child_cross_gap;
                    row_main = 0.0f; row_cross = 0.0f;
                    in_row = false; gap = 0.0f;
                }
                
                row_main += gap + child_main;
                row_cross = LEAF_MAX(row_cross, child_cross);
                in_row = true;
            }
            if (in_row) cross_total += row_cross;
            
            cross_total += LEAF_CROSS(h, cfg->padding.left + cfg->padding.right,
                                      cfg->padding.top  + cfg->padding.bottom);
            
            if (fit_h) { if (h) parent->bounding_box.height = cross_total; else parent->bounding_box.width  = cross_total; }
            if (fit_w && !h)    parent->bounding_box.width  = cross_total;
        }
        else
        {
            int32_t relative_count = 0;
            
            LEAF_FOREACH_CHILD(child, parent)
            {
                if (child->type == LEAF_NODE_TYPE_ELEMENT &&
                    child->element.config.positioning != LEAF_POSITIONING_RELATIVE)
                    continue;
                
                relative_count++;
                float cw = child->bounding_box.width;
                float ch = child->bounding_box.height;
                
                if (fit_w)
                {
                    if (h) parent->bounding_box.width += cw;
                    else   parent->bounding_box.width = LEAF_MAX(cw + cfg->padding.left + cfg->padding.right, parent->bounding_box.width);
                }
                if (fit_h)
                {
                    if (h) parent->bounding_box.height = LEAF_MAX(ch + cfg->padding.top + cfg->padding.bottom, parent->bounding_box.height);
                    else   parent->bounding_box.height += ch;
                }
            }
            
            float gap = LEAF_MAX(relative_count - 1, 0) * cfg->child_gap;
            if (fit_w && h)  parent->bounding_box.width  += gap;
            if (fit_h && !h) parent->bounding_box.height += gap;
        }
    }

    static void leaf_apply_uniform_sizing(Leaf_Node *parent)
    {
        const Leaf_ElementConfig *cfg = &parent->element.config;
        if (cfg->uniform_children == LEAF_UNIFORM_SIZING_NONE)
            return;

        bool do_w = cfg->uniform_children == LEAF_UNIFORM_SIZING_WIDTH  || cfg->uniform_children == LEAF_UNIFORM_SIZING_BOTH;
        bool do_h = cfg->uniform_children == LEAF_UNIFORM_SIZING_HEIGHT || cfg->uniform_children == LEAF_UNIFORM_SIZING_BOTH;

        float max_w = 0.0f, max_h = 0.0f;

        LEAF_FOREACH_CHILD(child, parent)
        {
            if (child->type != LEAF_NODE_TYPE_ELEMENT) continue;
            if (child->element.config.positioning != LEAF_POSITIONING_RELATIVE) continue;
            if (child->element.config.size.width.type  == LEAF_SIZE_TYPE_GROW ||
                child->element.config.size.width.type  == LEAF_SIZE_TYPE_PERCENT) { /* skip from width vote, still gets stretched below */ }
            max_w = LEAF_MAX(max_w, child->bounding_box.width);
            max_h = LEAF_MAX(max_h, child->bounding_box.height);
        }

        LEAF_FOREACH_CHILD(child, parent)
        {
            if (child->type != LEAF_NODE_TYPE_ELEMENT) continue;
            if (child->element.config.positioning != LEAF_POSITIONING_RELATIVE) continue;
            const Leaf_ElementConfig *cc = &child->element.config;

            if (do_w && cc->size.width.type != LEAF_SIZE_TYPE_GROW && cc->size.width.type != LEAF_SIZE_TYPE_PERCENT)
            {
                child->bounding_box.width = max_w;
                leaf_element_clamp_min_max(child, cc);
            }
            if (do_h && cc->size.height.type != LEAF_SIZE_TYPE_GROW && cc->size.height.type != LEAF_SIZE_TYPE_PERCENT)
            {
                child->bounding_box.height = max_h;
                leaf_element_clamp_min_max(child, cc);
            }
        }

        // If the parent direction matches the unified axis and parent is FIT, its size
        // was summed from pre-unification children — recompute it from the new uniform width.
        if (do_w && cfg->size.width.type == LEAF_SIZE_TYPE_FIT)
        {
            int32_t rel = parent->element.relative_child_count;
            if (cfg->direction == LEAF_DIRECTION_HORIZONTAL)
                parent->bounding_box.width = max_w * rel + LEAF_MAX(rel - 1, 0) * cfg->child_gap
                                            + cfg->padding.left + cfg->padding.right;
            // vertical case: width already correctly == max_w from recompute_fit, nothing to do
        }
        if (do_h && cfg->size.height.type == LEAF_SIZE_TYPE_FIT)
        {
            int32_t rel = parent->element.relative_child_count;
            if (cfg->direction == LEAF_DIRECTION_VERTICAL)
                parent->bounding_box.height = max_h * rel + LEAF_MAX(rel - 1, 0) * cfg->child_gap
                                            + cfg->padding.top + cfg->padding.bottom;
        }
    }
    
    static void leaf_size_pass(Leaf_Node *parent)
    {
        if (parent->type != LEAF_NODE_TYPE_ELEMENT)
            return;
        
        const Leaf_ElementConfig *parent_config = &parent->element.config;
        
        leaf_resolve_aspect_ratio(parent);
        
        bool h = parent_config->direction == LEAF_DIRECTION_HORIZONTAL;
        
        int32_t growing_width_count  = 0;
        int32_t growing_height_count = 0;
        float   free_width  = parent->bounding_box.width;
        float   free_height = parent->bounding_box.height;
        
        const float child_gap = LEAF_MAX(parent->element.relative_child_count - 1, 0) * parent_config->child_gap;
        if (h)  free_width  -= child_gap;
        else    free_height -= child_gap;
        free_width  -= parent_config->padding.left + parent_config->padding.right;
        free_height -= parent_config->padding.top  + parent_config->padding.bottom;
        
        LEAF_FOREACH_CHILD(child, parent)
        {
            if (child->type != LEAF_NODE_TYPE_ELEMENT)
            {
                if (h) free_width  -= child->bounding_box.width;
                else   free_height -= child->bounding_box.height;
                continue;
            }
            
            const Leaf_ElementConfig *child_config = &child->element.config;
            
            if (child_config->size.width.type == LEAF_SIZE_TYPE_PERCENT)
            {
                float avail = parent->bounding_box.width - parent_config->padding.left - parent_config->padding.right;
                child->bounding_box.width = avail * child_config->size.width.size.percent;
            }
            if (child_config->size.height.type == LEAF_SIZE_TYPE_PERCENT)
            {
                float avail = parent->bounding_box.height - parent_config->padding.top - parent_config->padding.bottom;
                child->bounding_box.height = avail * child_config->size.height.size.percent;
            }
            
            leaf_resolve_aspect_ratio(child);
            
            if (child_config->positioning != LEAF_POSITIONING_RELATIVE)
                continue;
            
            if (child_config->size.width.type == LEAF_SIZE_TYPE_GROW)
                growing_width_count++;
            else if (h)
                free_width -= child->bounding_box.width;
            
            if (child_config->size.height.type == LEAF_SIZE_TYPE_GROW)
                growing_height_count++;
            else if (!h)
                free_height -= child->bounding_box.height;
        }
        
        if (growing_width_count  == 0) growing_width_count  = 1;
        if (growing_height_count == 0) growing_height_count = 1;
        
        LEAF_FOREACH_CHILD(child, parent)
        {
            if (child->type != LEAF_NODE_TYPE_ELEMENT)
                continue;
            
            const Leaf_ElementConfig *child_config = &child->element.config;
            if (child_config->positioning == LEAF_POSITIONING_RELATIVE) {
                if (child_config->size.width.type == LEAF_SIZE_TYPE_GROW) {
                    child->bounding_box.width = free_width;
                    if (h) child->bounding_box.width /= growing_width_count;
                }
                if (child_config->size.height.type == LEAF_SIZE_TYPE_GROW) {
                    child->bounding_box.height = free_height;
                    if (!h) child->bounding_box.height /= growing_height_count;
                }
            }
            
            leaf_element_clamp_min_max(child, child_config);
        }
        
        LEAF_FOREACH_CHILD(child, parent)
        {
            if (child->type == LEAF_NODE_TYPE_TEXT)
            {
                float old_resolved = child->text.resolved_font_size;
                float new_resolved = leaf_resolve_font_size(&child->text.config.font_size, parent, child->text.text, child->text.size, &child->text.config);
                if (new_resolved != old_resolved)
                {
                    child->text.resolved_font_size = new_resolved;
                    Leaf_Dimensions d = leaf_ctx->measure_text(
                                                               child->text.text, child->text.size,
                                                               new_resolved, &child->text.config);
                    child->bounding_box.width  = d.width;
                    child->bounding_box.height = d.height;
                }
            }
            leaf_size_pass(child);
        }
        
        leaf_wrap_text_children(parent);
        leaf_resolve_aspect_ratio(parent);
        leaf_recompute_fit(parent);
        leaf_apply_uniform_sizing(parent);
    }
    
    static void leaf_assign_wrap_offsets(Leaf_Node *parent)
    {
        const Leaf_ElementConfig *cfg = &parent->element.config;
        bool h = cfg->direction == LEAF_DIRECTION_HORIZONTAL;
        
        float avail_main =
            LEAF_MAIN(h, parent->bounding_box.width,  parent->bounding_box.height) -
            LEAF_MAIN(h, cfg->padding.left + cfg->padding.right, cfg->padding.top  + cfg->padding.bottom);
        float avail_cross =
            LEAF_CROSS(h, parent->bounding_box.width,  parent->bounding_box.height) -
            LEAF_CROSS(h, cfg->padding.left + cfg->padding.right, cfg->padding.top  + cfg->padding.bottom);
        
        Leaf_LayoutAlignmentY cross_align = LEAF_CROSS(h, cfg->child_alignment.x, cfg->child_alignment.y);
        bool reverse = (cross_align == LEAF_ALIGN_Y_BOTTOM);
        
        float total_cross = 0.0f;
        {
            float row_main = 0.0f, row_cross = 0.0f;
            bool in_row = false;
            
            LEAF_FOREACH_CHILD(child, parent)
            {
                if (child->type == LEAF_NODE_TYPE_ELEMENT &&
                    child->element.config.positioning != LEAF_POSITIONING_RELATIVE)
                    continue;
                
                float child_main  = LEAF_MAIN (h, child->bounding_box.width, child->bounding_box.height);
                float child_cross = LEAF_CROSS(h, child->bounding_box.width, child->bounding_box.height);
                float gap = in_row ? cfg->child_gap : 0.0f;
                
                if (in_row && row_main + gap + child_main > avail_main)
                {
                    total_cross += row_cross + cfg->child_cross_gap;
                    row_main = 0.0f;
                    row_cross = 0.0f;
                    in_row = false;
                    gap = 0.0f;
                }
                
                row_main += gap + child_main;
                row_cross = LEAF_MAX(row_cross, child_cross);
                in_row = true;
            }
            if (in_row) total_cross += row_cross;
        }
        
        float free_cross_total = avail_cross - total_cross;
        float start_offset = LEAF_ALIGN_OFFSET(cross_align, LEAF_ALIGN_Y_BOTTOM, LEAF_ALIGN_Y_CENTER, free_cross_total);
        
        float cross_cursor = 0.0f;
        float row_main = 0.0f;
        float row_cross = 0.0f;
        Leaf_Node *row_start = NULL;
        
        LEAF_FOREACH_CHILD(child, parent)
        {
            if (child->type == LEAF_NODE_TYPE_ELEMENT &&
                child->element.config.positioning != LEAF_POSITIONING_RELATIVE)
                continue;
            
            float child_main = LEAF_MAIN (h, child->bounding_box.width, child->bounding_box.height);
            float child_cross = LEAF_CROSS(h, child->bounding_box.width, child->bounding_box.height);
            float gap = row_start ? cfg->child_gap : 0.0f;
            
            if (row_start && row_main + gap + child_main > avail_main)
            {
                float row_offset = reverse
                    ? (total_cross - cross_cursor - row_cross)
                    : cross_cursor;
                row_offset += start_offset;
                
                for (Leaf_Node *n = row_start; n != child; n = n->next_sibling)
                {
                    if (n->type == LEAF_NODE_TYPE_TEXT ||
                        n->element.config.positioning == LEAF_POSITIONING_RELATIVE)
                    {
                        n->row_cross_offset = row_offset;
                        n->row_cross_size   = row_cross;
                    }
                }
                
                cross_cursor += row_cross + cfg->child_cross_gap;
                row_main = 0.0f;
                row_cross = 0.0f;
                row_start = NULL;
                gap = 0.0f;
            }
            
            if (!row_start) row_start = child;
            row_main += gap + child_main;
            row_cross = LEAF_MAX(row_cross, child_cross);
        }
        
        if (row_start)
        {
            float row_offset = reverse
                ? (total_cross - cross_cursor - row_cross)
                : cross_cursor;
            row_offset += start_offset;
            
            for (Leaf_Node *n = row_start; n != NULL; n = n->next_sibling)
            {
                if (n->type == LEAF_NODE_TYPE_TEXT ||
                    n->element.config.positioning == LEAF_POSITIONING_RELATIVE)
                {
                    n->row_cross_offset = row_offset;
                    n->row_cross_size = row_cross;
                }
            }
        }
    }
    
    static void leaf_position_render(Leaf_Node *parent)
    {
        if (parent->type != LEAF_NODE_TYPE_ELEMENT)
            return;
        
        const Leaf_ElementConfig *parent_config = &parent->element.config;
        bool h = parent_config->direction == LEAF_DIRECTION_HORIZONTAL;
        
        float available_main =
            LEAF_MAIN (h, parent->bounding_box.width,  parent->bounding_box.height) -
            LEAF_MAIN (h, parent_config->padding.left + parent_config->padding.right,
                       parent_config->padding.top  + parent_config->padding.bottom);
        float available_cross =
            LEAF_CROSS(h, parent->bounding_box.width,  parent->bounding_box.height) -
            LEAF_CROSS(h, parent_config->padding.left + parent_config->padding.right,
                       parent_config->padding.top  + parent_config->padding.bottom);
        
        if (parent_config->wrap_children)
            leaf_assign_wrap_offsets(parent);
        
        float children_total = 0;
        if (!parent_config->wrap_children)
        {
            LEAF_FOREACH_CHILD(child, parent)
            {
                if (child->type == LEAF_NODE_TYPE_ELEMENT &&
                    child->element.config.positioning != LEAF_POSITIONING_RELATIVE)
                    continue;
                children_total += LEAF_MAIN(h, child->bounding_box.width, child->bounding_box.height);
            }
            children_total += LEAF_MAX(parent->element.relative_child_count - 1, 0) * parent_config->child_gap;
        }
        
        float free_main = available_main - children_total;
        
        float main_offset =
            LEAF_MAIN(h, parent_config->padding.left, parent_config->padding.top) +
            LEAF_ALIGN_OFFSET(
                              LEAF_MAIN(h, parent_config->child_alignment.x, parent_config->child_alignment.y),
                              LEAF_MAIN(h, LEAF_ALIGN_X_RIGHT,   LEAF_ALIGN_Y_BOTTOM),
                              LEAF_MAIN(h, LEAF_ALIGN_X_CENTER,  LEAF_ALIGN_Y_CENTER),
                              free_main);
        
        float prev_row_cross = -1.0f;
        
        LEAF_FOREACH_CHILD(child, parent)
        {
            if (child->type == LEAF_NODE_TYPE_ELEMENT)
            {
                const Leaf_ElementConfig *child_config = &child->element.config;
                
                if (child_config->positioning != LEAF_POSITIONING_RELATIVE)
                {
                    Leaf_BoundingBox anchor;
                    if (child_config->positioning == LEAF_POSITIONING_FLOATING_TO_PARENT)
                        anchor = parent->bounding_box;
                    else
                        anchor = leaf_ctx->stack[0]->bounding_box;
                    
                    float anchor_x = anchor.x, anchor_y = anchor.y;
                    if (child_config->floating.parent_alignment.x == LEAF_ALIGN_X_RIGHT)
                        anchor_x += anchor.width;
                    else if (child_config->floating.parent_alignment.x == LEAF_ALIGN_X_CENTER)
                        anchor_x += anchor.width * 0.5f;
                    
                    if (child_config->floating.parent_alignment.y == LEAF_ALIGN_Y_BOTTOM)
                        anchor_y += anchor.height;
                    else if (child_config->floating.parent_alignment.y == LEAF_ALIGN_Y_CENTER)
                        anchor_y += anchor.height * 0.5f;
                    
                    float elem_dx = 0.0f, elem_dy = 0.0f;
                    if (child_config->floating.self_alignment.x == LEAF_ALIGN_X_RIGHT)
                        elem_dx = child->bounding_box.width;
                    else if (child_config->floating.self_alignment.x == LEAF_ALIGN_X_CENTER)
                        elem_dx = child->bounding_box.width * 0.5f;
                    
                    if (child_config->floating.self_alignment.y == LEAF_ALIGN_Y_BOTTOM)
                        elem_dy = child->bounding_box.height;
                    else if (child_config->floating.self_alignment.y == LEAF_ALIGN_Y_CENTER)
                        elem_dy = child->bounding_box.height * 0.5f;
                    
                    child->bounding_box.x = anchor_x - elem_dx + child_config->floating.offset.x;
                    child->bounding_box.y = anchor_y - elem_dy + child_config->floating.offset.y;
                    
                    leaf_render_node(child);
                    if (child_config->clip_children)
                        leaf_push_render_cmd((Leaf_RenderCmd){ .type = LEAF_RENDER_CMD_SCISSOR_PUSH, .bounding_box = child->bounding_box });
                    leaf_position_render(child);
                    if (child_config->clip_children)
                        leaf_push_render_cmd((Leaf_RenderCmd){ .type = LEAF_RENDER_CMD_SCISSOR_POP });
                    continue;
                }
            }
            
            if (parent_config->wrap_children)
            {
                if (child->row_cross_offset != prev_row_cross)
                {
                    prev_row_cross = child->row_cross_offset;
                    
                    float row_main = 0.0f;
                    int32_t row_child_count = 0;
                    for (Leaf_Node *n = child; n != NULL; n = n->next_sibling)
                    {
                        if (n->type == LEAF_NODE_TYPE_ELEMENT &&
                            n->element.config.positioning != LEAF_POSITIONING_RELATIVE)
                            continue;
                        if (n->row_cross_offset != prev_row_cross)
                            break;
                        row_main += LEAF_MAIN(h, n->bounding_box.width, n->bounding_box.height);
                        row_child_count++;
                    }
                    row_main += LEAF_MAX(row_child_count - 1, 0) * parent_config->child_gap;
                    
                    float row_free_main = available_main - row_main;
                    main_offset =
                        LEAF_MAIN(h, parent_config->padding.left, parent_config->padding.top) +
                        LEAF_ALIGN_OFFSET(
                                          LEAF_MAIN(h, parent_config->child_alignment.x, parent_config->child_alignment.y),
                                          LEAF_MAIN(h, LEAF_ALIGN_X_RIGHT,  LEAF_ALIGN_Y_BOTTOM),
                                          LEAF_MAIN(h, LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER),
                                          row_free_main);
                }
                
                float free_cross = child->row_cross_size - LEAF_CROSS(h, child->bounding_box.width, child->bounding_box.height);
                float cross_base =
                    LEAF_CROSS(h, parent_config->padding.left, parent_config->padding.top) +
                    child->row_cross_offset +
                    LEAF_ALIGN_OFFSET(
                                      LEAF_CROSS(h, parent_config->child_alignment.x, parent_config->child_alignment.y),
                                      LEAF_CROSS(h, LEAF_ALIGN_X_RIGHT,  LEAF_ALIGN_Y_BOTTOM),
                                      LEAF_CROSS(h, LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER),
                                      free_cross);
                
                child->bounding_box.x = parent->bounding_box.x +
                    LEAF_MAIN (h, main_offset, cross_base) -
                    parent_config->child_offset.x;
                child->bounding_box.y = parent->bounding_box.y +
                    LEAF_CROSS(h, main_offset, cross_base) -
                    parent_config->child_offset.y;
            }
            else
            {
                float child_cross = LEAF_CROSS(h, child->bounding_box.width, child->bounding_box.height);
                float free_cross  = available_cross - child_cross;
                float cross_offset =
                    LEAF_CROSS(h, parent_config->padding.left, parent_config->padding.top) +
                    LEAF_ALIGN_OFFSET(
                                      LEAF_CROSS(h, parent_config->child_alignment.x, parent_config->child_alignment.y),
                                      LEAF_CROSS(h, LEAF_ALIGN_X_RIGHT,  LEAF_ALIGN_Y_BOTTOM),
                                      LEAF_CROSS(h, LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER),
                                      free_cross);
                
                child->bounding_box.x = parent->bounding_box.x + LEAF_MAIN (h, main_offset,  cross_offset) - parent_config->child_offset.x;
                child->bounding_box.y = parent->bounding_box.y + LEAF_CROSS(h, main_offset,  cross_offset) - parent_config->child_offset.y;
            }
            
            leaf_render_node(child);
            
            main_offset += LEAF_MAIN(h, child->bounding_box.width, child->bounding_box.height) + parent_config->child_gap;
            
            if (child->type == LEAF_NODE_TYPE_ELEMENT)
            {
                const Leaf_ElementConfig *child_config = &child->element.config;
                if (child_config->clip_children)
                    leaf_push_render_cmd((Leaf_RenderCmd){ .type = LEAF_RENDER_CMD_SCISSOR_PUSH, .bounding_box = child->bounding_box });
                leaf_position_render(child);
                if (child_config->clip_children)
                    leaf_push_render_cmd((Leaf_RenderCmd){ .type = LEAF_RENDER_CMD_SCISSOR_POP });
            }
        }
        
        Leaf_ID id = parent_config->id;
        if (id.value)
            leaf_set_layout_entry(id, (Leaf_LayoutFrameEntry){ parent->bounding_box });
    }
    
#undef LEAF_MAIN
#undef LEAF_CROSS
#undef LEAF_ALIGN_OFFSET
    
    void leaf_begin_frame(int32_t width, int32_t height)
    {
        leaf_arena_reset(&leaf_ctx->arena);
        leaf_ctx->stack_top = 0;
        leaf_ctx->frame++;
        
        Leaf_Node *root = leaf_alloc_node();
        root->element.config.size.width.type = LEAF_SIZE_TYPE_FIXED;
        root->element.config.size.height.type = LEAF_SIZE_TYPE_FIXED;
        root->bounding_box.width = (float)width;
        root->bounding_box.height = (float)height;
        leaf_stack_push(root);
    }
    
    Leaf_RenderCmdList leaf_end_frame(void)
    {
        Leaf_Node *root = leaf_stack_top();
        
        leaf_ctx->render_cmds = (Leaf_RenderCmdList){0};
        
        leaf_size_pass(root);
        leaf_position_render(root);
        
        return leaf_ctx->render_cmds;
    }
    
#endif // LEAF_IMPLEMENTATION
    
#ifdef __cplusplus
}
#endif
