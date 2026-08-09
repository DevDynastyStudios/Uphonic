#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef MGFX_ALL
#ifndef MGFX_VULKAN
#define MGFX_VULKAN 1
#endif
#ifndef MGFX_OPENGL
#define MGFX_OPENGL 1
#endif
#if !defined(MGFX_D3D11) && defined(_WIN32)
#define MGFX_D3D11 1
#endif
#if !defined(MGFX_METAL) && defined(__APPLE__)
#define MGFX_METAL 1
#endif
#endif

#ifndef MGFX_MAX_BINDABLE_IMAGES
#define MGFX_MAX_BINDABLE_IMAGES 8
#endif

#ifndef MGFX_MAX_BINDABLE_UNIFORMS
#define MGFX_MAX_BINDABLE_UNIFORMS 4
#endif

#ifndef MGFX_MAX_BINDABLE_STORAGE_BUFFERS
#define MGFX_MAX_BINDABLE_STORAGE_BUFFERS 4
#endif

#ifndef MGFX_MAX_COLOR_ATTACHMENTS
#define MGFX_MAX_COLOR_ATTACHMENTS 8
#endif

#ifndef MGFX_MAX_VERTEX_ATTRIBUTES
#define MGFX_MAX_VERTEX_ATTRIBUTES 8
#endif

#ifndef MGFX_MAX_DESCRIPTOR_CACHE
#define MGFX_MAX_DESCRIPTOR_CACHE (1 << 10)
#endif

#ifndef MGFX_MAX_DEVICE_ALLOCATIONS
#define MGFX_MAX_DEVICE_ALLOCATIONS (1 << 13)
#endif

#ifndef MGFX_MAX_UNIFORM_UPDATE_SIZE
#define MGFX_MAX_UNIFORM_UPDATE_SIZE (1 << 16)
#endif

#ifndef MGFX_MAX_SCRATCH_BUFFER_SIZE
#define MGFX_MAX_SCRATCH_BUFFER_SIZE (1 << 22)
#endif

#ifndef MGFX_API
#if defined(_WIN32) && defined(MG_DLL) && defined(MGFX_IMPL)
#define MGFX_API __declspec(dllexport)
#elif defined(_WIN32) && defined(MG_DLL)
#define MGFX_API __declspec(dllimport)
#else
#define MGFX_API extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef uint32_t mgfx_renderer_type;
    enum
    {
#if defined(MGFX_VULKAN)
        MGFX_RENDERER_VULKAN,
#endif
#if defined(MGFX_D3D11)
        MGFX_RENDERER_D3D11,
#endif
#if defined(MGFX_OPENGL)
#if !defined(__EMSCRIPTEN__)
        MGFX_RENDERER_OPENGL,
#else
        MGFX_RENDERER_OPENGLES
#endif
#endif
    };
    
    typedef uint32_t mgfx_shader_lang;
    enum
    {
        MGFX_SHADER_LANG_GLSL,
        MGFX_SHADER_LANG_GLSLES,
        MGFX_SHADER_LANG_HLSL,
        MGFX_SHADER_LANG_SPIRV,
        MGFX_SHADER_LANG_MSL
    };
    
    typedef uint32_t mgfx_index_type;
    enum
    {
        MGFX_INDEX_TYPE_UINT16,
        MGFX_INDEX_TYPE_UINT32
    };
    
    typedef uint32_t mgfx_buffer_usage;
    enum
    {
        MGFX_BUFFER_USAGE_INDEX,
        MGFX_BUFFER_USAGE_VERTEX
    };
    
    typedef uint32_t mgfx_access;
    enum
    {
        MGFX_ACCESS_GPU,
        MGFX_ACCESS_CPU
    };
    
    typedef struct
    {
        void *data;
        size_t size;
        mgfx_buffer_usage usage;
        mgfx_access access;
    }
    mgfx_buffer_create_info;
    
    typedef void *mgfx_buffer;
    
    typedef uint32_t mgfx_format;
    enum
    {
        MGFX_FORMAT_NONE,
        
        MGFX_FORMAT_R8_UNORM,
        MGFX_FORMAT_RG8_UNORM,
        MGFX_FORMAT_RGB8_UNORM,
        MGFX_FORMAT_RGB8_SRGB,
        MGFX_FORMAT_RGBA8_UNORM,
        MGFX_FORMAT_RGBA8_SRGB,
        
        MGFX_FORMAT_BGRA8_UNORM,
        MGFX_FORMAT_BGRA8_SRGB,
        
        MGFX_FORMAT_R16_SFLOAT,
        MGFX_FORMAT_RG16_SFLOAT,
        MGFX_FORMAT_RGB16_SFLOAT,
        MGFX_FORMAT_RGBA16_SFLOAT,
        
        MGFX_FORMAT_R32_SFLOAT,
        MGFX_FORMAT_RG32_SFLOAT,
        MGFX_FORMAT_RGBA32_SFLOAT,
        
        MGFX_FORMAT_R32_UINT,
        MGFX_FORMAT_R32_SINT,
        MGFX_FORMAT_RGB32_SFLOAT,
        
        MGFX_FORMAT_D16_UNORM_S8_UINT,
        MGFX_FORMAT_D24_UNORM_S8_UINT,
        MGFX_FORMAT_D32_SFLOAT_S8_UINT
    };
    
    typedef uint32_t mgfx_image_type;
    enum
    {
        MGFX_IMAGE_TYPE_2D,
        MGFX_IMAGE_TYPE_3D,
        MGFX_IMAGE_TYPE_CUBE
    };
    
    typedef uint32_t mgfx_image_usage;
    enum
    {
        MGFX_IMAGE_USAGE_NONE,
        MGFX_IMAGE_USAGE_COLOR_ATTACHMENT,
        MGFX_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT
    };
    
    typedef uint32_t mgfx_sampler_filter;
    enum
    {
        MGFX_SAMPLER_FILTER_NEAREST,
        MGFX_SAMPLER_FILTER_LINEAR
    };
    
    typedef uint32_t mgfx_sampler_address_mode;
    enum
    {
        MGFX_SAMPLER_ADDRESS_MODE_REPEAT,
        MGFX_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
    };
    
    typedef struct
    {
        mgfx_sampler_filter mag_filter;
        mgfx_sampler_filter min_filter;
        
        mgfx_sampler_address_mode address_mode_u;
        mgfx_sampler_address_mode address_mode_v;
        mgfx_sampler_address_mode address_mode_w;
    }
    mgfx_sampler_create_info;
    
    typedef void *mgfx_sampler;
    
    typedef struct
    {
        void *data;
        mgfx_format format;
        mgfx_image_type type;
        mgfx_image_usage usage;
        mgfx_access access;
        uint32_t width, height, depth;
    }
    mgfx_image_create_info;
    
    typedef void *mgfx_image;
    
    typedef struct
    {
        float r, g, b, a;
    }
    mgfx_color;
    
    typedef struct
    {
        mgfx_image color_images[MGFX_MAX_COLOR_ATTACHMENTS];
        mgfx_image depth_stencil_image;
        mgfx_color clear;
    }
    mgfx_pass_info;
    
    typedef uint32_t mgfx_primitive_topology;
    enum
    {
        MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        MGFX_PRIMITIVE_TOPOLOGY_LINE_LIST,
        MGFX_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        MGFX_PRIMITIVE_TOPOLOGY_POINT_LIST
    };
    
    typedef uint32_t mgfx_cull_mode;
    enum
    {
        MGFX_CULL_MODE_NONE,
        MGFX_CULL_MODE_FRONT,
        MGFX_CULL_MODE_BACK
    };
    
    typedef uint32_t mgfx_front_face;
    enum
    {
        MGFX_FRONT_FACE_CCW,
        MGFX_FRONT_FACE_CW
    };
    
    typedef uint32_t mgfx_blend_factor;
    enum
    {
        MGFX_BLEND_FACTOR_ZERO,
        MGFX_BLEND_FACTOR_ONE,
        MGFX_BLEND_FACTOR_SRC_COLOR,
        MGFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        MGFX_BLEND_FACTOR_DST_COLOR,
        MGFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        MGFX_BLEND_FACTOR_SRC_ALPHA,
        MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        MGFX_BLEND_FACTOR_DST_ALPHA,
        MGFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        MGFX_BLEND_FACTOR_CONSTANT_COLOR,
        MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        MGFX_BLEND_FACTOR_CONSTANT_ALPHA,
        MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        MGFX_BLEND_FACTOR_SRC_ALPHA_SATURATE,
        MGFX_BLEND_FACTOR_SRC1_COLOR,
        MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        MGFX_BLEND_FACTOR_SRC1_ALPHA,
        MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
    };
    
    typedef uint32_t mgfx_blend_op;
    enum
    {
        MGFX_BLEND_OP_ADD,
        MGFX_BLEND_OP_SUBTRACT,
        MGFX_BLEND_OP_REVERSE_SUBTRACT,
        MGFX_BLEND_OP_MIN,
        MGFX_BLEND_OP_MAX,
        MGFX_BLEND_OP_ZERO,
        MGFX_BLEND_OP_SRC,
        MGFX_BLEND_OP_DST,
        MGFX_BLEND_OP_INVERT,
        MGFX_BLEND_OP_RED
    };
    
    typedef struct
    {
        mgfx_blend_factor src_color_blend_factor;
        mgfx_blend_factor dst_color_blend_factor;
        mgfx_blend_op color_blend_op;
        
        mgfx_blend_factor src_alpha_blend_factor;
        mgfx_blend_factor dst_alpha_blend_factor;
        mgfx_blend_op alpha_blend_op;
        
        bool blend_enabled;
    }
    mgfx_color_blend;
    
    typedef uint32_t mgfx_compare_op;
    enum
    {
        MGFX_COMPARE_OP_NEVER,
        MGFX_COMPARE_OP_LESS,
        MGFX_COMPARE_OP_EQUAL,
        MGFX_COMPARE_OP_LESS_OR_EQUAL,
        MGFX_COMPARE_OP_GREATER,
        MGFX_COMPARE_OP_NOT_EQUAL,
        MGFX_COMPARE_OP_GREATER_OR_EQUAL,
        MGFX_COMPARE_OP_ALWAYS
    };
    
    typedef struct
    {
        mgfx_compare_op depth_compare_op;
        mgfx_format format;
        bool depth_write_enabled;
        bool stencil_test_enabled;
    }
    mgfx_depth_stencil_state;
    
    typedef uint32_t mgfx_vertex_format;
    enum
    {
        MGFX_VERTEX_FORMAT_NONE,
        
        MGFX_VERTEX_FORMAT_UINT,
        MGFX_VERTEX_FORMAT_INT,
        MGFX_VERTEX_FORMAT_FLOAT,
        
        MGFX_VERTEX_FORMAT_UINT2,
        MGFX_VERTEX_FORMAT_INT2,
        MGFX_VERTEX_FORMAT_FLOAT2,
        
        MGFX_VERTEX_FORMAT_UINT3,
        MGFX_VERTEX_FORMAT_INT3,
        MGFX_VERTEX_FORMAT_FLOAT3,
        
        MGFX_VERTEX_FORMAT_UINT4,
        MGFX_VERTEX_FORMAT_INT4,
        MGFX_VERTEX_FORMAT_FLOAT4,
        
        MGFX_VERTEX_FORMAT_UBYTE4,
        MGFX_VERTEX_FORMAT_BYTE4,
        
        MGFX_VERTEX_FORMAT_UBYTE4N,
        MGFX_VERTEX_FORMAT_BYTE4N
    };
    
    typedef struct
    {
        void *code;
        size_t size;
    }
    mgfx_shader_source;
    
    typedef struct
    {
        struct
        {
            const char *name;
            int32_t binding;
            uint32_t size;
        }
        uniform_blocks[MGFX_MAX_BINDABLE_UNIFORMS];
        
        struct
        {
            const char *name;
            int32_t binding;
        }
        sampled_images[MGFX_MAX_BINDABLE_IMAGES];
        
        mgfx_shader_source vertex;
        mgfx_shader_source fragment;
        mgfx_shader_source compute;
    }
    mgfx_shader;
    
    typedef struct
    {
        mgfx_shader shader;
        mgfx_vertex_format vertex_attributes[MGFX_MAX_VERTEX_ATTRIBUTES];
        
        mgfx_primitive_topology primitive_topology;
        mgfx_cull_mode cull_mode;
        mgfx_front_face front_face;
        mgfx_color_blend color_blend;
        mgfx_depth_stencil_state depth_stencil;
        mgfx_format color_formats[MGFX_MAX_COLOR_ATTACHMENTS];
    }
    mgfx_pipeline_create_info;
    
    typedef void *mgfx_pipeline;
    
    typedef union
    {
        struct
        {
            void *hwnd;
            void *instance;
        }
        win32;
        
        struct
        {
            void *window;
            void *display;
        }
        xlib;
    }
    mgfx_platform_handle;
    typedef void* mgfx_platform_handle_ptr;
    
    typedef struct
    {
        mgfx_platform_handle_ptr handle;
        mgfx_renderer_type type;
        int32_t width, height;
        bool vsync;
    }
    mgfx_init_info;
    
    typedef uint8_t mgfx_result;
    enum
    {
        MGFX_RESULT_SUCCESS,
        MGFX_RESULT_FAILURE
    };
    
    MGFX_API void mgfx_init (const mgfx_init_info *init_info);
    MGFX_API void mgfx_shutdown (void);
    
    MGFX_API mgfx_result mgfx_begin (void);
    MGFX_API void mgfx_end (void);
    
    MGFX_API void mgfx_viewport (int32_t x, int32_t y, uint32_t width, uint32_t height);
    MGFX_API void mgfx_scissor (int32_t x, int32_t y, uint32_t width, uint32_t height);
    
    MGFX_API void mgfx_draw (uint32_t vertex_count, uint32_t first_vertex);
    MGFX_API void mgfx_draw_indexed (uint32_t index_count, uint32_t first_index, int32_t vertex_offset);
    MGFX_API void mgfx_draw_instanced (uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance);
    MGFX_API void mgfx_draw_indexed_instanced (uint32_t index_count, uint32_t first_index, int32_t vertex_offset, uint32_t instance_count, uint32_t first_instance);
    MGFX_API void mgfx_dispatch (uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
    
    MGFX_API void mgfx_resize (uint32_t width, uint32_t height);
    MGFX_API void mgfx_vsync (bool vsync);
    
    MGFX_API void mgfx_bind_pass(const mgfx_pass_info *pass);
    
    MGFX_API mgfx_pipeline mgfx_create_pipeline (const mgfx_pipeline_create_info *create_info);
    MGFX_API void mgfx_destroy_pipeline (mgfx_pipeline pipeline);
    MGFX_API void mgfx_bind_pipeline (mgfx_pipeline pipeline);
    
    MGFX_API mgfx_buffer mgfx_create_buffer (const mgfx_buffer_create_info *create_info);
    MGFX_API void mgfx_destroy_buffer (mgfx_buffer buffer);
    MGFX_API void mgfx_update_buffer (mgfx_buffer buffer, size_t offset, size_t size, void *data);
    
    MGFX_API void mgfx_bind_vertex_buffer (mgfx_buffer buffer);
    MGFX_API void mgfx_bind_index_buffer (mgfx_buffer buffer, mgfx_index_type index_type);
    
    MGFX_API void mgfx_bind_uniforms (uint32_t binding, size_t size, void *data);
    
    MGFX_API mgfx_image mgfx_create_image (const mgfx_image_create_info *create_info);
    MGFX_API void mgfx_destroy_image (mgfx_image image);
    MGFX_API void mgfx_update_image (mgfx_image image, size_t size, void *data);
    MGFX_API void mgfx_bind_image (mgfx_image image, mgfx_sampler sampler, uint32_t binding);
    
    MGFX_API mgfx_sampler mgfx_create_sampler(const mgfx_sampler_create_info *create_info);
    MGFX_API void mgfx_destroy_sampler(mgfx_sampler sampler);
    
    MGFX_API mgfx_renderer_type mgfx_get_renderer_type (void);
    MGFX_API mgfx_shader_lang mgfx_get_shader_lang (void);

#ifdef __cplusplus
} // extern "C"

inline void mgfx_init(const mgfx_init_info &init_info) { return mgfx_init(&init_info); }
inline void mgfx_bind_pass(const mgfx_pass_info &pass_info) { return mgfx_bind_pass(&pass_info); }
inline mgfx_pipeline mgfx_create_pipeline(const mgfx_pipeline_create_info &create_info) { return mgfx_create_pipeline(&create_info); }
inline mgfx_buffer mgfx_create_buffer(const mgfx_buffer_create_info &create_info) { return mgfx_create_buffer(&create_info); }
inline mgfx_image mgfx_create_image(const mgfx_image_create_info &create_info) { return mgfx_create_image(&create_info); }
inline mgfx_sampler mgfx_create_sampler(const mgfx_sampler_create_info &create_info) { return mgfx_create_sampler(&create_info); }

#endif
    
#if defined(MG_IMPL) || defined(MGFX_IMPL)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MGFX_ASSERT(x, msg) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "[MGFX]: Assertion failed: %s\nFile: %s:%d\nMessage: %s\n", #x, __FILE__, __LINE__, msg); \
            abort(); \
        } \
    } while (0)

#if defined(MGFX_VULKAN)

#include <vulkan/vulkan.h>
#pragma comment (lib, "vulkan-1")

typedef struct
{
    uint8_t *buffer;
    uint32_t capacity;
    uint32_t element_size;
    
    uint32_t head;
    uint32_t tail;
    uint32_t count;
}
mgfx_queue;

typedef struct
{
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkFormat format;
    uint32_t width, height, depth;
    bool is_cpu;
}
mgfx_vk_image;

typedef struct
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    bool is_cpu;
}
mgfx_vk_buffer;

typedef struct
{
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkPipelineBindPoint bind_point;
}
mgfx_vk_pipeline;

typedef struct
{
    union
    {
        struct
        {
            VkImageView image_view;
            VkSampler sampler;
        };
        VkBuffer buffer; // later for storage buffers
    };
    uint32_t binding;
}
mgfx_vk_descriptor_binding;

typedef struct
{
    mgfx_vk_descriptor_binding bindings[MGFX_MAX_BINDABLE_IMAGES];
    uint64_t hash;
    uint32_t binding_count;
}
mgfx_vk_descriptor_set_key;

typedef struct
{
    mgfx_vk_descriptor_set_key key;
}
mgfx_vk_cached_descriptor_set;

typedef struct
{    
    VkDescriptorSet sets[MGFX_MAX_DESCRIPTOR_CACHE];
    mgfx_vk_cached_descriptor_set cache[MGFX_MAX_DESCRIPTOR_CACHE];
    uint32_t cache_size;
    
    mgfx_vk_descriptor_binding bound_images[MGFX_MAX_BINDABLE_IMAGES];
    bool bound_image_active[MGFX_MAX_BINDABLE_IMAGES];
    bool dirty;
}
mgfx_vk_descriptor_cache;

typedef uint8_t mgfx_vk_release_queue_entry_type;
enum
{
    MGFX_VK_RELEASE_QUEUE_ENTRY_BUFFER,
    MGFX_VK_RELEASE_QUEUE_ENTRY_IMAGE,
    MGFX_VK_RELEASE_QUEUE_ENTRY_SAMPLER,
    MGFX_VK_RELEASE_QUEUE_ENTRY_PIPELINE
};

typedef struct
{
    union
    {
        mgfx_vk_buffer *buffer;
        mgfx_vk_image *image;
        VkSampler sampler;
        mgfx_vk_pipeline *pipeline;
    };
    mgfx_vk_release_queue_entry_type type;
}
mgfx_vk_release_queue_entry;

typedef struct
{
    VkInstance instance;
    VkSurfaceKHR surface;
    
    struct
    {
        VkPhysicalDevice handle;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        uint32_t queue_family;
    }
    physical_device;
    
    struct
    {
        VkDevice handle;
        VkQueue graphics_compute_queue;
    }
    device;
    
    struct
    {
        VkSwapchainKHR handle;
        
        VkImage images[4];
        VkImageView image_views[4];
        
        VkImage depth_image;
        VkDeviceMemory depth_image_memory;
        VkImageView depth_image_view;
        VkFormat depth_format;
        
        uint32_t image_count;
        uint32_t image_index;
    }
    swapchain;
    
    struct
    {
        VkFence fence;
        VkSemaphore image_available_semaphore;
        VkSemaphore image_rendered_semaphore;
    }
    sync_objects;
    
    bool inside_pass;
    
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkDescriptorPool descriptor_pool;
    
    struct
    {
        VkDescriptorSetLayout scratch_buffer_layout;
        VkDescriptorSetLayout image_sampler_layout;
    }
    layouts;
    
    mgfx_vk_pipeline *current_pipeline;
    
    struct
    {
        mgfx_vk_image *color_images[MGFX_MAX_COLOR_ATTACHMENTS];
        uint32_t color_image_count;
        mgfx_vk_image *depth_image;
    }
    current_pass;
    
    struct
    {
        VkBuffer buffer;
        VkDeviceMemory memory;
        uint8_t *data;
        
        uint32_t bind_offsets[MGFX_MAX_BINDABLE_UNIFORMS];
        uint32_t offset;
        
        VkDescriptorSet ub_set;
    }
    scratch_buffer;
    
    mgfx_vk_descriptor_cache descriptor_cache;
    mgfx_queue release_queue;
    
    uint32_t width, height;
    bool vsync, rebuild_swapchain;
}
mgfx_vk_context;

#endif

#if defined(MGFX_OPENGL)

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

#else

#if defined(__linux__)
#include <EGL/egl.h>
#define GL_APIENTRY

#elif defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#pragma comment (lib, "kernel32")
#pragma comment (lib, "opengl32")
#define GL_APIENTRY APIENTRY
#endif

typedef unsigned int  GLenum;
typedef unsigned int  GLuint;
typedef int  GLsizei;
typedef char  GLchar;
typedef ptrdiff_t  GLintptr;
typedef ptrdiff_t  GLsizeiptr;
typedef double  GLclampd;
typedef unsigned short  GLushort;
typedef unsigned char  GLubyte;
typedef unsigned char  GLboolean;
typedef uint64_t  GLuint64;
typedef double  GLdouble;
typedef unsigned short  GLhalf;
typedef float  GLclampf;
typedef unsigned int  GLbitfield;
typedef signed char  GLbyte;
typedef short  GLshort;
typedef void  GLvoid;
typedef int64_t  GLint64;
typedef float  GLfloat;
typedef int  GLint;

#define __gl_h_ 1
#define __gl32_h_ 1
#define __gl31_h_ 1
#define __GL_H__ 1
#define __glext_h_ 1
#define __GLEXT_H_ 1
#define __gltypes_h_ 1
#define __glcorearb_h_ 1
#define __gl_glcorearb_h_ 1

#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_VERTEX_SHADER                  0x8B31
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_STATIC_DRAW                    0x88E4
#define GL_FUNC_SUBTRACT                  0x800A
#define GL_FUNC_REVERSE_SUBTRACT          0x800B
#define GL_CONSTANT_COLOR                 0x8001
#define GL_R8                             0x8229
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_DEPTH_TEST                     0x0B71
#define GL_RGBA16F                        0x881A
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_TEXTURE0                       0x84C0
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_SRC_ALPHA_SATURATE             0x0308
#define GL_ONE                            1
#define GL_RGB8                           0x8051
#define GL_ARRAY_BUFFER                   0x8892
#define GL_TEXTURE_2D                     0x0DE1
#define GL_FRONT                          0x0404
#define GL_REPEAT                         0x2901
#define GL_RGBA                           0x1908
#define GL_BGRA                           0x80E1
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_FLOAT                          0x1406
#define GL_DEPTH_COMPONENT                0x1902
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_TRIANGLES                      0x0004
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_ONE_MINUS_CONSTANT_ALPHA       0x8004
#define GL_NONE                           0
#define GL_SRC_COLOR                      0x0300
#define GL_SRC1_ALPHA                     0x8589
#define GL_SRC1_COLOR                     0x88F9
#define GL_ONE_MINUS_SRC1_ALPHA           0x88FB
#define GL_ONE_MINUS_SRC1_COLOR           0x88FA
#define GL_BYTE                           0x1400
#define GL_LINE_STRIP                     0x0003
#define GL_TEXTURE_3D                     0x806F
#define GL_TEXTURE_CUBE_MAP               0x8513
#define GL_CW                             0x0900
#define GL_LINEAR                         0x2601
#define GL_GEQUAL                         0x0206
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_RGBA32F                        0x8814
#define GL_BLEND                          0x0BE2
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_CCW                            0x0901
#define GL_RGB                            0x1907
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_FALSE                          0
#define GL_ZERO                           0
#define GL_CULL_FACE                      0x0B44
#define GL_INVERT                         0x150A
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_NEAREST                        0x2600
#define GL_SCISSOR_TEST                   0x0C11
#define GL_LEQUAL                         0x0203
#define GL_STENCIL_TEST                   0x0B90
#define GL_DEPTH_COMPONENT32F             0x8CAC
#define GL_EQUAL                          0x0202
#define GL_FRAMEBUFFER                    0x8D40
#define GL_LINES                          0x0001
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_SRC_ALPHA                      0x0302
#define GL_LESS                           0x0201
#define GL_BACK                           0x0405
#define GL_ALWAYS                         0x0207
#define GL_FUNC_ADD                       0x8006
#define GL_ONE_MINUS_DST_COLOR            0x0307
#define GL_NOTEQUAL                       0x0205
#define GL_DST_COLOR                      0x0306
#define GL_RED                            0x1903
#define GL_DST_ALPHA                      0x0304
#define GL_GREATER                        0x0204
#define GL_TRUE                           1
#define GL_NEVER                          0x0200
#define GL_POINTS                         0x0000
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_MIRRORED_REPEAT                0x8370
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_RGBA8                          0x8058
#define GL_RG8                            0x822B
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG                             0x8227
#define GL_RED_INTEGER                    0x8D94
#define GL_DEPTH_STENCIL                  0x84F9
#define GL_UNSIGNED_INT_24_8              0x84FA
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_DEPTH32F_STENCIL8              0x8CAD
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_CLAMP_TO_BORDER                0x812D
#define GL_MIN                            0x8007
#define GL_MAX                            0x8008

#endif

typedef struct
{
    GLuint id;
    GLenum target;
    GLenum usage;
}
mgfx_gl_buffer;

typedef enum
{
    MGFX_OPENGL_PIPELINE_TYPE_GRAPHICS,
    MGFX_OPENGL_PIPELINE_TYPE_COMPUTE
}
mgfx_gl_pipeline_type;

typedef struct
{
    uint32_t location;
    uint32_t offset;
    mgfx_vertex_format format;
}
mgfx_gl_vertex_attribute;

typedef struct
{
    mgfx_gl_pipeline_type type;
    GLuint program_id;
    GLuint sampler_image_location;
    
    struct
    {
        uint32_t stride;
        uint32_t attribute_count;
        mgfx_gl_vertex_attribute attributes[MGFX_MAX_VERTEX_ATTRIBUTES];
    }
    vertex_layout;
    
    GLenum primitive_topology;
    GLenum cull_mode;
    GLenum front_face;
    mgfx_color_blend color_blend;
    
    struct
    {
        bool stencil_test_enabled;
        bool depth_write_enabled;
        GLenum depth_compare_op;
    }
    depth_stencil;
}
mgfx_gl_pipeline;

typedef struct
{
    GLuint texture_id;
    GLenum texture_target;
    GLenum format;
    uint32_t width, height, depth;
}
mgfx_gl_image;

typedef struct
{
    GLint min_filter;
    GLint mag_filter;
    
    GLint address_mode_u;
    GLint address_mode_v;
    GLint address_mode_w;
}
mgfx_gl_sampler;

typedef struct
{
    GLuint vao;
    GLuint uniform_buffers[MGFX_MAX_BINDABLE_UNIFORMS];
    
    GLuint scratch_framebuffer;
    
    GLenum index_type;
    GLenum primitive_topology;
    
    struct
    {
        GLuint program;
        GLuint framebuffer;
        GLuint color_attachment;
    }
    swapchain;
    
    mgfx_gl_pipeline *current_pipeline;
    
#if defined(__EMSCRIPTEN__)
    struct
    {
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
    }
    emscripten;
#elif defined(_WIN32)
    struct
    {
        HINSTANCE opengl32_dll;
        HWND hwnd;
        HDC hdc;
        HGLRC hrc;
    }
    wgl;
#elif defined(__linux__)
    struct
    {
        EGLDisplay display;
        EGLSurface surface;
        EGLContext context;
    }
    egl;
#endif
    
    int32_t width, height;
    bool vsync;
}
mgfx_gl_context;

#endif

#if defined(MGFX_D3D11)

#ifndef D3D11_NO_HELPERS
#define D3D11_NO_HELPERS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <d3dcompiler.h>

#if defined(__cplusplus)
#define MGFX_D3D11_CALL0(obj, method) (obj)->method()
#define MGFX_D3D11_CALL(obj, method, ...) (obj)->method(__VA_ARGS__)
#define MGFX_IID(x) (x)
#else
#define MGFX_D3D11_CALL0(obj, method) (obj)->lpVtbl->method((obj))
#define MGFX_D3D11_CALL(obj, method, ...) (obj)->lpVtbl->method((obj), __VA_ARGS__)
#define MGFX_IID(x) (&(x))
#endif
#define MGFX_D3D11_SAFE_RELEASE(obj) if (obj) { MGFX_D3D11_CALL0(obj, Release); (obj) = NULL; }

#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")
#pragma comment (lib, "dxguid.lib")

typedef struct
{
    ID3D11Resource *texture;
    ID3D11ShaderResourceView *view;
    union
    {
        ID3D11RenderTargetView  *rtv;
        ID3D11DepthStencilView  *dsv;
    };
    uint32_t width, height;
    bool is_cpu;
}
mgfx_d3d11_image;

typedef struct
{
    ID3D11Buffer *buffer;
    bool is_cpu;
}
mgfx_d3d11_buffer;

typedef uint32_t mgfx_d3d11_pipeline_type;
enum
{
    MGFX_D3D11_PIPELINE_TYPE_GRAPHICS,
    MGFX_D3D11_PIPELINE_TYPE_COMPUTE
};

typedef struct
{
    ID3D11Buffer *constant_buffers[MGFX_MAX_BINDABLE_UNIFORMS];
    ID3D11InputLayout *vertex_layout;
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11ComputeShader *compute_shader;
    ID3D11RasterizerState *raster_state;
    ID3D11DepthStencilState *depth_stencil_state;
    ID3D11BlendState *blend_state;
    D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
    uint32_t layout_stride;
    mgfx_d3d11_pipeline_type type;
}
mgfx_d3d11_pipeline;

typedef struct
{
    mgfx_d3d11_pipeline *current_pipeline;
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain *swapchain;
    ID3D11DeviceContext *immediate_context;
    ID3D11RenderTargetView *target_view;
    
    uint32_t width, height;
    bool vsync, pending_resize;
}
mgfx_d3d11_context;

#endif

typedef void (*mgfx_init_fn)(const mgfx_init_info *init_info);
typedef void (*mgfx_shutdown_fn)(void);
typedef mgfx_result (*mgfx_begin_fn)(void);
typedef void (*mgfx_end_fn)(void);
typedef void (*mgfx_present_frame_fn)(void);

typedef void (*mgfx_resize_fn)(uint32_t width, uint32_t height);
typedef void (*mgfx_vsync_fn)(bool vsync);

typedef void (*mgfx_viewport_fn)(int32_t x, int32_t y, uint32_t width, uint32_t height);
typedef void (*mgfx_scissor_fn)(int32_t x, int32_t y, uint32_t width, uint32_t height);

typedef void (*mgfx_bind_pass_fn)(const mgfx_pass_info *pass_info);

typedef void *(*mgfx_create_pipeline_fn)(const mgfx_pipeline_create_info *create_info);
typedef void (*mgfx_destroy_pipeline_fn)(void *pipeline);
typedef void (*mgfx_bind_pipeline_fn)(void *pipeline);

typedef void *(*mgfx_create_image_fn)(const mgfx_image_create_info *create_info);
typedef void (*mgfx_destroy_image_fn)(void *image);
typedef void (*mgfx_update_image_fn)(void *image, size_t size, void *data);
typedef void (*mgfx_bind_image_fn)(void *image, void *sampler, uint32_t binding);

typedef void *(*mgfx_create_sampler_fn)(const mgfx_sampler_create_info *create_info);
typedef void (*mgfx_destroy_sampler_fn)(void *sampler);

typedef void *(*mgfx_create_buffer_fn)(const mgfx_buffer_create_info *create_info);
typedef void (*mgfx_destroy_buffer_fn)(void *buffer);
typedef void (*mgfx_update_buffer_fn)(void *buffer, size_t offset, size_t size, void *data);

typedef void (*mgfx_bind_vertex_buffer_fn)(void *buffer);
typedef void (*mgfx_bind_index_buffer_fn)(void *buffer, mgfx_index_type index_type);

typedef void (*mgfx_bind_uniforms_fn)(uint32_t binding, size_t size, void *data);

typedef void (*mgfx_draw_fn)(uint32_t vertex_count, uint32_t first_vertex);
typedef void (*mgfx_draw_indexed_fn)(uint32_t index_count, uint32_t first_index, int32_t vertex_offset);
typedef void (*mgfx_draw_instanced_fn)(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance);
typedef void (*mgfx_draw_indexed_instanced_fn)(uint32_t index_count, uint32_t first_index, int32_t vertex_offset, uint32_t instance_count, uint32_t first_instance);
typedef void (*mgfx_dispatch_fn)(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

typedef struct
{
    union
    {
#if defined(MGFX_VULKAN)
        mgfx_vk_context vk;
#endif
#if defined(MGFX_OPENGL)
        mgfx_gl_context gl;
#endif
#if defined(MGFX_D3D11)
        mgfx_d3d11_context d3d11;
#endif
    };

    mgfx_init_fn                    init;
    mgfx_shutdown_fn                shutdown;
    
    mgfx_begin_fn                   begin;
    mgfx_end_fn                     end;
    mgfx_present_frame_fn           present_frame;
    
    mgfx_resize_fn                  resize;
    mgfx_vsync_fn                   vsync;
    
    mgfx_viewport_fn                viewport;
    mgfx_scissor_fn                 scissor;
    
    mgfx_bind_pass_fn               bind_pass;
    
    mgfx_create_pipeline_fn         create_pipeline;
    mgfx_destroy_pipeline_fn        destroy_pipeline;
    mgfx_bind_pipeline_fn           bind_pipeline;
    
    mgfx_create_image_fn            create_image;
    mgfx_destroy_image_fn           destroy_image;
    mgfx_update_image_fn            update_image;
    mgfx_bind_image_fn              bind_image;
    
    mgfx_create_sampler_fn          create_sampler;
    mgfx_destroy_sampler_fn         destroy_sampler;
    
    mgfx_create_buffer_fn           create_buffer;
    mgfx_destroy_buffer_fn          destroy_buffer;
    mgfx_update_buffer_fn           update_buffer;
    
    mgfx_bind_vertex_buffer_fn      bind_vertex_buffer;
    mgfx_bind_index_buffer_fn       bind_index_buffer;
    
    mgfx_bind_uniforms_fn           bind_uniforms;
    
    mgfx_draw_fn                    draw;
    mgfx_draw_indexed_fn            draw_indexed;
    mgfx_draw_instanced_fn          draw_instanced;
    mgfx_draw_indexed_instanced_fn  draw_indexed_instanced;
    mgfx_dispatch_fn                dispatch;
    
    mgfx_renderer_type type;
    mgfx_shader_lang shader_lang;
}
mgfx_context;
static mgfx_context ctx;

static inline uint32_t mgfx_stride_align(uint32_t size, uint32_t alignment)
{
    return (size+(alignment-1)) & ~(alignment-1);
}

static inline bool mgfx_valid_pass(const mgfx_pass_info *pass)
{
    MGFX_ASSERT(pass != NULL, "Pass reference in mgfx_bind_pass cannot be null.");
    return pass->color_images[0] || pass->depth_stencil_image;
}

static inline uint8_t mgfx_format_bpp(mgfx_format format)
{
    switch (format)
    {
        case MGFX_FORMAT_R8_UNORM:           return 1;
        case MGFX_FORMAT_RG8_UNORM:          return 2;
        case MGFX_FORMAT_RGB8_UNORM:
        case MGFX_FORMAT_RGB8_SRGB:          return 3;
        case MGFX_FORMAT_RGBA8_UNORM:
        case MGFX_FORMAT_RGBA8_SRGB:
        case MGFX_FORMAT_BGRA8_UNORM:
        case MGFX_FORMAT_BGRA8_SRGB:         return 4;
        
        case MGFX_FORMAT_R16_SFLOAT:         return 2;
        case MGFX_FORMAT_RG16_SFLOAT:        return 4;
        case MGFX_FORMAT_RGB16_SFLOAT:       return 6;
        case MGFX_FORMAT_RGBA16_SFLOAT:      return 8;
        
        case MGFX_FORMAT_R32_SFLOAT:
        case MGFX_FORMAT_R32_UINT:
        case MGFX_FORMAT_R32_SINT:           return 4;
        case MGFX_FORMAT_RG32_SFLOAT:        return 8;
        case MGFX_FORMAT_RGB32_SFLOAT:       return 12;
        case MGFX_FORMAT_RGBA32_SFLOAT:      return 16;
        
        case MGFX_FORMAT_D16_UNORM_S8_UINT:  return 3;
        case MGFX_FORMAT_D24_UNORM_S8_UINT:  return 4;
        case MGFX_FORMAT_D32_SFLOAT_S8_UINT: return 5;
        
        default: return 4;
    }
}

static inline uint32_t mgfx_vertex_format_size(mgfx_vertex_format format)
{
    switch (format)
    {
        case MGFX_VERTEX_FORMAT_UINT:    return 4;
        case MGFX_VERTEX_FORMAT_INT:     return 4;
        case MGFX_VERTEX_FORMAT_FLOAT:   return 4;
        
        case MGFX_VERTEX_FORMAT_UINT2:   return 8;
        case MGFX_VERTEX_FORMAT_INT2:    return 8;
        case MGFX_VERTEX_FORMAT_FLOAT2:  return 8;
        
        case MGFX_VERTEX_FORMAT_UINT3:   return 12;
        case MGFX_VERTEX_FORMAT_INT3:    return 12;
        case MGFX_VERTEX_FORMAT_FLOAT3:  return 12;
        
        case MGFX_VERTEX_FORMAT_UINT4:   return 16;
        case MGFX_VERTEX_FORMAT_INT4:    return 16;
        case MGFX_VERTEX_FORMAT_FLOAT4:  return 16;
        
        case MGFX_VERTEX_FORMAT_UBYTE4:  return 4;
        case MGFX_VERTEX_FORMAT_BYTE4:   return 4;
        case MGFX_VERTEX_FORMAT_UBYTE4N: return 4;
        case MGFX_VERTEX_FORMAT_BYTE4N:  return 4;
        
        default: return 0;
    }
}

#if defined(MGFX_VULKAN)

static inline int32_t mgfx_clampi(int32_t d, int32_t min, int32_t max)
{
    const int t = d < min ? min : d;
    return t > max ? max : t;
}

static inline void mgfx_create_queue(mgfx_queue *queue, uint32_t element_size, uint32_t capacity)
{
    queue->buffer = (uint8_t*)malloc(capacity * element_size);
    queue->capacity = capacity;
    queue->element_size = element_size;
    
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

static inline void mgfx_destroy_queue(mgfx_queue *queue)
{
    free(queue->buffer);
}

static inline bool mgfx_queue_full(mgfx_queue *queue)
{
    return queue->count >= queue->capacity;
}

static inline bool mgfx_queue_empty(mgfx_queue *queue)
{
    return queue->count == 0;
}

static inline void mgfx_queue_push(mgfx_queue *queue, const void *data)
{
    if (mgfx_queue_full(queue))
        return;
    
    memcpy(
            queue->buffer + (queue->tail * queue->element_size),
            data,
            queue->element_size
            );
    
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
}

static inline void *mgfx_queue_pop(mgfx_queue *queue)
{
    if (mgfx_queue_empty(queue))
        return NULL;
    
    void *result = queue->buffer + (queue->head * queue->element_size);
    
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    return result;
}

static inline VkFormat mgfx_vk_get_format(mgfx_format format)
{
    switch (format)
    {
        case MGFX_FORMAT_R8_UNORM:            return VK_FORMAT_R8_UNORM;
        case MGFX_FORMAT_RG8_UNORM:           return VK_FORMAT_R8G8_UNORM;
        case MGFX_FORMAT_RGB8_UNORM:          return VK_FORMAT_R8G8B8_UNORM;
        case MGFX_FORMAT_RGB8_SRGB:           return VK_FORMAT_R8G8B8_SRGB;
        case MGFX_FORMAT_RGBA8_UNORM:         return VK_FORMAT_R8G8B8A8_UNORM;
        case MGFX_FORMAT_RGBA8_SRGB:          return VK_FORMAT_R8G8B8A8_SRGB;
        case MGFX_FORMAT_BGRA8_UNORM:         return VK_FORMAT_B8G8R8A8_UNORM;
        case MGFX_FORMAT_BGRA8_SRGB:          return VK_FORMAT_B8G8R8A8_SRGB;
        
        case MGFX_FORMAT_R16_SFLOAT:          return VK_FORMAT_R16_SFLOAT;
        case MGFX_FORMAT_RG16_SFLOAT:         return VK_FORMAT_R16G16_SFLOAT;
        case MGFX_FORMAT_RGB16_SFLOAT:        return VK_FORMAT_R16G16B16_SFLOAT;
        case MGFX_FORMAT_RGBA16_SFLOAT:       return VK_FORMAT_R16G16B16A16_SFLOAT;
        
        case MGFX_FORMAT_R32_SFLOAT:          return VK_FORMAT_R32_SFLOAT;
        case MGFX_FORMAT_RG32_SFLOAT:         return VK_FORMAT_R32G32_SFLOAT;
        case MGFX_FORMAT_RGB32_SFLOAT:        return VK_FORMAT_R32G32B32_SFLOAT;
        case MGFX_FORMAT_RGBA32_SFLOAT:       return VK_FORMAT_R32G32B32A32_SFLOAT;
        
        case MGFX_FORMAT_R32_UINT:            return VK_FORMAT_R32_UINT;
        case MGFX_FORMAT_R32_SINT:            return VK_FORMAT_R32_SINT;
        
        case MGFX_FORMAT_D16_UNORM_S8_UINT:   return VK_FORMAT_D16_UNORM_S8_UINT;
        case MGFX_FORMAT_D24_UNORM_S8_UINT:   return VK_FORMAT_D24_UNORM_S8_UINT;
        case MGFX_FORMAT_D32_SFLOAT_S8_UINT:  return VK_FORMAT_D32_SFLOAT_S8_UINT;
        
        default: return VK_FORMAT_UNDEFINED;
    }
}

static inline VkFormat mgfx_vk_get_vertex_format(mgfx_vertex_format format)
{
    switch (format)
    {
        case MGFX_VERTEX_FORMAT_UINT:    return VK_FORMAT_R32_UINT;
        case MGFX_VERTEX_FORMAT_INT:     return VK_FORMAT_R32_SINT;
        case MGFX_VERTEX_FORMAT_FLOAT:   return VK_FORMAT_R32_SFLOAT;
        
        case MGFX_VERTEX_FORMAT_UINT2:   return VK_FORMAT_R32G32_UINT;
        case MGFX_VERTEX_FORMAT_INT2:    return VK_FORMAT_R32G32_SINT;
        case MGFX_VERTEX_FORMAT_FLOAT2:  return VK_FORMAT_R32G32_SFLOAT;
        
        case MGFX_VERTEX_FORMAT_UINT3:   return VK_FORMAT_R32G32B32_UINT;
        case MGFX_VERTEX_FORMAT_INT3:    return VK_FORMAT_R32G32B32_SINT;
        case MGFX_VERTEX_FORMAT_FLOAT3:  return VK_FORMAT_R32G32B32_SFLOAT;
        
        case MGFX_VERTEX_FORMAT_UINT4:   return VK_FORMAT_R32G32B32A32_UINT;
        case MGFX_VERTEX_FORMAT_INT4:    return VK_FORMAT_R32G32B32A32_SINT;
        case MGFX_VERTEX_FORMAT_FLOAT4:  return VK_FORMAT_R32G32B32A32_SFLOAT;
        
        case MGFX_VERTEX_FORMAT_UBYTE4:  return VK_FORMAT_R8G8B8A8_UINT;
        case MGFX_VERTEX_FORMAT_BYTE4:   return VK_FORMAT_R8G8B8A8_SINT;
        
        case MGFX_VERTEX_FORMAT_UBYTE4N: return VK_FORMAT_R8G8B8A8_UNORM;
        case MGFX_VERTEX_FORMAT_BYTE4N:  return VK_FORMAT_R8G8B8A8_SNORM;
        
        default: return VK_FORMAT_UNDEFINED;
    }
}

static inline VkImageType mgfx_vk_get_image_type(mgfx_image_type type)
{
    switch (type)
    {
        case MGFX_IMAGE_TYPE_2D:   return VK_IMAGE_TYPE_2D;
        case MGFX_IMAGE_TYPE_3D:   return VK_IMAGE_TYPE_3D;
        case MGFX_IMAGE_TYPE_CUBE: return VK_IMAGE_TYPE_2D; // cube uses 2D layers
        default:                   return VK_IMAGE_TYPE_2D;
    }
}

static inline VkImageViewType mgfx_vk_get_image_view_type(mgfx_image_type type)
{
    switch (type)
    {
        case MGFX_IMAGE_TYPE_2D:   return VK_IMAGE_VIEW_TYPE_2D;
        case MGFX_IMAGE_TYPE_3D:   return VK_IMAGE_VIEW_TYPE_3D;
        case MGFX_IMAGE_TYPE_CUBE: return VK_IMAGE_VIEW_TYPE_CUBE;
        default:                   return VK_IMAGE_VIEW_TYPE_2D;
    }
}

static inline VkFilter mgfx_vk_get_filter(mgfx_sampler_filter filter)
{
    switch (filter)
    {
        case MGFX_SAMPLER_FILTER_NEAREST: return VK_FILTER_NEAREST;
        case MGFX_SAMPLER_FILTER_LINEAR:  return VK_FILTER_LINEAR;
        default:                          return VK_FILTER_NEAREST;
    }
}

static inline VkSamplerAddressMode mgfx_vk_get_address_mode(mgfx_sampler_address_mode mode)
{
    switch (mode)
    {
        case MGFX_SAMPLER_ADDRESS_MODE_REPEAT:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case MGFX_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:                                        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

static inline VkPrimitiveTopology mgfx_vk_get_primitive_topology(mgfx_primitive_topology topology)
{
    switch (topology)
    {
        case MGFX_PRIMITIVE_TOPOLOGY_POINT_LIST:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case MGFX_PRIMITIVE_TOPOLOGY_LINE_LIST:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case MGFX_PRIMITIVE_TOPOLOGY_LINE_STRIP:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        default:                                     return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static inline VkCullModeFlags mgfx_vk_get_cull_mode(mgfx_cull_mode mode)
{
    switch (mode)
    {
        case MGFX_CULL_MODE_NONE:  return VK_CULL_MODE_NONE;
        case MGFX_CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case MGFX_CULL_MODE_BACK:  return VK_CULL_MODE_BACK_BIT;
        default:                   return VK_CULL_MODE_NONE;
    }
}

static inline VkFrontFace mgfx_vk_get_front_face(mgfx_front_face front_face)
{
    switch (front_face)
    {
        case MGFX_FRONT_FACE_CCW: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case MGFX_FRONT_FACE_CW:  return VK_FRONT_FACE_CLOCKWISE;
        default:                  return VK_FRONT_FACE_CLOCKWISE;
    }
}

static inline VkBlendFactor mgfx_vk_get_blend_factor(mgfx_blend_factor factor)
{
    switch (factor)
    {
        case MGFX_BLEND_FACTOR_ZERO:                     return VK_BLEND_FACTOR_ZERO;
        case MGFX_BLEND_FACTOR_ONE:                      return VK_BLEND_FACTOR_ONE;
        case MGFX_BLEND_FACTOR_SRC_COLOR:                return VK_BLEND_FACTOR_SRC_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case MGFX_BLEND_FACTOR_DST_COLOR:                return VK_BLEND_FACTOR_DST_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case MGFX_BLEND_FACTOR_SRC_ALPHA:                return VK_BLEND_FACTOR_SRC_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case MGFX_BLEND_FACTOR_DST_ALPHA:                return VK_BLEND_FACTOR_DST_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case MGFX_BLEND_FACTOR_CONSTANT_COLOR:           return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case MGFX_BLEND_FACTOR_CONSTANT_ALPHA:           return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case MGFX_BLEND_FACTOR_SRC_ALPHA_SATURATE:       return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case MGFX_BLEND_FACTOR_SRC1_COLOR:               return VK_BLEND_FACTOR_SRC1_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:     return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case MGFX_BLEND_FACTOR_SRC1_ALPHA:               return VK_BLEND_FACTOR_SRC1_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:     return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:                                         return VK_BLEND_FACTOR_ZERO;
    }
}

static inline VkBlendOp mgfx_vk_get_blend_op(mgfx_blend_op op)
{
    switch (op)
    {
        case MGFX_BLEND_OP_ADD:              return VK_BLEND_OP_ADD;
        case MGFX_BLEND_OP_SUBTRACT:         return VK_BLEND_OP_SUBTRACT;
        case MGFX_BLEND_OP_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case MGFX_BLEND_OP_MIN:              return VK_BLEND_OP_MIN;
        case MGFX_BLEND_OP_MAX:              return VK_BLEND_OP_MAX;
        case MGFX_BLEND_OP_ZERO:             return VK_BLEND_OP_ZERO_EXT;
        case MGFX_BLEND_OP_SRC:              return VK_BLEND_OP_SRC_EXT;
        case MGFX_BLEND_OP_DST:              return VK_BLEND_OP_DST_EXT;
        case MGFX_BLEND_OP_INVERT:           return VK_BLEND_OP_INVERT_EXT;
        case MGFX_BLEND_OP_RED:              return VK_BLEND_OP_RED_EXT;
        default:                             return VK_BLEND_OP_ADD;
    }
}

static inline VkCompareOp mgfx_vk_get_compare_op(mgfx_compare_op op)
{
    switch (op)
    {
        case MGFX_COMPARE_OP_NEVER:            return VK_COMPARE_OP_NEVER;
        case MGFX_COMPARE_OP_LESS:             return VK_COMPARE_OP_LESS;
        case MGFX_COMPARE_OP_EQUAL:            return VK_COMPARE_OP_EQUAL;
        case MGFX_COMPARE_OP_LESS_OR_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case MGFX_COMPARE_OP_GREATER:          return VK_COMPARE_OP_GREATER;
        case MGFX_COMPARE_OP_NOT_EQUAL:        return VK_COMPARE_OP_NOT_EQUAL;
        case MGFX_COMPARE_OP_GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case MGFX_COMPARE_OP_ALWAYS:           return VK_COMPARE_OP_ALWAYS;
        default:                               return VK_COMPARE_OP_NEVER;
    }
}

static inline VkBufferUsageFlags mgfx_vk_get_buffer_usage(mgfx_buffer_usage usage)
{
    switch (usage)
    {
        case MGFX_BUFFER_USAGE_INDEX:  return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case MGFX_BUFFER_USAGE_VERTEX: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        default:                       return 0;
    }
}

static inline VkImageUsageFlags mgfx_vk_get_image_usage(mgfx_image_usage usage)
{
    switch (usage)
    {
        case MGFX_IMAGE_USAGE_COLOR_ATTACHMENT:         return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        case MGFX_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        default:                                        return 0;
    }
}

static inline VkIndexType mgfx_vk_get_index_type(mgfx_index_type index_type)
{
    return index_type == MGFX_INDEX_TYPE_UINT32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
}

static uint64_t mgfx_vk_hash_descriptor_key(const mgfx_vk_descriptor_set_key* key)
{
    uint64_t hash = 14695981039346656037ULL;
    const uint8_t* data = (const uint8_t*)key->bindings;
    size_t size = sizeof(mgfx_vk_descriptor_binding) * key->binding_count;
    
    for (size_t i = 0; i < size; i++)
    {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    
    return hash;
}

static bool mgfx_vk_compare_descriptor_keys(const mgfx_vk_descriptor_set_key* a, const mgfx_vk_descriptor_set_key* b)
{
    if (a->binding_count != b->binding_count || a->hash != b->hash)
        return false;
    
    return memcmp(a->bindings, b->bindings, sizeof(mgfx_vk_descriptor_binding) * a->binding_count) == 0;
}

static int32_t mgfx_vk_find_cached_set(mgfx_vk_descriptor_cache* cache, const mgfx_vk_descriptor_set_key* key)
{
    for (uint32_t i = 0; i < cache->cache_size; i++)
        if (mgfx_vk_compare_descriptor_keys(&cache->cache[i].key, key))
        return (int32_t)i;
    return -1;
}

static VkDescriptorSet mgfx_vk_commit_image_descriptor_cache(mgfx_vk_descriptor_cache *cache)
{
    if (!cache->dirty)
        return VK_NULL_HANDLE;
    
    mgfx_vk_descriptor_set_key key;
    key.binding_count = 0;
    
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_IMAGES; i++)
    {
        if (cache->bound_image_active[i])
            key.bindings[key.binding_count++] = cache->bound_images[i];
    }
    
    if (key.binding_count == 0) return VK_NULL_HANDLE;
    
    key.hash = mgfx_vk_hash_descriptor_key(&key);
    
    int32_t cached_index = mgfx_vk_find_cached_set(cache, &key);
    if (cached_index >= 0)
    {
        cache->dirty = false;
        return cache->sets[cached_index];
    }
    
    if (cache->cache_size >= MGFX_MAX_DESCRIPTOR_CACHE)
        return VK_NULL_HANDLE;
    
    uint32_t new_index = cache->cache_size++;
    
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = ctx.vk.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &ctx.vk.layouts.image_sampler_layout
    };
    vkAllocateDescriptorSets(ctx.vk.device.handle, &alloc_info, &cache->sets[new_index]);
    cache->cache[new_index].key = key;
    
    VkWriteDescriptorSet writes[MGFX_MAX_BINDABLE_IMAGES];
    VkDescriptorImageInfo image_infos[MGFX_MAX_BINDABLE_IMAGES];
    
    for (uint32_t i = 0; i < key.binding_count; i++)
    {
        image_infos[i] = (VkDescriptorImageInfo){
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = key.bindings[i].image_view,
            .sampler     = key.bindings[i].sampler,
        };
        writes[i] = (VkWriteDescriptorSet){
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = cache->sets[new_index],
            .dstBinding      = key.bindings[i].binding,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo      = &image_infos[i],
        };
    }
    vkUpdateDescriptorSets(ctx.vk.device.handle, key.binding_count, writes, 0, NULL);
    
    cache->dirty = false;
    return cache->sets[new_index];
}

static void mgfx_vk_remove_cache_entry(mgfx_vk_descriptor_cache* cache, uint32_t index)
{
    if (cache->cache_size == 0 || index >= cache->cache_size)
        return;
    
    vkFreeDescriptorSets(ctx.vk.device.handle, ctx.vk.descriptor_pool, 1, &cache->sets[index]);
    
    uint32_t last_index = cache->cache_size - 1;
    if (index != last_index)
    {
        cache->cache[index] = cache->cache[last_index];
        cache->sets[index] = cache->sets[last_index];
    }
    
    cache->cache_size--;
}

static void mgfx_vk_descriptor_cache_invalidate_image_view(mgfx_vk_descriptor_cache *cache, VkImageView view)
{
    for (uint32_t i = 0; i < cache->cache_size; i++)
        for (uint32_t j = 0; j < cache->cache[i].key.binding_count; j++)
        if (cache->cache[i].key.bindings[j].image_view == view)
    {
        mgfx_vk_remove_cache_entry(cache, i);
        i--;
        break;
    }
    
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_IMAGES; i++)
        if (cache->bound_image_active[i] && cache->bound_images[i].image_view == view)
    {
        cache->bound_image_active[i] = false;
        cache->dirty = true;
    }
}

static void mgfx_vk_descriptor_cache_invalidate_sampler(mgfx_vk_descriptor_cache *cache, VkSampler sampler)
{
    for (uint32_t i = 0; i < cache->cache_size; i++)
        for (uint32_t j = 0; j < cache->cache[i].key.binding_count; j++)
        if (cache->cache[i].key.bindings[j].sampler == sampler)
    {
        mgfx_vk_remove_cache_entry(cache, i);
        i--;
        break;
    }
    
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_IMAGES; i++)
        if (cache->bound_image_active[i] && cache->bound_images[i].sampler == sampler)
    {
        cache->bound_image_active[i] = false;
        cache->dirty = true;
    }
}

static uint32_t mgfx_vk_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(ctx.vk.physical_device.handle, &mem_properties);
    
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    
    return 0;
}

static void mgfx_vk_allocate_image(uint32_t width, uint32_t height, uint32_t depth, VkImageType type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *memory)
{
    VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = type;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = depth;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult result = vkCreateImage(ctx.vk.device.handle, &image_info, NULL, image);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan image.");
    
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(ctx.vk.device.handle, *image, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = mgfx_vk_find_memory_type(mem_requirements.memoryTypeBits, properties);
    
    result = vkAllocateMemory(ctx.vk.device.handle, &alloc_info, NULL, memory);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to allocated memory for vulkan image.");
    
    vkBindImageMemory(ctx.vk.device.handle, *image, *memory, 0);
}

static VkExtent2D mgfx_vk_choose_swap_extent(const VkSurfaceCapabilitiesKHR *capabilities, int32_t width, int32_t height)
{
    if (capabilities->currentExtent.width != UINT32_MAX)
        return capabilities->currentExtent;
    else
    {
        VkExtent2D actual_extent = {(uint32_t)width, (uint32_t)height};
        actual_extent.width = mgfx_clampi(actual_extent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
        actual_extent.height = mgfx_clampi(actual_extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
        return actual_extent;
    }
}

static VkFormat mgfx_vk_find_supported_format(const VkFormat *candidates, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (uint32_t i = 0; i < count; i++)
    {
        VkFormat format = candidates[i];
        
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx.vk.physical_device.handle, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }
    return VK_FORMAT_UNDEFINED;
}

static void mgfx_vk_create_or_recreate_swapchain(void)
{
    if (ctx.vk.swapchain.depth_format == VK_FORMAT_UNDEFINED)
        ctx.vk.swapchain.depth_format = mgfx_vk_find_supported_format((VkFormat[]){VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 3, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.vk.physical_device.handle, ctx.vk.surface, &capabilities);
    
    VkExtent2D extent = mgfx_vk_choose_swap_extent(&capabilities, ctx.vk.width, ctx.vk.height);
    
    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;
    
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = ctx.vk.surface,
        .minImageCount = image_count,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = ctx.vk.vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = ctx.vk.swapchain.handle
    };
    
    VkResult result = vkCreateSwapchainKHR(ctx.vk.device.handle, &create_info, NULL, &ctx.vk.swapchain.handle);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan swapchain.");
    
    if (create_info.oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ctx.vk.device.handle, ctx.vk.swapchain.depth_image_view, NULL);
        vkDestroyImage(ctx.vk.device.handle, ctx.vk.swapchain.depth_image, NULL);
        vkFreeMemory(ctx.vk.device.handle, ctx.vk.swapchain.depth_image_memory, NULL);
        
        for (uint32_t i = 0; i < ctx.vk.swapchain.image_count; i++)
            vkDestroyImageView(ctx.vk.device.handle, ctx.vk.swapchain.image_views[i], NULL);
        
        vkDestroySwapchainKHR(ctx.vk.device.handle, create_info.oldSwapchain, NULL);
    }
    
    ctx.vk.swapchain.image_count = image_count;
    vkGetSwapchainImagesKHR(ctx.vk.device.handle, ctx.vk.swapchain.handle, &image_count, ctx.vk.swapchain.images);
    
    for (uint32_t i = 0; i < image_count; i++)
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = ctx.vk.swapchain.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = create_info.imageFormat,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1
        };
        
        result = vkCreateImageView(ctx.vk.device.handle, &view_info, NULL, &ctx.vk.swapchain.image_views[i]);
        MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan image view for swapchain.");
    }
    
    mgfx_vk_allocate_image(extent.width, extent.height, 1,
                            VK_IMAGE_TYPE_2D, ctx.vk.swapchain.depth_format, VK_IMAGE_TILING_OPTIMAL,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            &ctx.vk.swapchain.depth_image, &ctx.vk.swapchain.depth_image_memory);
    
    VkImageViewCreateInfo depth_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = ctx.vk.swapchain.depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = ctx.vk.swapchain.depth_format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1
    };
    
    result = vkCreateImageView(ctx.vk.device.handle, &depth_view_info, NULL, &ctx.vk.swapchain.depth_image_view);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan depth image view for the swapchain.");
}

static void mgfx_vk_cleanup_swapchain(void)
{
    for (uint32_t i = 0; i < ctx.vk.swapchain.image_count; i++)
        vkDestroyImageView(ctx.vk.device.handle, ctx.vk.swapchain.image_views[i], NULL);
    
    vkDestroyImageView(ctx.vk.device.handle, ctx.vk.swapchain.depth_image_view, NULL);
    vkDestroyImage(ctx.vk.device.handle, ctx.vk.swapchain.depth_image, NULL);
    vkFreeMemory(ctx.vk.device.handle, ctx.vk.swapchain.depth_image_memory, NULL);
    
    vkDestroySwapchainKHR(ctx.vk.device.handle, ctx.vk.swapchain.handle, NULL);
}

static void mgfx_vk_recycle(void)
{
    while (!mgfx_queue_empty(&ctx.vk.release_queue))
    {
        mgfx_vk_release_queue_entry *entry = (mgfx_vk_release_queue_entry*)mgfx_queue_pop(&ctx.vk.release_queue);
        switch (entry->type)
        {
            case MGFX_VK_RELEASE_QUEUE_ENTRY_BUFFER:
            vkDestroyBuffer(ctx.vk.device.handle, entry->buffer->buffer, NULL);
            vkFreeMemory(ctx.vk.device.handle, entry->buffer->memory, NULL);
            free(entry->buffer);
            break;
            case MGFX_VK_RELEASE_QUEUE_ENTRY_IMAGE:
            vkDestroyImage(ctx.vk.device.handle, entry->image->image, NULL);
            vkFreeMemory(ctx.vk.device.handle, entry->image->memory, NULL);
            mgfx_vk_descriptor_cache_invalidate_image_view(&ctx.vk.descriptor_cache, entry->image->view);
            vkDestroyImageView(ctx.vk.device.handle, entry->image->view, NULL);
            free(entry->image);
            break;
            case MGFX_VK_RELEASE_QUEUE_ENTRY_SAMPLER:
            mgfx_vk_descriptor_cache_invalidate_sampler(&ctx.vk.descriptor_cache, entry->sampler);
            vkDestroySampler(ctx.vk.device.handle, entry->sampler, NULL);
            break;
            case MGFX_VK_RELEASE_QUEUE_ENTRY_PIPELINE:
            vkDestroyPipeline(ctx.vk.device.handle, entry->pipeline->pipeline, NULL);
            vkDestroyPipelineLayout(ctx.vk.device.handle, entry->pipeline->pipeline_layout, NULL);
            free(entry->pipeline);
            break;
        }
    }
}

static void mgfx_vk_resize(uint32_t width, uint32_t height)
{
    ctx.vk.width = width;
    ctx.vk.height = height;
    mgfx_vk_recycle();
}

static void mgfx_vk_vsync(bool vsync)
{
    ctx.vk.vsync = vsync;
    ctx.vk.rebuild_swapchain = true;
}

static VkCommandBuffer mgfx_vk_create_command_buffer(void)
{
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ctx.vk.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    
    VkCommandBuffer command_buffer;
    
    VkResult result = vkAllocateCommandBuffers(ctx.vk.device.handle, &alloc_info, &command_buffer);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to allocate vulkan command buffer.");
    
    return command_buffer;
}

static VkCommandBuffer mgfx_vk_begin_single_time_commands(void)
{
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = ctx.vk.command_pool,
        .commandBufferCount = 1
    };
    
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(ctx.vk.device.handle, &alloc_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(command_buffer, &begin_info);
    
    return command_buffer;
}

static void mgfx_vk_end_single_time_commands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    
    vkQueueSubmit(ctx.vk.device.graphics_compute_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.vk.device.graphics_compute_queue);
    
    vkFreeCommandBuffers(ctx.vk.device.handle, ctx.vk.command_pool, 1, &commandBuffer);
}

static void mgfx_vk_begin_command_buffer(VkCommandBuffer buffer)
{
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    
    VkResult result = vkBeginCommandBuffer(buffer, &begin_info);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to begin vulkan command buffer.");
}

static void mgfx_vk_end_command_buffer(VkCommandBuffer buffer)
{
    VkResult result = vkEndCommandBuffer(buffer);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to end vulkan command buffer.");
}

static void mgfx_vk_free_command_buffer(VkCommandBuffer buffer)
{
    vkFreeCommandBuffers(ctx.vk.device.handle, ctx.vk.command_pool, 1, &buffer);
}

static void mgfx_vk_command_buffer_set_viewport(VkCommandBuffer buffer, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    VkViewport viewport = {
        .width = (float)width,
        .height = (float)height,
        .x = (float)x,
        .y = (float)y,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    
    vkCmdSetViewport(buffer, 0, 1, &viewport);
}

static void mgfx_vk_command_buffer_set_scissor(VkCommandBuffer buffer, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    VkRect2D scissor = {
        .offset.x = x,
        .offset.y = y,
        .extent.width = width,
        .extent.height = height
    };
    
    vkCmdSetScissor(buffer, 0, 1, &scissor);
}

static void mgfx_vk_allocate_buffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory)
{
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    VkResult result = vkCreateBuffer(ctx.vk.device.handle, &buffer_info, NULL, buffer);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan buffer.");
    
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(ctx.vk.device.handle, *buffer, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = mgfx_vk_find_memory_type(mem_requirements.memoryTypeBits, properties)
    };
    
    result = vkAllocateMemory(ctx.vk.device.handle, &alloc_info, NULL, memory);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to allocate memory for vulkan buffer.");
    
    vkBindBufferMemory(ctx.vk.device.handle, *buffer, *memory, 0);
}

static void mgfx_vk_update_buffer(mgfx_vk_buffer *buffer, size_t offset, size_t size, void *data)
{
    if (buffer->is_cpu)
    {
        void *_data;
        vkMapMemory(ctx.vk.device.handle, buffer->memory, offset, size, 0, &_data);
        memcpy(_data, data, size);
        vkUnmapMemory(ctx.vk.device.handle, buffer->memory);
    }
    else
    {
        VkBuffer staging_buffer;
        VkDeviceMemory staging_memory;
        
        mgfx_vk_allocate_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &staging_buffer, &staging_memory);
        
        void *_data;
        vkMapMemory(ctx.vk.device.handle, staging_memory, 0, size, 0, &_data);
        memcpy(_data, data, size);
        vkUnmapMemory(ctx.vk.device.handle, staging_memory);
        
        VkCommandBuffer cmd = mgfx_vk_begin_single_time_commands();
        VkBufferCopy copy_region = { .srcOffset = 0, .dstOffset = offset, .size = size };
        vkCmdCopyBuffer(cmd, staging_buffer, buffer->buffer, 1, &copy_region);
        mgfx_vk_end_single_time_commands(cmd);
        
        vkDestroyBuffer(ctx.vk.device.handle, staging_buffer, NULL);
        vkFreeMemory(ctx.vk.device.handle, staging_memory, NULL);
    }
}

static mgfx_vk_buffer *mgfx_vk_create_buffer(const mgfx_buffer_create_info *create_info)
{
    mgfx_vk_buffer *buffer = (mgfx_vk_buffer*)calloc(1, sizeof(mgfx_vk_buffer));
    
    if (create_info->access == MGFX_ACCESS_GPU)
    {
        mgfx_vk_allocate_buffer(create_info->size,
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | mgfx_vk_get_buffer_usage(create_info->usage),
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &buffer->buffer, &buffer->memory);
    }
    else
    {
        mgfx_vk_allocate_buffer(create_info->size, mgfx_vk_get_buffer_usage(create_info->usage),
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffer->buffer, &buffer->memory);
    }
    
    buffer->is_cpu = create_info->access == MGFX_ACCESS_CPU;
    if (create_info->data)
        mgfx_vk_update_buffer(buffer, 0, create_info->size, create_info->data);
    
    return buffer;
}

static void mgfx_vk_destroy_buffer(mgfx_vk_buffer *buffer)
{
    mgfx_vk_release_queue_entry entry = {
        .buffer = buffer,
        .type = MGFX_VK_RELEASE_QUEUE_ENTRY_BUFFER
    };
    mgfx_queue_push(&ctx.vk.release_queue, &entry);
}

static void mgfx_vk_bind_vertex_buffer(mgfx_vk_buffer *buffer)
{
    VkBuffer vertex_buffers[] = {buffer->buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(ctx.vk.command_buffer, 0, 1, vertex_buffers, offsets);
}

static void mgfx_vk_bind_index_buffer(mgfx_vk_buffer *buffer, mgfx_index_type index_type)
{
    vkCmdBindIndexBuffer(ctx.vk.command_buffer, buffer->buffer, 0, mgfx_vk_get_index_type(index_type));
}

static void mgfx_vk_transition_image_layout_temp(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = mgfx_vk_begin_single_time_commands();
    
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };
    
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        MGFX_ASSERT(false, "Unsupported image layout transition.");
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    
    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
    mgfx_vk_end_single_time_commands(command_buffer);
}

static void mgfx_vk_copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
    VkCommandBuffer command_buffer = mgfx_vk_begin_single_time_commands();
    
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        
        .imageOffset = (VkOffset3D){0, 0, 0},
        .imageExtent = (VkExtent3D){
            width,
            height,
            depth
        }
    };
    
    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    mgfx_vk_end_single_time_commands(command_buffer);
}

static void mgfx_vk_update_image(mgfx_vk_image *image, size_t size, void *data)
{
    const VkDeviceSize image_size = size;
    
    if (image->is_cpu)
    {
        void *_data;
        vkMapMemory(ctx.vk.device.handle, image->memory, 0, image_size, 0, &_data);
        memcpy(_data, data, image_size);
        vkUnmapMemory(ctx.vk.device.handle, image->memory);
        return;
    }
    
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    
    mgfx_vk_allocate_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &staging_buffer, &staging_memory);
    
    void* _data;
    vkMapMemory(ctx.vk.device.handle, staging_memory, 0, image_size, 0, &_data);
    memcpy(_data, data, image_size);
    vkUnmapMemory(ctx.vk.device.handle, staging_memory);
    
    mgfx_vk_transition_image_layout_temp(image->image, image->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    mgfx_vk_copy_buffer_to_image(staging_buffer, image->image, image->width, image->height, image->depth);
    mgfx_vk_transition_image_layout_temp(image->image, image->format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(ctx.vk.device.handle, staging_buffer, NULL);
    vkFreeMemory(ctx.vk.device.handle, staging_memory, NULL);
}

static mgfx_vk_image *mgfx_vk_create_image(const mgfx_image_create_info *create_info)
{
    mgfx_vk_image *image = (mgfx_vk_image*)calloc(1, sizeof(mgfx_vk_image));
    
    VkImageUsageFlags usage_flags =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        mgfx_vk_get_image_usage(create_info->usage);
    
    VkImageTiling tiling;
    VkMemoryPropertyFlags mem_props;
    
    if (create_info->access == MGFX_ACCESS_CPU)
    {
        tiling = VK_IMAGE_TILING_LINEAR;
        mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else
    {
        tiling = VK_IMAGE_TILING_OPTIMAL;
        mem_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    
    const uint32_t depth = create_info->depth ? create_info->depth : 1;
    mgfx_vk_allocate_image(create_info->width, create_info->height, depth, mgfx_vk_get_image_type(create_info->type), mgfx_vk_get_format(create_info->format), tiling, usage_flags, mem_props, &image->image, &image->memory);
    
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image->image,
        .viewType = mgfx_vk_get_image_view_type(create_info->type),
        .format = mgfx_vk_get_format(create_info->format),
        .subresourceRange.aspectMask =
            create_info->usage == MGFX_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT ?
            VK_IMAGE_ASPECT_DEPTH_BIT :
        VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };
    
    VkResult result = vkCreateImageView(ctx.vk.device.handle, &view_info, NULL, &image->view);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan image view.");
    
    image->width  = create_info->width;
    image->height = create_info->height;
    image->depth = depth;
    
    image->format = view_info.format;
    image->is_cpu = create_info->access == MGFX_ACCESS_CPU;
    
    if (create_info->data)
        mgfx_vk_update_image(image, image->width * image->height * depth * mgfx_format_bpp(create_info->format), create_info->data);
    
    return image;
}

static void mgfx_vk_destroy_image(mgfx_vk_image *image)
{
    mgfx_vk_release_queue_entry entry = {
        .image = image,
        .type = MGFX_VK_RELEASE_QUEUE_ENTRY_IMAGE
    };
    mgfx_queue_push(&ctx.vk.release_queue, &entry);
}

static void mgfx_vk_bind_image(mgfx_vk_image *image, VkSampler sampler, uint32_t binding)
{
    if (binding >= MGFX_MAX_BINDABLE_IMAGES)
        return;
    
    mgfx_vk_descriptor_cache *cache = &ctx.vk.descriptor_cache;
    cache->bound_images[binding].image_view = image->view;
    cache->bound_images[binding].sampler = sampler;
    cache->bound_images[binding].binding = binding;
    cache->bound_image_active[binding] = true;
    cache->dirty = true;
}

static VkSampler mgfx_vk_create_sampler(const mgfx_sampler_create_info *create_info)
{
    VkSampler sampler;
    
    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = mgfx_vk_get_filter(create_info->mag_filter);
    samplerInfo.minFilter = mgfx_vk_get_filter(create_info->min_filter);
    
    samplerInfo.addressModeU = mgfx_vk_get_address_mode(create_info->address_mode_u);
    samplerInfo.addressModeV = mgfx_vk_get_address_mode(create_info->address_mode_v);
    samplerInfo.addressModeW = mgfx_vk_get_address_mode(create_info->address_mode_w);
    
    samplerInfo.anisotropyEnable = VK_FALSE;
    
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    VkResult result = vkCreateSampler(ctx.vk.device.handle, &samplerInfo, NULL, &sampler);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan sampler.");
    
    return sampler;
}

static void mgfx_vk_destroy_sampler(VkSampler sampler)
{
    mgfx_vk_release_queue_entry entry = {
        .sampler = sampler,
        .type = MGFX_VK_RELEASE_QUEUE_ENTRY_SAMPLER
    };
    mgfx_queue_push(&ctx.vk.release_queue, &entry);
}

static VkShaderModule mgfx_vk_create_shader(const uint32_t *code, size_t size)
{
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code
    };
    
    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(ctx.vk.device.handle, &create_info, NULL, &shader_module);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan shader module.");
    
    return shader_module;
}

static void mgfx_vk_fill_graphics_pipeline(mgfx_vk_pipeline *pipeline, const mgfx_pipeline_create_info *create_info)
{
    VkShaderModule vertex_shader_module = mgfx_vk_create_shader((const uint32_t*)create_info->shader.vertex.code, create_info->shader.vertex.size);
    VkShaderModule fragment_shader_module = mgfx_vk_create_shader((const uint32_t*)create_info->shader.fragment.code, create_info->shader.fragment.size);
    
    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_shader_module,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_shader_module,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};
    
    static const VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states
    };
    
    VkVertexInputAttributeDescription attribute_descriptions[MGFX_MAX_VERTEX_ATTRIBUTES];
    
    uint32_t attribute_count = 0;
    uint32_t stride = 0;
    for (; attribute_count < MGFX_MAX_VERTEX_ATTRIBUTES && create_info->vertex_attributes[attribute_count]; attribute_count++)
    {
        const mgfx_vertex_format format = create_info->vertex_attributes[attribute_count];
        VkVertexInputAttributeDescription *desc = &attribute_descriptions[attribute_count];
        desc->format = mgfx_vk_get_vertex_format(format);
        desc->location = attribute_count;
        desc->offset = stride;
        desc->binding = 0;
        stride += mgfx_vertex_format_size(format);
    }
    
    VkVertexInputBindingDescription binding_description = {
        .binding = 0,
        .stride = stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = attribute_count,
        .pVertexAttributeDescriptions = attribute_descriptions
    };
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = mgfx_vk_get_primitive_topology(create_info->primitive_topology),
        .primitiveRestartEnable = VK_FALSE
    };
    
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = mgfx_vk_get_cull_mode(create_info->cull_mode),
        .frontFace = mgfx_vk_get_front_face(create_info->front_face),
        .depthBiasEnable = VK_FALSE
    };
    
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .stencilTestEnable = create_info->depth_stencil.stencil_test_enabled
    };

    if (create_info->depth_stencil.depth_compare_op != MGFX_COMPARE_OP_NEVER)
    {
        depth_stencil.depthTestEnable = true;
        depth_stencil.depthWriteEnable = create_info->depth_stencil.depth_write_enabled;
        depth_stencil.depthCompareOp = mgfx_vk_get_compare_op(create_info->depth_stencil.depth_compare_op);
    }
    
    VkFormat color_formats[MGFX_MAX_COLOR_ATTACHMENTS];
    VkPipelineColorBlendAttachmentState color_blend_attachments[MGFX_MAX_COLOR_ATTACHMENTS];
    uint32_t color_attachment_count = 0;
    
    for (; color_attachment_count < MGFX_MAX_COLOR_ATTACHMENTS && create_info->color_formats[color_attachment_count]; color_attachment_count++)
    {
        color_formats[color_attachment_count] = mgfx_vk_get_format(create_info->color_formats[color_attachment_count]);
        
        VkPipelineColorBlendAttachmentState *attachment = &color_blend_attachments[color_attachment_count];
        attachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment->blendEnable = create_info->color_blend.blend_enabled;
        
        if (create_info->color_blend.blend_enabled)
        {
            attachment->srcColorBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.src_color_blend_factor);
            attachment->dstColorBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.dst_color_blend_factor);
            attachment->colorBlendOp        = mgfx_vk_get_blend_op(create_info->color_blend.color_blend_op);
            attachment->srcAlphaBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.src_alpha_blend_factor);
            attachment->dstAlphaBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.dst_alpha_blend_factor);
            attachment->alphaBlendOp        = mgfx_vk_get_blend_op(create_info->color_blend.alpha_blend_op);
        }
    }
    
    if (color_attachment_count == 0)
    {
        color_formats[0] = VK_FORMAT_B8G8R8A8_UNORM;
        color_blend_attachments[0] = (VkPipelineColorBlendAttachmentState){
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = create_info->color_blend.blend_enabled
        };
        if (create_info->color_blend.blend_enabled)
        {
            color_blend_attachments[0].srcColorBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.src_color_blend_factor);
            color_blend_attachments[0].dstColorBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.dst_color_blend_factor);
            color_blend_attachments[0].colorBlendOp        = mgfx_vk_get_blend_op(create_info->color_blend.color_blend_op);
            color_blend_attachments[0].srcAlphaBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.src_alpha_blend_factor);
            color_blend_attachments[0].dstAlphaBlendFactor = mgfx_vk_get_blend_factor(create_info->color_blend.dst_alpha_blend_factor);
            color_blend_attachments[0].alphaBlendOp        = mgfx_vk_get_blend_op(create_info->color_blend.alpha_blend_op);
        }
        color_attachment_count = 1;
    }
    
    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = color_attachment_count,
        .pAttachments = color_blend_attachments
    };
    
    const VkDescriptorSetLayout set_layouts[] = {
        ctx.vk.layouts.scratch_buffer_layout,
        ctx.vk.layouts.image_sampler_layout,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pSetLayouts = set_layouts,
        .setLayoutCount = 2
    };
    
    VkResult result = vkCreatePipelineLayout(ctx.vk.device.handle, &pipeline_layout_info, NULL, &pipeline->pipeline_layout);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan graphics pipeline layout.");
    
    VkPipelineRenderingCreateInfo rendering_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = color_attachment_count,
        .pColorAttachmentFormats = color_formats,
        .depthAttachmentFormat = mgfx_vk_get_format(create_info->depth_stencil.format)
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_create_info,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = pipeline->pipeline_layout
    };
    
    result = vkCreateGraphicsPipelines(ctx.vk.device.handle, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline->pipeline);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan graphics pipeline.");
    
    vkDestroyShaderModule(ctx.vk.device.handle, vertex_shader_module, NULL);
    vkDestroyShaderModule(ctx.vk.device.handle, fragment_shader_module, NULL);
    
    pipeline->bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

static void mgfx_vk_fill_compute_pipeline(mgfx_vk_pipeline *pipeline, const mgfx_pipeline_create_info *create_info)
{
    VkShaderModule compute_shader = mgfx_vk_create_shader((const uint32_t*)create_info->shader.compute.code, create_info->shader.compute.size);
    
    const VkDescriptorSetLayout set_layouts[] = {
        ctx.vk.layouts.scratch_buffer_layout,
        ctx.vk.layouts.image_sampler_layout
    };
    
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 2,
        .pSetLayouts = set_layouts
    };
    
    VkResult result = vkCreatePipelineLayout(
                                                ctx.vk.device.handle, &layout_info, NULL, &pipeline->pipeline_layout);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan compute pipeline layout.");
    
    VkPipelineShaderStageCreateInfo stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader,
        .pName = "main"
    };
    
    VkComputePipelineCreateInfo compute_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = stage,
        .layout = pipeline->pipeline_layout
    };
    
    result = vkCreateComputePipelines(
                                        ctx.vk.device.handle, VK_NULL_HANDLE, 1, &compute_info, NULL, &pipeline->pipeline);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan compute pipeline.");
    
    vkDestroyShaderModule(ctx.vk.device.handle, compute_shader, NULL);
    pipeline->bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
}

mgfx_vk_pipeline *mgfx_vk_create_pipeline(const mgfx_pipeline_create_info *create_info)
{
    mgfx_vk_pipeline *pipeline = (mgfx_vk_pipeline*)calloc(1, sizeof(mgfx_vk_pipeline));
    if (create_info->shader.compute.size)
        mgfx_vk_fill_compute_pipeline(pipeline, create_info);
    else mgfx_vk_fill_graphics_pipeline(pipeline, create_info);
    return pipeline;
}

void mgfx_vk_destroy_pipeline(mgfx_vk_pipeline *pipeline)
{
    mgfx_vk_release_queue_entry entry = {
        .pipeline = pipeline,
        .type = MGFX_VK_RELEASE_QUEUE_ENTRY_PIPELINE
    };
    mgfx_queue_push(&ctx.vk.release_queue, &entry);
}

void mgfx_vk_bind_pipeline(mgfx_vk_pipeline *pipeline)
{
    vkCmdBindPipeline(ctx.vk.command_buffer, pipeline->bind_point, pipeline->pipeline);
    ctx.vk.current_pipeline = pipeline;
}

static void mgfx_vk_transition_image_layout(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, int layer_count)
{
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = (uint32_t)layer_count,
        },
    };
    
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;
        src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else
    {
        MGFX_ASSERT(false, "Unsupported vulkan image layout transition.");
        return;
    }
    
    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static void mgfx_vk_bind_pass(const mgfx_pass_info *pass)
{
    VkCommandBuffer cmd = ctx.vk.command_buffer;

    if (ctx.vk.inside_pass)
    {
        vkCmdEndRendering(cmd);

        for (uint32_t i = 0; i < ctx.vk.current_pass.color_image_count; i++)
            mgfx_vk_transition_image_layout(cmd, ctx.vk.current_pass.color_images[i]->image, ctx.vk.current_pass.color_images[i]->format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

        if (ctx.vk.current_pass.depth_image)
            mgfx_vk_transition_image_layout(cmd, ctx.vk.current_pass.depth_image->image, ctx.vk.current_pass.depth_image->format, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
    }
    else
        ctx.vk.inside_pass = true;

    VkRenderingAttachmentInfo color_attachments[MGFX_MAX_COLOR_ATTACHMENTS];
    uint32_t actual_width  = ctx.vk.width;
    uint32_t actual_height = ctx.vk.height;
    uint32_t color_count = 0;

    if (!mgfx_valid_pass(pass))
    {
        mgfx_vk_transition_image_layout(cmd, ctx.vk.swapchain.images[ctx.vk.swapchain.image_index], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

        color_attachments[0] = (VkRenderingAttachmentInfo){
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = ctx.vk.swapchain.image_views[ctx.vk.swapchain.image_index],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue.color.float32 = { pass->clear.r, pass->clear.g, pass->clear.b, pass->clear.a }
        };

        ctx.vk.current_pass.color_image_count = 0;
        ctx.vk.current_pass.depth_image = NULL;

        VkRenderingInfo rendering_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { .offset = {0, 0}, .extent = {actual_width, actual_height} },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = color_attachments,
            .pDepthAttachment = NULL,
        };

        vkCmdBeginRendering(cmd, &rendering_info);
        mgfx_vk_command_buffer_set_viewport(cmd, 0, 0, actual_width, actual_height);
        mgfx_vk_command_buffer_set_scissor(cmd, 0, 0, actual_width, actual_height);
        return;
    }

    for (; color_count < MGFX_MAX_COLOR_ATTACHMENTS && pass->color_images[color_count]; color_count++)
    {
        mgfx_vk_image *color_image = (mgfx_vk_image*)pass->color_images[color_count];

        mgfx_vk_transition_image_layout(cmd, color_image->image, color_image->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

        color_attachments[color_count] = (VkRenderingAttachmentInfo){
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = color_image->view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue.color.float32 = { pass->clear.r, pass->clear.g, pass->clear.b, pass->clear.a }
        };

        if (color_count == 0)
        {
            actual_width  = color_image->width;
            actual_height = color_image->height;
        }
        else
            MGFX_ASSERT(color_image->width == actual_width && color_image->height == actual_height, "Mismatched color attachment sizes");

        ctx.vk.current_pass.color_images[color_count] = color_image;
    }
    ctx.vk.current_pass.color_image_count = color_count;

    VkRenderingAttachmentInfo depth_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO
    };
    VkRenderingAttachmentInfo *depth_attachment_ptr = NULL;

    mgfx_vk_image *depth_image = (mgfx_vk_image*)pass->depth_stencil_image;
    if (depth_image)
    {
        mgfx_vk_transition_image_layout(cmd, depth_image->image, depth_image->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

        depth_attachment.imageView = depth_image->view;
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.clearValue.depthStencil = (VkClearDepthStencilValue){1.0f, 0};

        depth_attachment_ptr = &depth_attachment;
        ctx.vk.current_pass.depth_image = depth_image;

        if (color_count == 0)
        {
            actual_width  = depth_image->width;
            actual_height = depth_image->height;
        }
        else
            MGFX_ASSERT(depth_image->width == actual_width && depth_image->height == actual_height, "Depth attachment size mismatch");
    }
    else ctx.vk.current_pass.depth_image = NULL;

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .offset = {0, 0}, .extent = {actual_width, actual_height} },
        .layerCount = 1,
        .colorAttachmentCount = color_count,
        .pColorAttachments = color_attachments,
        .pDepthAttachment = depth_attachment_ptr,
    };

    vkCmdBeginRendering(cmd, &rendering_info);

    mgfx_vk_command_buffer_set_viewport(cmd, 0, 0, actual_width, actual_height);
    mgfx_vk_command_buffer_set_scissor(cmd, 0, 0, actual_width, actual_height);
}

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define MGFX_VULKAN_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#include <vulkan/vulkan_win32.h>
void mgfx_vk_create_surface(mgfx_platform_handle *handle)
{
    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    create_info.hwnd = (HWND)handle->win32.hwnd;
    create_info.hinstance = (HINSTANCE)handle->win32.instance;
    
    VkResult result = vkCreateWin32SurfaceKHR(ctx.vk.instance, &create_info, NULL, &ctx.vk.surface);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan surface.");
}
#elif defined(__linux__)
#include <vulkan/vulkan_xlib.h>
#define VK_USE_PLATFORM_XLIB_KHR
#define MGFX_VULKAN_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
void mgfx_vk_create_surface(mgfx_platform_handle *handle)
{
    VkXlibSurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
    create_info.dpy = (Display*)handle->xlib.display;
    create_info.window = (Window)handle->xlib.window;
    
    VkResult result = vkCreateXlibSurfaceKHR(ctx.vk.instance, &create_info, NULL, &ctx.vk.surface);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan surface.");
}
#endif

static void mgfx_vk_create_instance(void)
{
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_3,
        .pApplicationName = NULL,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Magma",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    };

    const char *instance_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        MGFX_VULKAN_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = instance_extensions
    };
    
    VkResult result = vkCreateInstance(&create_info, NULL, &ctx.vk.instance);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan instance.");
}

static void mgfx_vk_get_physical_device(void)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(ctx.vk.instance, &device_count, NULL);
    MGFX_ASSERT(device_count != 0, "Failed to get vulkan physical devices.");
    
    VkPhysicalDevice *devices = (VkPhysicalDevice*)malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(ctx.vk.instance, &device_count, devices);
    
    int32_t best_score = -1;
    for (uint32_t i = 0; i < device_count; i++)
    {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        vkGetPhysicalDeviceFeatures(devices[i], &features);
        
        int32_t score = 0;
        
        switch (props.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 1000;
            break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 100;
            break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 50;
            break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 10;
            break;
            default:
            continue;
        }
        
        if (score > best_score)
        {
            best_score = score;
            ctx.vk.physical_device.properties = props;
            ctx.vk.physical_device.features = features;
            ctx.vk.physical_device.handle = devices[i];
        }
    }
    
    free(devices);
    
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.vk.physical_device.handle, &queue_family_count, NULL);
    
    VkQueueFamilyProperties *queue_families = (VkQueueFamilyProperties*)malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.vk.physical_device.handle, &queue_family_count, queue_families);
    
    for (uint32_t i = 0; i < queue_family_count; i++)
        if (queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
    {
        ctx.vk.physical_device.queue_family = i;
        break;
    }
    
    free(queue_families);
}

static void mgfx_vk_create_device(void)
{
    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = ctx.vk.physical_device.queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    VkPhysicalDeviceFeatures device_features = {0};
    
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = VK_TRUE,
    };
    
    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamic_rendering,
        .pQueueCreateInfos = &queue_create_info,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &device_features
    };

    const char *device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = device_extensions;
    
    VkResult result = vkCreateDevice(ctx.vk.physical_device.handle, &create_info, NULL, &ctx.vk.device.handle);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan logical device.");
    
    vkGetDeviceQueue(ctx.vk.device.handle, ctx.vk.physical_device.queue_family, 0, &ctx.vk.device.graphics_compute_queue);
}

static void mgfx_vk_create_command_pool(void)
{
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx.vk.physical_device.queue_family
    };
    
    VkResult result = vkCreateCommandPool(ctx.vk.device.handle, &pool_info, NULL, &ctx.vk.command_pool);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan command pool.");
}

static void mgfx_vk_create_sync_objects(void)
{
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    VkResult result = vkCreateSemaphore(ctx.vk.device.handle, &semaphore_info, NULL, &ctx.vk.sync_objects.image_available_semaphore);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan avaliable image semaphore.");
    
    result = vkCreateSemaphore(ctx.vk.device.handle, &semaphore_info, NULL, &ctx.vk.sync_objects.image_rendered_semaphore);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan rendered image semaphore.");
    
    result = vkCreateFence(ctx.vk.device.handle, &fence_info, NULL, &ctx.vk.sync_objects.fence);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan fence.");
}

static void mgfx_vk_create_descriptor_pool(void)
{
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MGFX_MAX_BINDABLE_UNIFORMS},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MGFX_MAX_DESCRIPTOR_CACHE * MGFX_MAX_BINDABLE_IMAGES}
    };
    
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .poolSizeCount = 2,
        .pPoolSizes = pool_sizes,
        .maxSets = MGFX_MAX_BINDABLE_UNIFORMS + MGFX_MAX_DESCRIPTOR_CACHE
    };

    VkResult result = vkCreateDescriptorPool(ctx.vk.device.handle, &pool_info, NULL, &ctx.vk.descriptor_pool);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan descriptor pool.");
}

static void mgfx_vk_create_descriptor_set_layouts(void)
{
    VkDescriptorSetLayoutBinding scratch_buffer_layout_bindings[MGFX_MAX_BINDABLE_UNIFORMS];
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_UNIFORMS; i++)
    {
        scratch_buffer_layout_bindings[i] = (VkDescriptorSetLayoutBinding) {
            .binding = i,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        };
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = MGFX_MAX_BINDABLE_UNIFORMS,
        .pBindings = scratch_buffer_layout_bindings
    };
    
    VkResult result = vkCreateDescriptorSetLayout(ctx.vk.device.handle, &layout_info, NULL, &ctx.vk.layouts.scratch_buffer_layout);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan uniform buffer descriptor set layout.");
    
    VkDescriptorSetLayoutBinding image_sampler_layout_bindings[MGFX_MAX_BINDABLE_IMAGES];
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_IMAGES; i++)
    {
        image_sampler_layout_bindings[i] = (VkDescriptorSetLayoutBinding) {
            .binding = i,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };
    }
    
    layout_info.bindingCount = MGFX_MAX_BINDABLE_IMAGES;
    layout_info.pBindings = image_sampler_layout_bindings;
    
    result = vkCreateDescriptorSetLayout(ctx.vk.device.handle, &layout_info, NULL, &ctx.vk.layouts.image_sampler_layout);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to create vulkan combined image sampler descriptor set layout.");
}

static void mgfx_vk_create_scratch_buffer(void)
{
    mgfx_vk_allocate_buffer(MGFX_MAX_SCRATCH_BUFFER_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &ctx.vk.scratch_buffer.buffer, &ctx.vk.scratch_buffer.memory);
    vkMapMemory(ctx.vk.device.handle, ctx.vk.scratch_buffer.memory, 0, MGFX_MAX_SCRATCH_BUFFER_SIZE, 0, (void**)&ctx.vk.scratch_buffer.data);
    
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = ctx.vk.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &ctx.vk.layouts.scratch_buffer_layout
    };
    
    VkResult result = vkAllocateDescriptorSets(ctx.vk.device.handle, &alloc_info, &ctx.vk.scratch_buffer.ub_set);
    MGFX_ASSERT(result == VK_SUCCESS, "Failed to allocate vulkan descriptor sets.");
    
    VkWriteDescriptorSet writes[MGFX_MAX_BINDABLE_UNIFORMS];
    VkDescriptorBufferInfo buffer_infos[MGFX_MAX_BINDABLE_UNIFORMS];
    
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_UNIFORMS; i++)
    {
        buffer_infos[i] = (VkDescriptorBufferInfo){
            .buffer = ctx.vk.scratch_buffer.buffer,
            .offset = 0,
            .range = MGFX_MAX_UNIFORM_UPDATE_SIZE
        };

        writes[i] = (VkWriteDescriptorSet){
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = ctx.vk.scratch_buffer.ub_set,
            .dstBinding = i,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .pBufferInfo = &buffer_infos[i]
        };
    }
    
    vkUpdateDescriptorSets(ctx.vk.device.handle, MGFX_MAX_BINDABLE_UNIFORMS, writes, 0, NULL);
}

static void mgfx_vk_init(const mgfx_init_info *init_info)
{
    ctx.vk = (mgfx_vk_context){ 0 };
    
    mgfx_vk_create_instance();
    mgfx_vk_create_surface((mgfx_platform_handle*)init_info->handle);
    
    mgfx_vk_get_physical_device();
    mgfx_vk_create_device();
    
    mgfx_vk_create_command_pool();
    ctx.vk.command_buffer = mgfx_vk_create_command_buffer();
    
    mgfx_vk_create_sync_objects();
    
    ctx.vk.width = init_info->width;
    ctx.vk.height = init_info->height;
    ctx.vk.vsync = init_info->vsync;
    mgfx_vk_create_or_recreate_swapchain();
    
    mgfx_vk_create_descriptor_set_layouts();
    mgfx_vk_create_descriptor_pool();
    
    mgfx_vk_create_scratch_buffer();
    
    mgfx_create_queue(&ctx.vk.release_queue, sizeof(mgfx_vk_release_queue_entry), MGFX_MAX_DEVICE_ALLOCATIONS);
}

static void mgfx_vk_shutdown(void)
{
    vkDeviceWaitIdle(ctx.vk.device.handle);
    
    mgfx_vk_recycle();
    mgfx_destroy_queue(&ctx.vk.release_queue);
    
    vkDestroyDescriptorSetLayout(ctx.vk.device.handle, ctx.vk.layouts.image_sampler_layout, NULL);
    vkDestroyDescriptorSetLayout(ctx.vk.device.handle, ctx.vk.layouts.scratch_buffer_layout, NULL);
    
    vkDestroyBuffer(ctx.vk.device.handle, ctx.vk.scratch_buffer.buffer, NULL);
    vkFreeMemory(ctx.vk.device.handle, ctx.vk.scratch_buffer.memory, NULL);
    vkFreeDescriptorSets(ctx.vk.device.handle, ctx.vk.descriptor_pool, 1, &ctx.vk.scratch_buffer.ub_set);
    
    vkDestroyDescriptorPool(ctx.vk.device.handle, ctx.vk.descriptor_pool, NULL);
    
    mgfx_vk_cleanup_swapchain();
    
    vkDestroySemaphore(ctx.vk.device.handle, ctx.vk.sync_objects.image_available_semaphore, NULL);
    vkDestroySemaphore(ctx.vk.device.handle, ctx.vk.sync_objects.image_rendered_semaphore, NULL);
    vkDestroyFence(ctx.vk.device.handle, ctx.vk.sync_objects.fence, NULL);
    
    mgfx_vk_free_command_buffer(ctx.vk.command_buffer);
    vkDestroyCommandPool(ctx.vk.device.handle, ctx.vk.command_pool, NULL);
    
    vkDestroyDevice(ctx.vk.device.handle, NULL);
    
    vkDestroySurfaceKHR(ctx.vk.instance, ctx.vk.surface, NULL);
    vkDestroyInstance(ctx.vk.instance, NULL);
}

static mgfx_result mgfx_vk_begin(void)
{
    if (ctx.vk.rebuild_swapchain)
    {
        vkDeviceWaitIdle(ctx.vk.device.handle);
        mgfx_vk_create_or_recreate_swapchain();
        ctx.vk.rebuild_swapchain = false;
    }
    
    vkWaitForFences(ctx.vk.device.handle, 1, &ctx.vk.sync_objects.fence, VK_TRUE, UINT64_MAX);
    VkResult result = vkAcquireNextImageKHR(ctx.vk.device.handle, ctx.vk.swapchain.handle, UINT64_MAX, ctx.vk.sync_objects.image_available_semaphore, VK_NULL_HANDLE, &ctx.vk.swapchain.image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        ctx.vk.rebuild_swapchain = true;
        return MGFX_RESULT_FAILURE;
    }
    
    vkResetFences(ctx.vk.device.handle, 1, &ctx.vk.sync_objects.fence);
    vkResetCommandBuffer(ctx.vk.command_buffer, 0);
    
    mgfx_vk_begin_command_buffer(ctx.vk.command_buffer);
    
    ctx.vk.current_pass.color_image_count = 0;
    ctx.vk.current_pass.depth_image = NULL;
    ctx.vk.scratch_buffer.offset = 0;
    ctx.vk.inside_pass = false;
    ctx.vk.descriptor_cache.dirty = false;

    return MGFX_RESULT_SUCCESS;
}

static void mgfx_vk_end(void)
{
    VkCommandBuffer cmd = ctx.vk.command_buffer;
    
    if (ctx.vk.inside_pass)
    {
        vkCmdEndRendering(cmd);
        mgfx_vk_transition_image_layout(
            cmd,
            ctx.vk.swapchain.images[ctx.vk.swapchain.image_index],
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
    }
    
    mgfx_vk_end_command_buffer(cmd);
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };
    
    VkSemaphore wait_semaphores[] = {ctx.vk.sync_objects.image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    
    VkSemaphore signal_semaphores[] = {ctx.vk.sync_objects.image_rendered_semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    
    vkQueueSubmit(ctx.vk.device.graphics_compute_queue, 1, &submit_info, ctx.vk.sync_objects.fence);
    
    VkSwapchainKHR swapchains[] = {ctx.vk.swapchain.handle};

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &ctx.vk.swapchain.image_index
    };

    VkResult result = vkQueuePresentKHR(ctx.vk.device.graphics_compute_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        ctx.vk.rebuild_swapchain = true;
    
    mgfx_vk_recycle();
}

static void mgfx_vk_viewport(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    mgfx_vk_command_buffer_set_viewport(ctx.vk.command_buffer, x, y, width, height);
}

static void mgfx_vk_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    mgfx_vk_command_buffer_set_scissor(ctx.vk.command_buffer, x, y, width, height);
}

static void mgfx_vk_draw_instanced(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    VkDescriptorSet set = mgfx_vk_commit_image_descriptor_cache(&ctx.vk.descriptor_cache);
    if (set != VK_NULL_HANDLE)
        vkCmdBindDescriptorSets(ctx.vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.vk.current_pipeline->pipeline_layout, 1, 1, &set, 0, NULL);
    vkCmdDraw(ctx.vk.command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

static void mgfx_vk_draw_indexed_instanced(uint32_t index_count, uint32_t first_index, int32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    VkDescriptorSet set = mgfx_vk_commit_image_descriptor_cache(&ctx.vk.descriptor_cache);
    if (set != VK_NULL_HANDLE)
        vkCmdBindDescriptorSets(ctx.vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.vk.current_pipeline->pipeline_layout, 1, 1, &set, 0, NULL);
    vkCmdDrawIndexed(ctx.vk.command_buffer, index_count, instance_count, first_index, first_vertex, first_instance);
}

static void mgfx_vk_draw(uint32_t vertex_count, uint32_t first_vertex)
{
    mgfx_vk_draw_instanced(vertex_count, first_vertex, 1, 0);
}

static void mgfx_vk_draw_indexed(uint32_t index_count, uint32_t first_index, int32_t first_vertex)
{
    mgfx_vk_draw_indexed_instanced(index_count, first_index, first_vertex, 1, 0);
}

static void mgfx_vk_dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    vkCmdDispatch(ctx.vk.command_buffer, x, y, z);
}

static void mgfx_vk_bind_uniforms(uint32_t binding, size_t size, void *data)
{
    const uint32_t alignment = (const uint32_t)ctx.vk.physical_device.properties.limits.minUniformBufferOffsetAlignment;
    memcpy(ctx.vk.scratch_buffer.data + ctx.vk.scratch_buffer.offset, data, size);
    ctx.vk.scratch_buffer.bind_offsets[binding] = ctx.vk.scratch_buffer.offset;
    ctx.vk.scratch_buffer.offset = mgfx_stride_align(ctx.vk.scratch_buffer.offset + (uint32_t)size, alignment);
    
    vkCmdBindDescriptorSets(ctx.vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.vk.current_pipeline->pipeline_layout, 0, 1, &ctx.vk.scratch_buffer.ub_set, MGFX_MAX_BINDABLE_UNIFORMS, ctx.vk.scratch_buffer.bind_offsets);
}

#endif // MGFX_VULKAN

#if defined(MGFX_OPENGL)

#if defined(__EMSCRIPTEN__)

void _mgfx_gl_load_platform(void)
{
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.alpha = EM_TRUE;
    attrs.depth = EM_TRUE;
    attrs.stencil = EM_TRUE;
    attrs.antialias = EM_TRUE;
    attrs.enableExtensionsByDefault = EM_TRUE;
    
    ctx.emscripten.context = emscripten_webgl_create_context("#canvas", &attrs);
    if (ctx.emscripten.context <= 0)
    {
        emscripten_log(EM_LOG_ERROR, "Failed to create WebGL2 context");
        return;
    }
    
    emscripten_webgl_make_context_current(ctx.emscripten.context);
}

void _mgfx_gl_unload_platform(void)
{
    if (!ctx.emscripten.context)
        return;
    emscripten_webgl_destroy_context(ctx.emscripten.context);
    ctx.emscripten.context = 0;
}

void _mgfx_gl_swapbuffers(void)
{
    
}

void _mgfx_gl_set_swap_interval(bool enabled)
{
    
}

#else

#define _MGFX_GL_FUNCS \
_MGFX_XMACRO(glGetUniformBlockIndex,    GLuint, (GLuint program, const GLchar* uniformBlockName)) \
_MGFX_XMACRO(glUniformBlockBinding,     void,   (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)) \
_MGFX_XMACRO(glClearColor,              void,   (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) \
_MGFX_XMACRO(glClear,                   void,   (GLbitfield mask)) \
_MGFX_XMACRO(glBindVertexArray,         void,   (GLuint array)) \
_MGFX_XMACRO(glGenFramebuffers,         void,   (GLsizei n, GLuint* framebuffers)) \
_MGFX_XMACRO(glBindFramebuffer,         void,   (GLenum target, GLuint framebuffer)) \
_MGFX_XMACRO(glUniform1iv,              void,   (GLint location, GLsizei count, const GLint* value)) \
_MGFX_XMACRO(glUseProgram,              void,   (GLuint program)) \
_MGFX_XMACRO(glShaderSource,            void,   (GLuint shader, GLsizei count, const GLchar *const* string, const GLint* length)) \
_MGFX_XMACRO(glLinkProgram,             void,   (GLuint program)) \
_MGFX_XMACRO(glGetUniformLocation,      GLint,  (GLuint program, const GLchar* name)) \
_MGFX_XMACRO(glDeleteShader,            void,   (GLuint shader)) \
_MGFX_XMACRO(glDeleteProgram,           void,   (GLuint program)) \
_MGFX_XMACRO(glCompileShader,           void,   (GLuint shader)) \
_MGFX_XMACRO(glGenBuffers,              void,   (GLsizei n, GLuint* buffers)) \
_MGFX_XMACRO(glActiveTexture,           void,   (GLenum texture)) \
_MGFX_XMACRO(glGenTextures,             void,   (GLsizei n, GLuint* textures)) \
_MGFX_XMACRO(glDrawElements,            void,   (GLenum mode, GLsizei count, GLenum type, const void* indices)) \
_MGFX_XMACRO(glDeleteFramebuffers,      void,   (GLsizei n, const GLuint* framebuffers)) \
_MGFX_XMACRO(glBlendEquationSeparate,   void,   (GLenum modeRGB, GLenum modeAlpha)) \
_MGFX_XMACRO(glDeleteTextures,          void,   (GLsizei n, const GLuint* textures)) \
_MGFX_XMACRO(glBindTexture,             void,   (GLenum target, GLuint texture)) \
_MGFX_XMACRO(glTexImage2D,              void,   (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)) \
_MGFX_XMACRO(glTexImage3D,              void,   (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels)) \
_MGFX_XMACRO(glTexSubImage2D,           void,   (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)) \
_MGFX_XMACRO(glTexSubImage3D,           void,   (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels)) \
_MGFX_XMACRO(glCreateShader,            GLuint, (GLenum type)) \
_MGFX_XMACRO(glFramebufferTexture2D,    void,   (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
_MGFX_XMACRO(glCreateProgram,           GLuint, (void)) \
_MGFX_XMACRO(glViewport,                void,   (GLint x, GLint y, GLsizei width, GLsizei height)) \
_MGFX_XMACRO(glDeleteBuffers,           void,   (GLsizei n, const GLuint* buffers)) \
_MGFX_XMACRO(glDrawArrays,              void,   (GLenum mode, GLint first, GLsizei count)) \
_MGFX_XMACRO(glDrawElementsInstanced,   void,   (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount)) \
_MGFX_XMACRO(glVertexAttribPointer,     void,   (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)) \
_MGFX_XMACRO(glVertexAttribIPointer,    void,   (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer)) \
_MGFX_XMACRO(glDisable,                 void,   (GLenum cap)) \
_MGFX_XMACRO(glBindBuffer,              void,   (GLenum target, GLuint buffer)) \
_MGFX_XMACRO(glDeleteVertexArrays,      void,   (GLsizei n, const GLuint* arrays)) \
_MGFX_XMACRO(glDepthMask,               void,   (GLboolean flag)) \
_MGFX_XMACRO(glDrawArraysInstanced,     void,   (GLenum mode, GLint first, GLsizei count, GLsizei instancecount)) \
_MGFX_XMACRO(glScissor,                 void,   (GLint x, GLint y, GLsizei width, GLsizei height)) \
_MGFX_XMACRO(glBufferData,              void,   (GLenum target, GLsizeiptr size, const void* data, GLenum usage)) \
_MGFX_XMACRO(glBufferSubData,           void,   (GLenum target, GLintptr offset, GLsizeiptr size, const void* data)) \
_MGFX_XMACRO(glBlendFuncSeparate,       void,   (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)) \
_MGFX_XMACRO(glTexParameteri,           void,   (GLenum target, GLenum pname, GLint param)) \
_MGFX_XMACRO(glEnable,                  void,   (GLenum cap)) \
_MGFX_XMACRO(glAttachShader,            void,   (GLuint program, GLuint shader)) \
_MGFX_XMACRO(glDepthFunc,               void,   (GLenum func)) \
_MGFX_XMACRO(glEnableVertexAttribArray, void,   (GLuint index)) \
_MGFX_XMACRO(glBlendFunc,               void,   (GLenum sfactor, GLenum dfactor)) \
_MGFX_XMACRO(glGenVertexArrays,         void,   (GLsizei n, GLuint* arrays)) \
_MGFX_XMACRO(glFrontFace,               void,   (GLenum mode)) \
_MGFX_XMACRO(glCullFace,                void,   (GLenum mode)) \
_MGFX_XMACRO(glBindBufferBase,          void,   (GLenum target, GLuint index, GLuint buffer)) \
_MGFX_XMACRO(glDispatchCompute,         void,   (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)) \
_MGFX_XMACRO(glDrawElementsBaseVertex,          void, (GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex)) \
_MGFX_XMACRO(glDrawElementsInstancedBaseVertex, void, (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex)) \
_MGFX_XMACRO(glDrawBuffers,             void,   (GLsizei n, const GLenum* bufs))

#define _MGFX_XMACRO(name, ret, args) typedef ret (GL_APIENTRY* PFN_ ## name) args;
_MGFX_GL_FUNCS
#undef _MGFX_XMACRO

#define _MGFX_XMACRO(name, ret, args) static PFN_ ## name name;
_MGFX_GL_FUNCS
#undef _MGFX_XMACRO

#if defined(_WIN32)

#pragma comment(lib, "gdi32")

typedef PROC (WINAPI * _mgfx_wglGetProcAddress)(LPCSTR);
static void* _mgfx_gl_getprocaddr(const char* name, _mgfx_wglGetProcAddress wgl_getprocaddress)
{
    void *proc_addr = (void*) wgl_getprocaddress(name);
    if (!proc_addr)
        proc_addr = (void*) GetProcAddress(ctx.gl.wgl.opengl32_dll, name);
    return proc_addr;
}

static void _mgfx_gl_load_opengl(void)
{
    ctx.gl.wgl.opengl32_dll = LoadLibraryA("opengl32.dll");
    _mgfx_wglGetProcAddress wgl_getprocaddress = (_mgfx_wglGetProcAddress) GetProcAddress(ctx.gl.wgl.opengl32_dll, "wglGetProcAddress");
#define _MGFX_XMACRO(name, ret, args) name = (PFN_ ## name) _mgfx_gl_getprocaddr(#name, wgl_getprocaddress);
    _MGFX_GL_FUNCS
#undef _MGFX_XMACRO
}

static void _mgfx_gl_unload_opengl(void)
{
    FreeLibrary(ctx.gl.wgl.opengl32_dll);
}

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001

typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;

static void _mgfx_gl_load_platform(mgfx_platform_handle *handle)
{
    ctx.gl.wgl.hwnd = (HWND)handle->win32.hwnd;
    ctx.gl.wgl.hdc = GetDC(ctx.gl.wgl.hwnd);

    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32
    };

    INT format = ChoosePixelFormat(ctx.gl.wgl.hdc, &pfd);
    SetPixelFormat(ctx.gl.wgl.hdc, format, &pfd);

    HGLRC temp_rc = wglCreateContext(ctx.gl.wgl.hdc);
    wglMakeCurrent(ctx.gl.wgl.hdc, temp_rc);

    typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    const int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    ctx.gl.wgl.hrc = wglCreateContextAttribsARB(ctx.gl.wgl.hdc, NULL, attribs);

    wglMakeCurrent(ctx.gl.wgl.hdc, ctx.gl.wgl.hrc);
    wglDeleteContext(temp_rc);

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
}

static void _mgfx_gl_unload_platform(void)
{
    wglMakeCurrent(0, 0);
    wglDeleteContext(ctx.gl.wgl.hrc);
    ReleaseDC(ctx.gl.wgl.hwnd, ctx.gl.wgl.hdc);
}

static void _mgfx_gl_swapbuffers(void)
{
    SwapBuffers(ctx.gl.wgl.hdc);
}

static void _mgfx_gl_set_swap_interval(bool enabled)
{
    if (wglSwapIntervalEXT)
        wglSwapIntervalEXT(enabled);
}

#elif defined(__linux__)

static void _mgfx_gl_load_opengl(void)
{
#define _MGFX_XMACRO(name, ret, args) name = (PFN_##name)eglGetProcAddress(#name);
    _MGFX_GL_FUNCS
#undef _MGFX_XMACRO
}

static void _mgfx_gl_unload_opengl(void)
{
    
}

static PFNEGLSWAPINTERVALPROC _mgfx_eglSwapInterval = NULL;

static void _mgfx_gl_load_platform(mgfx_platform_handle *handle)
{
    ctx.gl.egl.display = eglGetDisplay((EGLNativeDisplayType)handle->xlib.display);
    eglInitialize(ctx.gl.egl.display, NULL, NULL);
    eglBindAPI(EGL_OPENGL_API);
    
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
        EGL_NONE
    };
    
    EGLConfig config;
    EGLint num_configs;
    eglChooseConfig(ctx.gl.egl.display, config_attribs, &config, 1, &num_configs);
    
    EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 5,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    
    ctx.gl.egl.context = eglCreateContext(ctx.gl.egl.display, config, EGL_NO_CONTEXT, context_attribs);
    ctx.gl.egl.surface = eglCreateWindowSurface(ctx.gl.egl.display, config, (EGLNativeWindowType)handle->xlib.window, NULL);
    eglMakeCurrent(ctx.gl.egl.display, ctx.gl.egl.surface, ctx.gl.egl.surface, ctx.gl.egl.context);
    
    _mgfx_eglSwapInterval = (PFNEGLSWAPINTERVALPROC)eglGetProcAddress("eglSwapInterval");
}

static void _mgfx_gl_unload_platform(void)
{
    eglMakeCurrent(ctx.gl.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(ctx.gl.egl.display, ctx.gl.egl.context);
    eglDestroySurface(ctx.gl.egl.display, ctx.gl.egl.surface);
    eglTerminate(ctx.gl.egl.display);
}

static void _mgfx_gl_swapbuffers(void)
{
    eglSwapBuffers(ctx.gl.egl.display, ctx.gl.egl.surface);
}

static void _mgfx_gl_set_swap_interval(bool enabled)
{
    if (_mgfx_eglSwapInterval)
        _mgfx_eglSwapInterval(ctx.gl.egl.display, enabled);
}
#endif
#endif

static inline GLenum mgfx_gl_get_index_type(mgfx_index_type index_type)
{
    if (index_type == MGFX_INDEX_TYPE_UINT16)
        return GL_UNSIGNED_SHORT;
    return GL_UNSIGNED_INT;
}

static inline GLenum mgfx_gl_get_buffer_target(mgfx_buffer_usage usage)
{
    switch (usage)
    {
        case MGFX_BUFFER_USAGE_VERTEX: return GL_ARRAY_BUFFER;
        case MGFX_BUFFER_USAGE_INDEX: return GL_ELEMENT_ARRAY_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

static inline GLenum mgfx_gl_get_data_usage(mgfx_access access)
{
    if (access == MGFX_ACCESS_GPU)
        return GL_STATIC_DRAW;
    return GL_DYNAMIC_DRAW;
}

static void mgfx_gl_bind_vertex_attributes(void)
{
    const mgfx_gl_pipeline *pipeline = ctx.gl.current_pipeline;
    GLenum type;
    GLint size;
    GLboolean norm;
    GLboolean is_integer;
    for (uint32_t i = 0; i < pipeline->vertex_layout.attribute_count; i++)
    {
        const mgfx_gl_vertex_attribute *attr = &pipeline->vertex_layout.attributes[i];
        switch (attr->format)
        {
            case MGFX_VERTEX_FORMAT_UINT:    type = GL_UNSIGNED_INT;  size = 1; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_INT:     type = GL_INT;           size = 1; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_FLOAT:   type = GL_FLOAT;         size = 1; norm = GL_FALSE; is_integer = GL_FALSE; break;
            case MGFX_VERTEX_FORMAT_UINT2:   type = GL_UNSIGNED_INT;  size = 2; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_INT2:    type = GL_INT;           size = 2; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_FLOAT2:  type = GL_FLOAT;         size = 2; norm = GL_FALSE; is_integer = GL_FALSE; break;
            case MGFX_VERTEX_FORMAT_UINT3:   type = GL_UNSIGNED_INT;  size = 3; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_INT3:    type = GL_INT;           size = 3; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_FLOAT3:  type = GL_FLOAT;         size = 3; norm = GL_FALSE; is_integer = GL_FALSE; break;
            case MGFX_VERTEX_FORMAT_UINT4:   type = GL_UNSIGNED_INT;  size = 4; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_INT4:    type = GL_INT;           size = 4; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_FLOAT4:  type = GL_FLOAT;         size = 4; norm = GL_FALSE; is_integer = GL_FALSE; break;
            case MGFX_VERTEX_FORMAT_UBYTE4:  type = GL_UNSIGNED_BYTE; size = 4; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_BYTE4:   type = GL_BYTE;          size = 4; norm = GL_FALSE; is_integer = GL_TRUE;  break;
            case MGFX_VERTEX_FORMAT_UBYTE4N: type = GL_UNSIGNED_BYTE; size = 4; norm = GL_TRUE;  is_integer = GL_FALSE; break;
            case MGFX_VERTEX_FORMAT_BYTE4N:  type = GL_BYTE;          size = 4; norm = GL_TRUE;  is_integer = GL_FALSE; break;
        }
        if (is_integer)
            glVertexAttribIPointer(attr->location, size, type, pipeline->vertex_layout.stride, (const void*)(uintptr_t)attr->offset);
        else glVertexAttribPointer(attr->location, size, type, norm, pipeline->vertex_layout.stride, (const void*)(uintptr_t)attr->offset);
        glEnableVertexAttribArray(attr->location);
    }
}

static mgfx_gl_buffer *mgfx_gl_create_buffer(const mgfx_buffer_create_info *create_info)
{
    mgfx_gl_buffer *buffer = (mgfx_gl_buffer*)calloc(1, sizeof(mgfx_gl_buffer));
    buffer->target = mgfx_gl_get_buffer_target(create_info->usage);
    buffer->usage = mgfx_gl_get_data_usage(create_info->access);
    glGenBuffers(1, &buffer->id);
    glBindBuffer(buffer->target, buffer->id);
    glBufferData(buffer->target, create_info->size, create_info->data, buffer->usage);
    return buffer;
}

static void mgfx_gl_update_buffer(mgfx_gl_buffer *buffer, size_t offset, size_t size, void *data)
{
    glBindBuffer(buffer->target, buffer->id);
    glBufferSubData(buffer->target, offset, size, data);
}

static void mgfx_gl_bind_vertex_buffer(mgfx_gl_buffer *buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
    mgfx_gl_bind_vertex_attributes();
}

static void mgfx_gl_bind_index_buffer(mgfx_gl_buffer *buffer, mgfx_index_type index_type)
{
    ctx.gl.index_type = mgfx_gl_get_index_type(index_type);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->id);
}

static void mgfx_gl_destroy_buffer(mgfx_gl_buffer *buffer)
{
    glDeleteBuffers(1, &buffer->id);
    free(buffer);
}

static inline GLenum mgfx_gl_get_internal_format(mgfx_format format)
{
    switch (format)
    {
        case MGFX_FORMAT_R8_UNORM:          return GL_R8;
        case MGFX_FORMAT_RG8_UNORM:         return GL_RG8;
        case MGFX_FORMAT_RGBA8_UNORM:       return GL_RGBA8;
        case MGFX_FORMAT_RGBA8_SRGB:        return GL_SRGB8_ALPHA8;
        case MGFX_FORMAT_BGRA8_UNORM:       return GL_RGBA8;
        case MGFX_FORMAT_BGRA8_SRGB:        return GL_SRGB8_ALPHA8;
        
        case MGFX_FORMAT_R16_SFLOAT:        return GL_R16F;
        case MGFX_FORMAT_RG16_SFLOAT:       return GL_RG16F;
        case MGFX_FORMAT_RGBA16_SFLOAT:     return GL_RGBA16F;
        
        case MGFX_FORMAT_R32_SFLOAT:        return GL_R32F;
        case MGFX_FORMAT_RG32_SFLOAT:       return GL_RG32F;
        case MGFX_FORMAT_RGBA32_SFLOAT:     return GL_RGBA32F;
        
        case MGFX_FORMAT_R32_UINT:          return GL_R32UI;
        case MGFX_FORMAT_R32_SINT:          return GL_R32I;
        
        case MGFX_FORMAT_D16_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8;
        case MGFX_FORMAT_D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8;
        case MGFX_FORMAT_D32_SFLOAT_S8_UINT:return GL_DEPTH32F_STENCIL8;
    }
    return GL_RGBA8;
}

static inline GLenum mgfx_gl_get_format(mgfx_format format)
{
    switch (format)
    {
        case MGFX_FORMAT_R8_UNORM:          return GL_RED;
        case MGFX_FORMAT_RG8_UNORM:         return GL_RG;
        case MGFX_FORMAT_RGBA8_UNORM:       return GL_RGBA;
        case MGFX_FORMAT_RGBA8_SRGB:        return GL_RGBA;
        case MGFX_FORMAT_BGRA8_UNORM:       return GL_BGRA;
        case MGFX_FORMAT_BGRA8_SRGB:        return GL_BGRA;
        
        case MGFX_FORMAT_R16_SFLOAT:        return GL_RED;
        case MGFX_FORMAT_RG16_SFLOAT:       return GL_RG;
        case MGFX_FORMAT_RGBA16_SFLOAT:     return GL_RGBA;
        
        case MGFX_FORMAT_R32_SFLOAT:        return GL_RED;
        case MGFX_FORMAT_RG32_SFLOAT:       return GL_RG;
        case MGFX_FORMAT_RGBA32_SFLOAT:     return GL_RGBA;
        
        case MGFX_FORMAT_R32_UINT:          return GL_RED_INTEGER;
        case MGFX_FORMAT_R32_SINT:          return GL_RED_INTEGER;
        
        case MGFX_FORMAT_D16_UNORM_S8_UINT:
        case MGFX_FORMAT_D24_UNORM_S8_UINT:
        case MGFX_FORMAT_D32_SFLOAT_S8_UINT: return GL_DEPTH_STENCIL;
    }
    return GL_RGBA;
}

static inline GLint mgfx_gl_get_filter(mgfx_sampler_filter filter)
{
    switch (filter)
    {
        case MGFX_SAMPLER_FILTER_NEAREST: return GL_NEAREST;
        case MGFX_SAMPLER_FILTER_LINEAR: return GL_LINEAR;
    }
    return GL_NEAREST;
}

static inline GLint mgfx_gl_get_address_mode(mgfx_sampler_address_mode address_mode)
{
    switch (address_mode)
    {
        case MGFX_SAMPLER_ADDRESS_MODE_REPEAT:
        return GL_REPEAT;
        case MGFX_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;
#if !defined(__EMSCRIPTEN__)
        case MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
        case MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
        return GL_CLAMP_TO_BORDER;
        default:
        return GL_REPEAT;
#else
        default:
        return GL_CLAMP_TO_EDGE;
#endif
    }
}

static mgfx_gl_image *mgfx_gl_create_image(const mgfx_image_create_info *create_info)
{
    mgfx_gl_image *image = (mgfx_gl_image*)calloc(1, sizeof(mgfx_gl_image));
    
    glGenTextures(1, &image->texture_id);
    
    switch (create_info->type)
    {
        case MGFX_IMAGE_TYPE_2D: image->texture_target = GL_TEXTURE_2D; break;
        case MGFX_IMAGE_TYPE_3D: image->texture_target = GL_TEXTURE_3D; break;
        case MGFX_IMAGE_TYPE_CUBE: image->texture_target = GL_TEXTURE_CUBE_MAP; break;
    }
    
    glBindTexture(image->texture_target, image->texture_id);
    
    GLenum internal_format = mgfx_gl_get_internal_format(create_info->format);
    image->format = mgfx_gl_get_format(create_info->format);
    
    const GLuint usage = create_info->usage != MGFX_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_24_8;

    if (image->texture_target == GL_TEXTURE_2D)
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
                        create_info->width, create_info->height, 0, image->format, usage, create_info->data);
    else if (image->texture_target == GL_TEXTURE_3D)
        glTexImage3D(GL_TEXTURE_3D, 0, internal_format,
                        create_info->width, create_info->height, create_info->depth, 0, image->format, GL_UNSIGNED_BYTE, create_info->data);
    
    image->width = create_info->width;
    image->height = create_info->height;
    image->depth = create_info->depth;
    
    return image;
}

static void mgfx_gl_destroy_image(mgfx_gl_image *image)
{
    glDeleteTextures(1, &image->texture_id);
    free(image);
}

static void mgfx_gl_update_image(mgfx_gl_image *image, size_t size, void *data)
{
    glBindTexture(image->texture_target, image->texture_id);
    if (image->texture_target == GL_TEXTURE_2D)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        image->width, image->height, image->format, GL_UNSIGNED_BYTE, data);
    else if (image->texture_target == GL_TEXTURE_3D)
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
                        image->width, image->height, image->depth, image->format, GL_UNSIGNED_BYTE, data);
}

static void mgfx_gl_bind_image(mgfx_gl_image *image, mgfx_gl_sampler *sampler, uint32_t binding)
{
    glActiveTexture(GL_TEXTURE0 + binding);
    glBindTexture(image->texture_target, image->texture_id);
    glTexParameteri(image->texture_target, GL_TEXTURE_MIN_FILTER, sampler->min_filter);
    glTexParameteri(image->texture_target, GL_TEXTURE_MAG_FILTER, sampler->mag_filter);
    glTexParameteri(image->texture_target, GL_TEXTURE_WRAP_S, sampler->address_mode_u);
    glTexParameteri(image->texture_target, GL_TEXTURE_WRAP_T, sampler->address_mode_v);
    glTexParameteri(image->texture_target, GL_TEXTURE_WRAP_R, sampler->address_mode_w);
}

static mgfx_gl_sampler *mgfx_gl_create_sampler(const mgfx_sampler_create_info *create_info)
{
    mgfx_gl_sampler *sampler = (mgfx_gl_sampler*)calloc(1, sizeof(mgfx_gl_sampler));
    
    sampler->min_filter = mgfx_gl_get_filter(create_info->min_filter);
    sampler->mag_filter = mgfx_gl_get_filter(create_info->mag_filter);
    
    sampler->address_mode_u = mgfx_gl_get_address_mode(create_info->address_mode_u);
    sampler->address_mode_v = mgfx_gl_get_address_mode(create_info->address_mode_v);
    sampler->address_mode_w = mgfx_gl_get_address_mode(create_info->address_mode_w);
    
    return sampler;
}

static void mgfx_gl_destroy_sampler(mgfx_gl_sampler *sampler)
{
    free(sampler);
}

static inline GLenum mgfx_gl_get_blend_factor(mgfx_blend_factor factor)
{
    switch (factor)
    {
        case MGFX_BLEND_FACTOR_ZERO: return GL_ZERO;
        case MGFX_BLEND_FACTOR_ONE: return GL_ONE;
        case MGFX_BLEND_FACTOR_SRC_COLOR: return GL_SRC_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
        case MGFX_BLEND_FACTOR_DST_COLOR: return GL_DST_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
        case MGFX_BLEND_FACTOR_SRC_ALPHA: return GL_SRC_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case MGFX_BLEND_FACTOR_DST_ALPHA: return GL_DST_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case MGFX_BLEND_FACTOR_CONSTANT_COLOR: return GL_CONSTANT_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
        case MGFX_BLEND_FACTOR_CONSTANT_ALPHA: return GL_CONSTANT_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
        case MGFX_BLEND_FACTOR_SRC_ALPHA_SATURATE: return GL_SRC_ALPHA_SATURATE;
#if !defined(__EMSCRIPTEN__)
        case MGFX_BLEND_FACTOR_SRC1_COLOR: return GL_SRC1_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: return GL_ONE_MINUS_SRC1_COLOR;
        case MGFX_BLEND_FACTOR_SRC1_ALPHA: return GL_SRC1_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: return GL_ONE_MINUS_SRC1_ALPHA;
#endif
    }
    
    return GL_ZERO;
}

static inline GLenum mgfx_gl_get_blend_op(mgfx_blend_op blend_op)
{
    switch (blend_op)
    {
        case MGFX_BLEND_OP_ADD: return GL_FUNC_ADD;
        case MGFX_BLEND_OP_SUBTRACT: return GL_FUNC_SUBTRACT;
        case MGFX_BLEND_OP_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        case MGFX_BLEND_OP_MIN: return GL_MIN;
        case MGFX_BLEND_OP_MAX: return GL_MAX;
        case MGFX_BLEND_OP_ZERO: return GL_ZERO;
        case MGFX_BLEND_OP_SRC: return GL_SRC_COLOR;
        case MGFX_BLEND_OP_DST: return GL_DST_COLOR;
        case MGFX_BLEND_OP_INVERT: return GL_INVERT;
        case MGFX_BLEND_OP_RED: return GL_RED;
        default: return GL_FUNC_ADD;
    }
}

static inline GLenum mgfx_gl_get_primitive_topology(mgfx_primitive_topology topology)
{
    switch (topology)
    {
        case MGFX_PRIMITIVE_TOPOLOGY_POINT_LIST: return GL_POINTS;
        case MGFX_PRIMITIVE_TOPOLOGY_LINE_LIST: return GL_LINES;
        case MGFX_PRIMITIVE_TOPOLOGY_LINE_STRIP: return GL_LINE_STRIP;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return GL_TRIANGLES;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return GL_TRIANGLE_FAN;
    }
    return GL_POINTS;
}

static inline GLenum mgfx_gl_get_cull_mode(mgfx_cull_mode mode)
{
    switch (mode)
    {
        case MGFX_CULL_MODE_NONE: return GL_NONE;
        case MGFX_CULL_MODE_FRONT: return GL_FRONT;
        case MGFX_CULL_MODE_BACK: return GL_BACK;
    }
    return GL_NONE;
}

static inline GLenum mgfx_gl_get_compare_op(mgfx_compare_op compare_op)
{
    switch (compare_op)
    {
        case MGFX_COMPARE_OP_LESS: return GL_LESS;
        case MGFX_COMPARE_OP_EQUAL: return GL_EQUAL;
        case MGFX_COMPARE_OP_LESS_OR_EQUAL: return GL_LEQUAL;
        case MGFX_COMPARE_OP_GREATER: return GL_GREATER;
        case MGFX_COMPARE_OP_NOT_EQUAL: return GL_NOTEQUAL;
        case MGFX_COMPARE_OP_GREATER_OR_EQUAL: return GL_GEQUAL;
        case MGFX_COMPARE_OP_ALWAYS: return GL_ALWAYS;
    }
    return GL_NEVER;
}

static GLenum mgfx_gl_get_front_face(mgfx_front_face front_face)
{
    switch (front_face)
    {
        case MGFX_FRONT_FACE_CCW: return GL_CCW;
        case MGFX_FRONT_FACE_CW: return GL_CW;
    }
    return GL_CW;
}

static void mgfx_gl_fill_graphics_pipeline(mgfx_gl_pipeline *pipeline, const mgfx_pipeline_create_info *create_info)
{
    uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const GLchar *const *)&create_info->shader.vertex.code, (const GLint *)&create_info->shader.vertex.size);
    glCompileShader(vertex_shader);
    
    uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const GLchar *const *)&create_info->shader.fragment.code, (const GLint *)&create_info->shader.fragment.size);
    glCompileShader(fragment_shader);
    
    pipeline->program_id = glCreateProgram();
    glAttachShader(pipeline->program_id, vertex_shader);
    glAttachShader(pipeline->program_id, fragment_shader);
    glLinkProgram(pipeline->program_id);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    glUseProgram(pipeline->program_id);
    
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_UNIFORMS && create_info->shader.uniform_blocks[i].name; i++)
    {
        const uint32_t index = glGetUniformBlockIndex(pipeline->program_id, create_info->shader.uniform_blocks[i].name);   
        glUniformBlockBinding(pipeline->program_id, index, create_info->shader.uniform_blocks[i].binding);
    }
    
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_IMAGES && create_info->shader.sampled_images[i].name; i++)
    {
        const GLint index = glGetUniformLocation(pipeline->program_id, create_info->shader.sampled_images[i].name);
        glUniform1iv(index, 1, &create_info->shader.sampled_images[i].binding);
    }
    
    uint32_t attribute_count = 0;
    uint32_t stride = 0;
    for (; attribute_count < MGFX_MAX_VERTEX_ATTRIBUTES && create_info->vertex_attributes[attribute_count]; attribute_count++)
    {
        mgfx_gl_vertex_attribute *attrb = &pipeline->vertex_layout.attributes[attribute_count];
        attrb->format = create_info->vertex_attributes[attribute_count];
        attrb->location = attribute_count;
        attrb->offset = stride;
        stride += mgfx_vertex_format_size(create_info->vertex_attributes[attribute_count]);
    }
    
    pipeline->vertex_layout.stride = stride;
    pipeline->vertex_layout.attribute_count = attribute_count;
    
    pipeline->color_blend = create_info->color_blend;
    pipeline->primitive_topology = mgfx_gl_get_primitive_topology(create_info->primitive_topology);
    pipeline->cull_mode = mgfx_gl_get_cull_mode(create_info->cull_mode);
    pipeline->front_face = mgfx_gl_get_front_face(create_info->front_face);
    
    pipeline->depth_stencil.stencil_test_enabled = create_info->depth_stencil.stencil_test_enabled;
    pipeline->depth_stencil.depth_write_enabled = create_info->depth_stencil.depth_write_enabled;
    if (create_info->depth_stencil.depth_compare_op)
        pipeline->depth_stencil.depth_compare_op = mgfx_gl_get_compare_op(create_info->depth_stencil.depth_compare_op);
    else pipeline->depth_stencil.depth_compare_op = 0;
    
    pipeline->type = MGFX_OPENGL_PIPELINE_TYPE_GRAPHICS;
}

static void mgfx_gl_fill_compute_pipeline(mgfx_gl_pipeline *pipeline, const mgfx_pipeline_create_info *create_info)
{
    uint32_t compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_shader, 1, (const GLchar *const *)&create_info->shader.compute.code, (const GLint *)&create_info->shader.compute.size);
    glCompileShader(compute_shader);
    
    pipeline->program_id = glCreateProgram();
    glAttachShader(pipeline->program_id, compute_shader);
    glLinkProgram(pipeline->program_id);
    
    glDeleteShader(compute_shader);
    
    pipeline->type = MGFX_OPENGL_PIPELINE_TYPE_COMPUTE;
}

static mgfx_gl_pipeline *mgfx_gl_create_pipeline(const mgfx_pipeline_create_info *create_info)
{
    mgfx_gl_pipeline *pipeline = (mgfx_gl_pipeline*)calloc(1, sizeof(mgfx_gl_pipeline));
    if (create_info->shader.compute.size)
        mgfx_gl_fill_compute_pipeline(pipeline, create_info);
    else mgfx_gl_fill_graphics_pipeline(pipeline, create_info);
    return pipeline;
}

static void mgfx_gl_destroy_pipeline(mgfx_gl_pipeline *pipeline)
{
    glDeleteProgram(pipeline->program_id);
    free(pipeline);
}

static void mgfx_gl_bind_pipeline(mgfx_gl_pipeline *pipeline)
{
    glUseProgram(pipeline->program_id);
    
    if (pipeline->type == MGFX_OPENGL_PIPELINE_TYPE_COMPUTE)
        return;
    
    ctx.gl.primitive_topology = pipeline->primitive_topology;
    ctx.gl.current_pipeline = pipeline;
    
    if (pipeline->color_blend.blend_enabled)
    {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(mgfx_gl_get_blend_factor(pipeline->color_blend.src_color_blend_factor),
                            mgfx_gl_get_blend_factor(pipeline->color_blend.dst_color_blend_factor),
                            mgfx_gl_get_blend_factor(pipeline->color_blend.src_alpha_blend_factor),
                            mgfx_gl_get_blend_factor(pipeline->color_blend.dst_alpha_blend_factor));
        glBlendEquationSeparate(mgfx_gl_get_blend_op(pipeline->color_blend.color_blend_op),
                                mgfx_gl_get_blend_op(pipeline->color_blend.alpha_blend_op));
    }
    else
        glDisable(GL_BLEND);
    
    if (pipeline->cull_mode == GL_NONE)
        glDisable(GL_CULL_FACE);
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(pipeline->cull_mode);
    }
    
    glFrontFace(pipeline->front_face);
    
    if (pipeline->depth_stencil.depth_compare_op)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(pipeline->depth_stencil.depth_compare_op);
        glDepthMask(pipeline->depth_stencil.depth_write_enabled);
    }
    else
        glDisable(GL_DEPTH_TEST);
    pipeline->depth_stencil.stencil_test_enabled ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
}

static void mgfx_gl_bind_pass(const mgfx_pass_info *pass)
{
    if (!mgfx_valid_pass(pass))
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.gl.swapchain.framebuffer);
        glViewport(0, 0, ctx.gl.width, ctx.gl.height);
        glScissor(0, 0, ctx.gl.width, ctx.gl.height);
        glClearColor(pass->clear.r, pass->clear.g, pass->clear.b, pass->clear.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ctx.gl.scratch_framebuffer);
    GLenum draw_buffers[MGFX_MAX_COLOR_ATTACHMENTS];
    uint32_t width = 0, height = 0;
    uint32_t color_count = 0;
    for (; color_count < MGFX_MAX_COLOR_ATTACHMENTS && pass->color_images[color_count]; color_count++)
    {
        mgfx_gl_image *img = (mgfx_gl_image*)pass->color_images[color_count];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_count, img->texture_target, img->texture_id, 0);
        draw_buffers[color_count] = GL_COLOR_ATTACHMENT0 + color_count;

        if (color_count == 0)
        {
            width = img->width;
            height = img->height;
        }
        else
        {
            MGFX_ASSERT(img->width == width && img->height == height, "Mismatched color attachment sizes");
        }
    }
    for (uint32_t i = color_count; i < MGFX_MAX_COLOR_ATTACHMENTS; i++)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
    if (color_count > 0)
        glDrawBuffers(color_count, draw_buffers);
    else
        glDrawBuffers(0, NULL);

    mgfx_gl_image *depth = (mgfx_gl_image*)pass->depth_stencil_image;
    if (depth)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depth->texture_target, depth->texture_id, 0);
        if (color_count == 0)
        {
            width = depth->width;
            height = depth->height;
        }
        else
        {
            MGFX_ASSERT(depth->width == width && depth->height == height, "Depth attachment size mismatch");
        }
    }
    else
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
    glClearColor(pass->clear.r, pass->clear.g, pass->clear.b, pass->clear.a);
    GLbitfield clear_mask = color_count > 0 ? GL_COLOR_BUFFER_BIT : 0;
    if (depth) clear_mask |= GL_DEPTH_BUFFER_BIT;
    glClear(clear_mask);
}

static void mgfx_gl_resize(uint32_t width, uint32_t height)
{
    glBindTexture(GL_TEXTURE_2D, ctx.gl.swapchain.color_attachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    
    ctx.gl.width = width;
    ctx.gl.height = height;
}

static void mgfx_gl_vsync(bool vsync)
{
    _mgfx_gl_set_swap_interval(vsync);
    ctx.gl.vsync = vsync;
}

static void mgfx_gl_init_backbuffer(void)
{
    const char *vert_code =
#if defined(__EMSCRIPTEN__)
    "#version 300 es\n"
        "precision mediump float;"
#else
    "#version 450 core\n"
#endif
    "const vec2 positions[4] = vec2[]("
        "vec2(-1.0, -1.0),"
        "vec2(1.0, -1.0),"
        "vec2(-1.0, 1.0),"
        "vec2(1.0, 1.0)"
        ");"
        "const vec2 tex_coords[4] = vec2[]("
        "vec2(0.0, 0.0),"
        "vec2(1.0, 0.0),"
        "vec2(0.0, 1.0),"
        "vec2(1.0, 1.0)"
        ");"
        "out vec2 tex_coord;"
        "void main()"
        "{"
        "gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);"
        "gl_Position.y = -gl_Position.y;"
        "tex_coord = tex_coords[gl_VertexID];"
        "}\0";
    
    const char *frag_code =
#if defined(__EMSCRIPTEN__)
    "#version 300 es\n"
        "precision mediump float;"
#else
    "#version 450 core\n"
#endif
    "out vec4 out_color;"
        "in vec2 tex_coord;"
        "uniform sampler2D u_texture;"
        "void main()"
        "{"
        "out_color = texture(u_texture, tex_coord);"
        "}\0";
    
    uint32_t vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vert_code, NULL);
    glCompileShader(vertex);
    
    uint32_t fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &frag_code, NULL);
    glCompileShader(fragment);
    
    ctx.gl.swapchain.program = glCreateProgram();
    glAttachShader(ctx.gl.swapchain.program, vertex);
    glAttachShader(ctx.gl.swapchain.program, fragment);
    glLinkProgram(ctx.gl.swapchain.program);
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    glGenFramebuffers(1, &ctx.gl.swapchain.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.gl.swapchain.framebuffer);
    
    glGenTextures(1, &ctx.gl.swapchain.color_attachment);
    glBindTexture(GL_TEXTURE_2D, ctx.gl.swapchain.color_attachment);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx.gl.swapchain.color_attachment, 0);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void mgfx_gl_destroy_backbuffer(void)
{
    glDeleteFramebuffers(1, &ctx.gl.swapchain.framebuffer);
    glDeleteTextures(1, &ctx.gl.swapchain.color_attachment);
    glDeleteProgram(ctx.gl.swapchain.program);
}

static void mgfx_gl_init(const mgfx_init_info *init_info)
{
    ctx.gl = (mgfx_gl_context){ 0 };
    
    _mgfx_gl_load_platform((mgfx_platform_handle*)init_info->handle);
#if !defined(__EMSCRIPTEN__)
    _mgfx_gl_load_opengl();
#endif
    mgfx_gl_init_backbuffer();
    
    mgfx_gl_resize(init_info->width, init_info->height);
    _mgfx_gl_set_swap_interval(init_info->vsync);
    ctx.gl.vsync = init_info->vsync;
    
    glGenVertexArrays(1, &ctx.gl.vao);
    glGenFramebuffers(1, &ctx.gl.scratch_framebuffer);
    glGenBuffers(MGFX_MAX_BINDABLE_UNIFORMS, ctx.gl.uniform_buffers);
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_UNIFORMS; i++)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, ctx.gl.uniform_buffers[i]);
        glBufferData(GL_UNIFORM_BUFFER, MGFX_MAX_UNIFORM_UPDATE_SIZE, NULL, GL_DYNAMIC_DRAW);
    }
}

static void mgfx_gl_shutdown(void)
{
    glDeleteBuffers(MGFX_MAX_BINDABLE_UNIFORMS, ctx.gl.uniform_buffers);
    glDeleteFramebuffers(1, &ctx.gl.scratch_framebuffer);
    glDeleteVertexArrays(1, &ctx.gl.vao);
    mgfx_gl_destroy_backbuffer();
#if !defined(__EMSCRIPTEN__)
    _mgfx_gl_unload_opengl();
#endif
    _mgfx_gl_unload_platform();
}

static mgfx_result mgfx_gl_begin(void)
{
    glEnable(GL_SCISSOR_TEST);
    glBindVertexArray(ctx.gl.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.gl.swapchain.framebuffer);
    return MGFX_RESULT_SUCCESS;
}

static void mgfx_gl_end(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(ctx.gl.swapchain.program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.gl.swapchain.color_attachment);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    _mgfx_gl_swapbuffers();
}

static void mgfx_gl_viewport(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    glViewport(x, y, width, height);
}

static void mgfx_gl_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    glScissor(x, y, width, height);
}

static void mgfx_gl_draw(uint32_t vertex_count, uint32_t first_vertex)
{
    glDrawArrays(ctx.gl.primitive_topology, first_vertex, vertex_count);
}

static void mgfx_gl_draw_indexed(uint32_t index_count, uint32_t first_index, int32_t first_vertex)
{
    const int index_size = (ctx.gl.index_type == GL_UNSIGNED_SHORT) ? 2 : 4;
#if !defined(__EMSCRIPTEN__)
    glDrawElementsBaseVertex(ctx.gl.primitive_topology, index_count, ctx.gl.index_type, (void*)(uintptr_t)(first_index * index_size), first_vertex);
#else
    glDrawElements(ctx.gl.primitive_topology, index_count, ctx.gl.index_type, (void*)(uintptr_t)(first_index * index_size));
#endif
}

static void mgfx_gl_draw_instanced(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    glDrawArraysInstanced(ctx.gl.primitive_topology, first_vertex, vertex_count, instance_count);
}

static void mgfx_gl_draw_indexed_instanced(uint32_t index_count, uint32_t first_index, int32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    const int index_size = (ctx.gl.index_type == GL_UNSIGNED_SHORT) ? 2 : 4;
#if !defined(__EMSCRIPTEN__)
    glDrawElementsInstancedBaseVertex(ctx.gl.primitive_topology, index_count, ctx.gl.index_type, (void*)(uintptr_t)(first_index * index_size), instance_count, first_vertex);
#else
    glDrawElementsInstanced(ctx.gl.primitive_topology, index_count, ctx.gl.index_type, (void*)(uintptr_t)(first_index * index_size), instance_count);
#endif
}

static void mgfx_gl_dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    glDispatchCompute(group_count_x, group_count_y, group_count_z);
}

static void mgfx_gl_bind_uniforms(uint32_t binding, size_t size, void *data)
{
    glBindBuffer(GL_UNIFORM_BUFFER, ctx.gl.uniform_buffers[binding]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ctx.gl.uniform_buffers[binding]);
}

#endif // MGFX_OPENGL

#if defined(MGFX_D3D11)

static DXGI_FORMAT mgfx_d3d11_get_format(mgfx_format format)
{
    switch (format)
    {
        case MGFX_FORMAT_R8_UNORM:          return DXGI_FORMAT_R8_UNORM;
        case MGFX_FORMAT_RG8_UNORM:         return DXGI_FORMAT_R8G8_UNORM;
        case MGFX_FORMAT_RGBA8_UNORM:       return DXGI_FORMAT_R8G8B8A8_UNORM;
        case MGFX_FORMAT_RGBA8_SRGB:        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case MGFX_FORMAT_BGRA8_UNORM:       return DXGI_FORMAT_B8G8R8A8_UNORM;
        case MGFX_FORMAT_BGRA8_SRGB:        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        
        case MGFX_FORMAT_R16_SFLOAT:        return DXGI_FORMAT_R16_FLOAT;
        case MGFX_FORMAT_RG16_SFLOAT:       return DXGI_FORMAT_R16G16_FLOAT;
        case MGFX_FORMAT_RGBA16_SFLOAT:     return DXGI_FORMAT_R16G16B16A16_FLOAT;
        
        case MGFX_FORMAT_R32_SFLOAT:        return DXGI_FORMAT_R32_FLOAT;
        case MGFX_FORMAT_RG32_SFLOAT:       return DXGI_FORMAT_R32G32_FLOAT;
        case MGFX_FORMAT_RGBA32_SFLOAT:     return DXGI_FORMAT_R32G32B32A32_FLOAT;
        
        case MGFX_FORMAT_R32_UINT:          return DXGI_FORMAT_R32_UINT;
        case MGFX_FORMAT_R32_SINT:          return DXGI_FORMAT_R32_SINT;
        
        case MGFX_FORMAT_RGB8_UNORM:
        case MGFX_FORMAT_RGB8_SRGB:
        case MGFX_FORMAT_RGB16_SFLOAT:
        case MGFX_FORMAT_RGB32_SFLOAT:     return DXGI_FORMAT_UNKNOWN;
        
        case MGFX_FORMAT_D16_UNORM_S8_UINT: return DXGI_FORMAT_R16_TYPELESS;
        case MGFX_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24G8_TYPELESS;
        case MGFX_FORMAT_D32_SFLOAT_S8_UINT:return DXGI_FORMAT_R32G8X24_TYPELESS;
    }
    return DXGI_FORMAT_UNKNOWN;
}

static inline DXGI_FORMAT mgfx_d3d11_get_vertex_format(mgfx_vertex_format format)
{
    switch (format)
    {
        case MGFX_VERTEX_FORMAT_FLOAT: return DXGI_FORMAT_R32_FLOAT; break;
        case MGFX_VERTEX_FORMAT_FLOAT2: return DXGI_FORMAT_R32G32_FLOAT; break;
        case MGFX_VERTEX_FORMAT_FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT; break;
        case MGFX_VERTEX_FORMAT_FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT; break;
        
        case MGFX_VERTEX_FORMAT_UINT: return DXGI_FORMAT_R32_UINT; break;
        case MGFX_VERTEX_FORMAT_UINT2: return DXGI_FORMAT_R32G32_UINT; break;
        case MGFX_VERTEX_FORMAT_UINT3: return DXGI_FORMAT_R32G32B32_UINT; break;
        case MGFX_VERTEX_FORMAT_UINT4: return DXGI_FORMAT_R32G32B32A32_UINT; break;
        
        case MGFX_VERTEX_FORMAT_INT: return DXGI_FORMAT_R32_SINT; break;
        case MGFX_VERTEX_FORMAT_INT2: return DXGI_FORMAT_R32G32_SINT; break;
        case MGFX_VERTEX_FORMAT_INT3: return DXGI_FORMAT_R32G32B32_SINT; break;
        case MGFX_VERTEX_FORMAT_INT4: return DXGI_FORMAT_R32G32B32A32_SINT; break;
        
        case MGFX_VERTEX_FORMAT_UBYTE4: return DXGI_FORMAT_R8G8B8A8_UINT; break;
        case MGFX_VERTEX_FORMAT_BYTE4: return DXGI_FORMAT_R8G8B8A8_SINT; break;
        
        case MGFX_VERTEX_FORMAT_UBYTE4N: return DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case MGFX_VERTEX_FORMAT_BYTE4N: return DXGI_FORMAT_R8G8B8A8_SNORM; break;
        
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

static inline DXGI_FORMAT mgfx_d3d11_get_index_type(mgfx_index_type index_type)
{
    if (index_type == MGFX_INDEX_TYPE_UINT16)
        return DXGI_FORMAT_R16_UINT;
    return DXGI_FORMAT_R32_UINT;
}

static inline D3D11_CULL_MODE mgfx_d3d11_get_cull_mode(mgfx_cull_mode mode)
{
    switch (mode)
    {
        case MGFX_CULL_MODE_NONE: return D3D11_CULL_NONE;
        case MGFX_CULL_MODE_FRONT: return D3D11_CULL_FRONT;
        case MGFX_CULL_MODE_BACK: return D3D11_CULL_BACK;
    }
    return D3D11_CULL_NONE;
}

static inline D3D11_COMPARISON_FUNC mgfx_d3d11_get_comparison_func(mgfx_compare_op func)
{
    switch (func)
    {
        case MGFX_COMPARE_OP_NEVER: return D3D11_COMPARISON_NEVER;
        case MGFX_COMPARE_OP_LESS: return D3D11_COMPARISON_LESS;
        case MGFX_COMPARE_OP_EQUAL: return D3D11_COMPARISON_EQUAL;
        case MGFX_COMPARE_OP_LESS_OR_EQUAL: return D3D11_COMPARISON_LESS_EQUAL;
        case MGFX_COMPARE_OP_GREATER: return D3D11_COMPARISON_GREATER;
        case MGFX_COMPARE_OP_NOT_EQUAL: return D3D11_COMPARISON_NOT_EQUAL;
        case MGFX_COMPARE_OP_GREATER_OR_EQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
        case MGFX_COMPARE_OP_ALWAYS: return D3D11_COMPARISON_ALWAYS;
    }
    return D3D11_COMPARISON_NEVER;
}

static DXGI_FORMAT mgfx_d3d11_get_dsv_format(mgfx_format format)
{
    switch (format)
    {
        case MGFX_FORMAT_D16_UNORM_S8_UINT: return DXGI_FORMAT_D16_UNORM;
        case MGFX_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case MGFX_FORMAT_D32_SFLOAT_S8_UINT:return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

static DXGI_FORMAT mgfx_d3d11_get_srv_format(mgfx_format format)
{
    switch (format)
    {
        case MGFX_FORMAT_D16_UNORM_S8_UINT: return DXGI_FORMAT_R16_UNORM;
        case MGFX_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case MGFX_FORMAT_D32_SFLOAT_S8_UINT:return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    }
    return mgfx_d3d11_get_format(format);
}

static inline D3D_SRV_DIMENSION mgfx_d3d11_get_srv_dimension(mgfx_image_type type)
{
    switch (type)
    {
        case MGFX_IMAGE_TYPE_2D: return D3D11_SRV_DIMENSION_TEXTURE2D;
        case MGFX_IMAGE_TYPE_3D: return D3D11_SRV_DIMENSION_TEXTURE3D;
        case MGFX_IMAGE_TYPE_CUBE: return D3D11_SRV_DIMENSION_TEXTURECUBE;
    }
    return D3D_SRV_DIMENSION_TEXTURE2D;
}

static inline D3D11_FILTER mgfx_d3d11_get_filter(mgfx_sampler_filter filter)
{
    switch (filter)
    {
        case MGFX_SAMPLER_FILTER_NEAREST: return D3D11_FILTER_MIN_MAG_MIP_POINT;
        case MGFX_SAMPLER_FILTER_LINEAR: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }
    return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

static inline D3D11_TEXTURE_ADDRESS_MODE mgfx_d3d11_get_texture_address_mode(mgfx_sampler_address_mode mode)
{
    switch (mode)
    {
        case MGFX_SAMPLER_ADDRESS_MODE_REPEAT: return D3D11_TEXTURE_ADDRESS_WRAP;
        case MGFX_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: return D3D11_TEXTURE_ADDRESS_MIRROR;
        case MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: return D3D11_TEXTURE_ADDRESS_CLAMP;
        case MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return D3D11_TEXTURE_ADDRESS_BORDER;
    }
    return D3D11_TEXTURE_ADDRESS_WRAP;
}

static inline D3D11_BLEND mgfx_d3d11_get_blend_factor(mgfx_blend_factor factor)
{
    switch (factor)
    {
        case MGFX_BLEND_FACTOR_ZERO: return D3D11_BLEND_ZERO;
        case MGFX_BLEND_FACTOR_ONE: return D3D11_BLEND_ONE;
        case MGFX_BLEND_FACTOR_SRC_COLOR: return D3D11_BLEND_SRC_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
        case MGFX_BLEND_FACTOR_DST_COLOR: return D3D11_BLEND_DEST_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
        case MGFX_BLEND_FACTOR_SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
        case MGFX_BLEND_FACTOR_DST_ALPHA: return D3D11_BLEND_DEST_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
        /*case MGFX_BLEND_FACTOR_CONSTANT_COLOR: return D3D11_BLEND_CONSTANT;
        case MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return D3D11_BLEND_INV_CONST_COLOR;
        case MGFX_BLEND_FACTOR_CONSTANT_ALPHA: return D3D11_BLEND_CONSTANT_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return D3D11_BLEND_INV_CONST_ALPHA;*/
        case MGFX_BLEND_FACTOR_SRC_ALPHA_SATURATE: return D3D11_BLEND_SRC_ALPHA_SAT;
        case MGFX_BLEND_FACTOR_SRC1_COLOR: return D3D11_BLEND_SRC1_COLOR;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: return D3D11_BLEND_INV_SRC1_COLOR;
        case MGFX_BLEND_FACTOR_SRC1_ALPHA: return D3D11_BLEND_SRC1_ALPHA;
        case MGFX_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: return D3D11_BLEND_INV_SRC1_ALPHA;
    }
    return D3D11_BLEND_ZERO;
}

static inline D3D11_BLEND_OP mgfx_d3d11_get_blend_op(mgfx_blend_op op)
{
    switch (op)
    {
        case MGFX_BLEND_OP_ADD: return D3D11_BLEND_OP_ADD;
        case MGFX_BLEND_OP_SUBTRACT: return D3D11_BLEND_OP_SUBTRACT;
        case MGFX_BLEND_OP_REVERSE_SUBTRACT: return D3D11_BLEND_OP_REV_SUBTRACT;
        case MGFX_BLEND_OP_MIN: return D3D11_BLEND_OP_MIN;
        case MGFX_BLEND_OP_MAX: return D3D11_BLEND_OP_MAX;
    }
    return D3D11_BLEND_OP_ADD;
}

static inline D3D11_PRIMITIVE_TOPOLOGY mgfx_d3d11_get_primitive_topology(mgfx_primitive_topology topology)
{
    switch (topology)
    {
        case MGFX_PRIMITIVE_TOPOLOGY_POINT_LIST: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case MGFX_PRIMITIVE_TOPOLOGY_LINE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case MGFX_PRIMITIVE_TOPOLOGY_LINE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case MGFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    }
    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

static inline UINT mgfx_d3d11_get_bind_flags(mgfx_buffer_usage usage)
{
    switch (usage)
    {
        case MGFX_BUFFER_USAGE_VERTEX: return D3D11_BIND_VERTEX_BUFFER;
        case MGFX_BUFFER_USAGE_INDEX: return D3D11_BIND_INDEX_BUFFER;
    }
    return D3D11_BIND_VERTEX_BUFFER;
}

static void mgfx_d3d11_init(const mgfx_init_info *init_info)
{
    ctx.d3d11 = (mgfx_d3d11_context){ 0 };
    
    DXGI_SWAP_CHAIN_DESC sd = {
        .BufferCount = 1,
        .BufferDesc.Width = (UINT)init_info->width,
        .BufferDesc.Height = (UINT)init_info->height,
        .BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .SampleDesc.Count = 1,
        .SampleDesc.Quality = 0,
        .OutputWindow = (HWND)((mgfx_platform_handle*)init_info->handle)->win32.hwnd,
        .Windowed = TRUE
    };
    
    D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        NULL,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &ctx.d3d11.swapchain,
        &ctx.d3d11.device,
        NULL,
        &ctx.d3d11.immediate_context);
    
    ID3D11Texture2D *backbuffer;
    MGFX_D3D11_CALL(ctx.d3d11.swapchain, GetBuffer, 0, MGFX_IID(IID_ID3D11Texture2D), (void**)&backbuffer);
    MGFX_D3D11_CALL(ctx.d3d11.device, CreateRenderTargetView, (ID3D11Resource*)backbuffer, NULL, &ctx.d3d11.target_view);
    MGFX_D3D11_CALL0(backbuffer, Release);
    
    ctx.d3d11.width = init_info->width;
    ctx.d3d11.height = init_info->height;
    ctx.d3d11.vsync = init_info->vsync;
}

static void mgfx_d3d11_shutdown(void)
{
    MGFX_D3D11_CALL0(ctx.d3d11.immediate_context, ClearState);
    MGFX_D3D11_CALL0(ctx.d3d11.target_view, Release);
    MGFX_D3D11_CALL0(ctx.d3d11.swapchain, Release);
    MGFX_D3D11_CALL0(ctx.d3d11.immediate_context, Release);
    MGFX_D3D11_CALL0(ctx.d3d11.device, Release);
}

static void mgfx_d3d11_resize_backbuffer(void)
{
    MGFX_D3D11_CALL0(ctx.d3d11.immediate_context, ClearState);
    MGFX_D3D11_CALL0(ctx.d3d11.immediate_context, Flush);
    
    MGFX_D3D11_SAFE_RELEASE(ctx.d3d11.target_view);
    
    HRESULT hr = MGFX_D3D11_CALL(ctx.d3d11.swapchain, ResizeBuffers, 0, ctx.d3d11.width, ctx.d3d11.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    
    if (FAILED(hr))
        return;
    
    ID3D11Texture2D* backbuffer = NULL;
    
    hr = MGFX_D3D11_CALL(ctx.d3d11.swapchain, GetBuffer, 0, MGFX_IID(IID_ID3D11Texture2D), (void**)&backbuffer);
    
    if (FAILED(hr))
        return;
    
    hr = MGFX_D3D11_CALL(ctx.d3d11.device, CreateRenderTargetView, (ID3D11Resource*)backbuffer, NULL, &ctx.d3d11.target_view);
    
    MGFX_D3D11_CALL0(backbuffer, Release);
}

static mgfx_result mgfx_d3d11_begin(void)
{
    if (ctx.d3d11.pending_resize)
    {
        ctx.d3d11.pending_resize = false;
        mgfx_d3d11_resize_backbuffer();
    }
    return MGFX_RESULT_SUCCESS;
}

static void mgfx_d3d11_end(void)
{
    MGFX_D3D11_CALL(ctx.d3d11.swapchain, Present, ctx.d3d11.vsync, 0);
}

static void mgfx_d3d11_resize(uint32_t width, uint32_t height)
{
    ctx.d3d11.width = width;
    ctx.d3d11.height = height;
    ctx.d3d11.pending_resize = true;
}

static void mgfx_d3d11_vsync(bool vsync)
{
    ctx.d3d11.vsync = vsync;
}

static void mgfx_d3d11_viewport(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    D3D11_VIEWPORT vp = {
        .Width = (FLOAT)width,
        .Height = (FLOAT)height,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
        .TopLeftX = (FLOAT)x,
        .TopLeftY = (FLOAT)y
    };

    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, RSSetViewports, 1, &vp);
}

static void mgfx_d3d11_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    D3D11_RECT rect = {
        .left = x,
        .top = y,
        .right = (LONG)(x + width),
        .bottom = (LONG)(y + height)
    };

    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, RSSetScissorRects, 1, &rect);
}

static void mgfx_d3d11_draw(uint32_t vertex_count, uint32_t first_vertex)
{
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Draw, vertex_count, first_vertex);
}

static void mgfx_d3d11_draw_indexed(uint32_t index_count, uint32_t first_index, int32_t first_vertex)
{
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, DrawIndexed, index_count, first_index, first_vertex);
}

static void mgfx_d3d11_draw_instanced(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, DrawInstanced, vertex_count, instance_count, first_vertex, first_instance);
}

static void mgfx_d3d11_draw_indexed_instanced(uint32_t index_count, uint32_t first_index, int32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, DrawIndexedInstanced, index_count, instance_count, first_index, first_vertex, first_instance);
}

static void mgfx_d3d11_dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Dispatch, group_count_x, group_count_y, group_count_z);
}

static void mgfx_d3d11_bind_uniforms(uint32_t binding, size_t size, void *data)
{
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Map, (ID3D11Resource*)ctx.d3d11.current_pipeline->constant_buffers[binding], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    memcpy(mapped_resource.pData, data, size);
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Unmap, (ID3D11Resource*)ctx.d3d11.current_pipeline->constant_buffers[binding], 0);
}

static mgfx_d3d11_buffer *mgfx_d3d11_create_buffer(const mgfx_buffer_create_info *create_info)
{
    mgfx_d3d11_buffer *buffer = (mgfx_d3d11_buffer*)calloc(1, sizeof(mgfx_d3d11_buffer));
    
    D3D11_BUFFER_DESC desc = {
        .ByteWidth = (UINT)create_info->size,
        .BindFlags = mgfx_d3d11_get_bind_flags(create_info->usage)
    };

    if (create_info->access == MGFX_ACCESS_GPU)
    {
        desc.Usage = D3D11_USAGE_DEFAULT;
        buffer->is_cpu = false;
    }
    else
    {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer->is_cpu = true;
    }

    D3D11_SUBRESOURCE_DATA *init_data_ptr = NULL;
    D3D11_SUBRESOURCE_DATA init_data = { 0 };
    if (create_info->data)
    {
        init_data.pSysMem = create_info->data;
        init_data_ptr = &init_data;
    }
    
    MGFX_D3D11_CALL(ctx.d3d11.device, CreateBuffer, &desc, init_data_ptr, &buffer->buffer);
    
    return buffer;
}

static void mgfx_d3d11_update_buffer(mgfx_d3d11_buffer *buffer, size_t offset, size_t size, void *data)
{
    if (buffer->is_cpu)
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Map, (ID3D11Resource*)buffer->buffer, 0,
                        (offset == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped_resource); // TODO (box): add proper buffer tracking after implementing pool allocators
        memcpy((uint8_t*)mapped_resource.pData + offset, data, size);
        MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Unmap, (ID3D11Resource*)buffer->buffer, 0);
        return;
    }
    
    D3D11_BOX box = { (UINT)offset, 0, 0, (UINT)(offset + size), 1, 1 };
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, UpdateSubresource, (ID3D11Resource*)buffer->buffer, 0, &box, data, 0, 0);
}

static void mgfx_d3d11_bind_vertex_buffer(mgfx_d3d11_buffer *buffer)
{
    UINT offset = 0;
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, IASetVertexBuffers, 0, 1, &buffer->buffer, &ctx.d3d11.current_pipeline->layout_stride, &offset);
}

static void mgfx_d3d11_bind_index_buffer(mgfx_d3d11_buffer *buffer, mgfx_index_type index_type)
{
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, IASetIndexBuffer, buffer->buffer, mgfx_d3d11_get_index_type(index_type), 0);
}

static void mgfx_d3d11_destroy_buffer(mgfx_d3d11_buffer *buffer)
{
    MGFX_D3D11_SAFE_RELEASE(buffer->buffer);
    free(buffer);
}

static mgfx_d3d11_image *mgfx_d3d11_create_image(const mgfx_image_create_info *create_info)
{
    mgfx_d3d11_image *image = (mgfx_d3d11_image*)calloc(1, sizeof(mgfx_d3d11_image));
    
    const DXGI_FORMAT format = mgfx_d3d11_get_format(create_info->format);
    
    D3D11_SUBRESOURCE_DATA *init_data_ptr = NULL;
    D3D11_SUBRESOURCE_DATA init_data = { 0 };
    if (create_info->data)
    {
        init_data.pSysMem = create_info->data;
        init_data.SysMemPitch = create_info->width * mgfx_format_bpp(create_info->format);
        init_data.SysMemSlicePitch = init_data.SysMemPitch * create_info->height;
        init_data_ptr = &init_data;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {
        view_desc.Format = mgfx_d3d11_get_srv_format(create_info->format),
        view_desc.ViewDimension = mgfx_d3d11_get_srv_dimension(create_info->type)
    };

    if (create_info->type == MGFX_IMAGE_TYPE_2D)
    {
        D3D11_TEXTURE2D_DESC texture_desc = {
            .Width = create_info->width,
            .Height = create_info->height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = format,
            .SampleDesc.Count = 1,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE
        };
        
        if (create_info->access == MGFX_ACCESS_CPU)
        {
            texture_desc.Usage = D3D11_USAGE_DYNAMIC;
            texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else texture_desc.Usage = D3D11_USAGE_DEFAULT;
        
        if (create_info->usage == MGFX_IMAGE_USAGE_COLOR_ATTACHMENT)
            texture_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        else if (create_info->usage == MGFX_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
            texture_desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        
        MGFX_D3D11_CALL(ctx.d3d11.device, CreateTexture2D, &texture_desc, init_data_ptr, (ID3D11Texture2D**)&image->texture);
        view_desc.Texture2D.MipLevels = 1;
    }
    else if (create_info->type == MGFX_IMAGE_TYPE_3D)
    {
        D3D11_TEXTURE3D_DESC texture_desc = {
            .Width = create_info->width,
            .Height = create_info->height,
            .Depth = create_info->depth,
            .MipLevels = 1,
            .Format = format,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };
        
        if (create_info->access == MGFX_ACCESS_CPU)
        {
            texture_desc.Usage = D3D11_USAGE_DYNAMIC;
            texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else texture_desc.Usage = D3D11_USAGE_DEFAULT;
        
        MGFX_D3D11_CALL(ctx.d3d11.device, CreateTexture3D, &texture_desc, init_data_ptr, (ID3D11Texture3D**)&image->texture);
        view_desc.Texture3D.MipLevels = 1;
    }

    MGFX_D3D11_CALL(ctx.d3d11.device, CreateShaderResourceView, (ID3D11Resource*)image->texture, &view_desc, &image->view);
    
    if (create_info->usage == MGFX_IMAGE_USAGE_COLOR_ATTACHMENT)
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {
            .Format = format,
            .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D
        };
        MGFX_D3D11_CALL(ctx.d3d11.device, CreateRenderTargetView, image->texture, &rtv_desc, &image->rtv);
    }
    else if (create_info->usage == MGFX_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
            .Format = mgfx_d3d11_get_dsv_format(create_info->format),
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D
        };
        MGFX_D3D11_CALL(ctx.d3d11.device, CreateDepthStencilView, image->texture, &dsv_desc, &image->dsv);
    }
    
    image->width = create_info->width;
    image->height = create_info->height;
    image->is_cpu = create_info->access == MGFX_ACCESS_CPU;
    
    return image;
}

static void mgfx_d3d11_destroy_image(mgfx_d3d11_image *image)
{
    MGFX_D3D11_SAFE_RELEASE(image->rtv);
    MGFX_D3D11_SAFE_RELEASE(image->view);
    MGFX_D3D11_SAFE_RELEASE(image->texture);
    free(image);
}

static void mgfx_d3d11_update_image(mgfx_d3d11_image *image, size_t size, void *data)
{
    if (image->is_cpu)
    {
        D3D11_MAPPED_SUBRESOURCE mapped = { 0 };
        MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Map,
                        (ID3D11Resource*)image->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        const uint32_t row_pitch = (uint32_t)size / image->height;
        for (uint32_t y = 0; y < image->height; y++)
            memcpy((uint8_t*)mapped.pData + y * mapped.RowPitch,
                    (uint8_t*)data + y * row_pitch,
                    row_pitch);
        MGFX_D3D11_CALL(ctx.d3d11.immediate_context, Unmap, (ID3D11Resource*)image->texture, 0);
        return;
    }
    
    const uint32_t row_pitch = (uint32_t)size / image->height;
    const uint32_t depth_pitch = (uint32_t)size;
    
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, UpdateSubresource, (ID3D11Resource*)image->texture, 0, NULL, data, row_pitch, depth_pitch);
}

static void mgfx_d3d11_bind_image(mgfx_d3d11_image *image, ID3D11SamplerState *sampler, uint32_t binding)
{
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, PSSetShaderResources, binding, 1, &image->view);
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, PSSetSamplers, binding, 1, &sampler);
}

static ID3D11SamplerState *mgfx_d3d11_create_sampler(const mgfx_sampler_create_info *create_info)
{
    ID3D11SamplerState *sampler;
    D3D11_SAMPLER_DESC samp_desc = {
        .Filter = mgfx_d3d11_get_filter(create_info->min_filter),
        .AddressU = mgfx_d3d11_get_texture_address_mode(create_info->address_mode_u),
        .AddressV = mgfx_d3d11_get_texture_address_mode(create_info->address_mode_v),
        .AddressW = mgfx_d3d11_get_texture_address_mode(create_info->address_mode_w),
        .ComparisonFunc = D3D11_COMPARISON_NEVER,
        .MinLOD = 0,
        .MaxLOD = D3D11_FLOAT32_MAX
    };
    MGFX_D3D11_CALL(ctx.d3d11.device, CreateSamplerState, &samp_desc, &sampler);
    return sampler;
}

static void mgfx_d3d11_destroy_sampler(ID3D11SamplerState *sampler)
{
    MGFX_D3D11_SAFE_RELEASE(sampler);
}

static void mgfx_d3d11_begin_clear_render_target(ID3D11RenderTargetView *color_attachment, mgfx_color clear_value, uint32_t width, uint32_t height)
{
    mgfx_d3d11_viewport(0, 0, width, height);
    mgfx_d3d11_scissor(0, 0, width, height);
    
    const float clear_color[4] = {
        clear_value.r,
        clear_value.g,
        clear_value.b,
        clear_value.a
    };
    
    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, ClearRenderTargetView, color_attachment, clear_color);
}

static void mgfx_d3d11_bind_pass(const mgfx_pass_info *pass)
{
    if (!mgfx_valid_pass(pass))
    {
        MGFX_D3D11_CALL(ctx.d3d11.immediate_context, OMSetRenderTargets, 1, &ctx.d3d11.target_view, NULL);
        mgfx_d3d11_begin_clear_render_target(ctx.d3d11.target_view, pass->clear, ctx.d3d11.width, ctx.d3d11.height);
        return;
    }

    ID3D11RenderTargetView *rtvs[MGFX_MAX_COLOR_ATTACHMENTS];
    uint32_t color_count = 0;
    uint32_t width = 0, height = 0;

    for (; color_count < MGFX_MAX_COLOR_ATTACHMENTS && pass->color_images[color_count]; color_count++)
    {
        mgfx_d3d11_image *img = (mgfx_d3d11_image*)pass->color_images[color_count];
        rtvs[color_count] = img->rtv;

        if (color_count == 0)
        {
            width = img->width;
            height = img->height;
        }
        else MGFX_ASSERT(img->width == width && img->height == height, "Mismatched color attachment sizes");
    }

    mgfx_d3d11_image *depth = (mgfx_d3d11_image*)pass->depth_stencil_image;
    if (depth)
    {
        if (color_count == 0)
        {
            width = depth->width;
            height = depth->height;
        }
        else MGFX_ASSERT(depth->width == width && depth->height == height, "Depth attachment size mismatch");
    }

    MGFX_D3D11_CALL(ctx.d3d11.immediate_context, OMSetRenderTargets, color_count, rtvs, depth ? depth->dsv : NULL);
    mgfx_d3d11_viewport(0, 0, width, height);
    mgfx_d3d11_scissor(0, 0, width, height);

    const float clear_color[4] = {
        pass->clear.r,
        pass->clear.g,
        pass->clear.b,
        pass->clear.a
    };

    for (uint32_t i = 0; i < color_count; i++)
        MGFX_D3D11_CALL(ctx.d3d11.immediate_context, ClearRenderTargetView, rtvs[i], clear_color);

    if (depth)
        MGFX_D3D11_CALL(ctx.d3d11.immediate_context, ClearDepthStencilView, depth->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

static mgfx_d3d11_pipeline *mgfx_d3d11_create_pipeline(const mgfx_pipeline_create_info *create_info)
{
    mgfx_d3d11_pipeline *pipeline = (mgfx_d3d11_pipeline*)calloc(1, sizeof(mgfx_d3d11_pipeline));
    
    ID3DBlob* error_blob = NULL;
    
    if (create_info->shader.compute.size)
    {
        ID3DBlob* cs_blob = NULL;
        const mgfx_shader_source *cs = &create_info->shader.compute;
        
        D3DCompile(cs->code, cs->size, NULL, NULL, NULL,
                    "main", "cs_5_0", 0, 0, &cs_blob, &error_blob
                    );
        
        MGFX_D3D11_CALL(ctx.d3d11.device, CreateComputeShader, MGFX_D3D11_CALL0(cs_blob, GetBufferPointer), MGFX_D3D11_CALL0(cs_blob, GetBufferSize), NULL, &pipeline->compute_shader);
        
        MGFX_D3D11_CALL0(cs_blob, Release);
        pipeline->type = MGFX_D3D11_PIPELINE_TYPE_COMPUTE;
        return pipeline;
    }
    
    ID3DBlob* vs_blob = NULL;
    ID3DBlob* ps_blob = NULL;
    
    const mgfx_shader_source *vs = &create_info->shader.vertex;
    const mgfx_shader_source *fg = &create_info->shader.fragment;
    
    D3DCompile(vs->code, vs->size, NULL, NULL, NULL,
                "main", "vs_5_0", 0, 0, &vs_blob, &error_blob
                );
    
    D3DCompile(fg->code, fg->size, NULL, NULL, NULL,
                "main", "ps_5_0", 0, 0, &ps_blob, &error_blob
                );
    
    if (error_blob)
    {
        fprintf(stderr, "%s\n", (char*)MGFX_D3D11_CALL0(error_blob, GetBufferPointer));
        MGFX_D3D11_CALL0(error_blob, Release);
    }
    
    void *vs_buffer_ptr = MGFX_D3D11_CALL0(vs_blob, GetBufferPointer);
    size_t vs_buffer_size = MGFX_D3D11_CALL0(vs_blob, GetBufferSize);
    MGFX_D3D11_CALL(ctx.d3d11.device, CreateVertexShader,
                    vs_buffer_ptr,
                    vs_buffer_size,
                    NULL,
                    &pipeline->vertex_shader
                    );
    
    MGFX_D3D11_CALL(ctx.d3d11.device, CreatePixelShader,
                    MGFX_D3D11_CALL0(ps_blob, GetBufferPointer),
                    MGFX_D3D11_CALL0(ps_blob, GetBufferSize),
                    NULL,
                    &pipeline->pixel_shader
                    );
    
    if (create_info->vertex_attributes[0])
    {
        D3D11_INPUT_ELEMENT_DESC layout[MGFX_MAX_VERTEX_ATTRIBUTES];
        
        uint32_t attribute_count = 0;
        uint32_t stride = 0;
        for (; attribute_count < MGFX_MAX_VERTEX_ATTRIBUTES && create_info->vertex_attributes[attribute_count]; attribute_count++)
        {
            const mgfx_vertex_format format = create_info->vertex_attributes[attribute_count];
            layout[attribute_count] = (D3D11_INPUT_ELEMENT_DESC){
                .SemanticName = "TEXCOORD",
                .SemanticIndex = attribute_count,
                .Format = mgfx_d3d11_get_vertex_format(format),
                .AlignedByteOffset = stride,
                .InputSlot = 0,
                .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0
            };
            stride += mgfx_vertex_format_size(format);
        }
        
        MGFX_D3D11_CALL(ctx.d3d11.device, CreateInputLayout, layout, attribute_count, vs_buffer_ptr, vs_buffer_size, &pipeline->vertex_layout);
        pipeline->layout_stride = stride;
    }
    else pipeline->vertex_layout = NULL;
    
    D3D11_RASTERIZER_DESC raster_desc = {
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = mgfx_d3d11_get_cull_mode(create_info->cull_mode),
        .FrontCounterClockwise = create_info->front_face == MGFX_FRONT_FACE_CCW,
        .DepthClipEnable = TRUE,
        .ScissorEnable = TRUE
    };
    MGFX_D3D11_CALL(ctx.d3d11.device, CreateRasterizerState, &raster_desc, &pipeline->raster_state);
    
    D3D11_DEPTH_STENCIL_DESC depth_stencil_state_desc = {
        .StencilEnable = create_info->depth_stencil.stencil_test_enabled
    };
    
    if (create_info->depth_stencil.depth_compare_op != MGFX_COMPARE_OP_NEVER)
    {
        depth_stencil_state_desc.DepthEnable = true;
        depth_stencil_state_desc.DepthWriteMask =
            create_info->depth_stencil.depth_write_enabled ?
            D3D11_DEPTH_WRITE_MASK_ALL :
        D3D11_DEPTH_WRITE_MASK_ZERO;
        depth_stencil_state_desc.DepthFunc = mgfx_d3d11_get_comparison_func(create_info->depth_stencil.depth_compare_op);
    }

    MGFX_D3D11_CALL(ctx.d3d11.device, CreateDepthStencilState, &depth_stencil_state_desc, &pipeline->depth_stencil_state);
    
    D3D11_BLEND_DESC blendDesc = {
        .RenderTarget[0].BlendEnable = create_info->color_blend.blend_enabled,
        .RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
    };

    if (create_info->color_blend.blend_enabled)
    {
        blendDesc.RenderTarget[0].SrcBlend = mgfx_d3d11_get_blend_factor(create_info->color_blend.src_color_blend_factor);
        blendDesc.RenderTarget[0].DestBlend = mgfx_d3d11_get_blend_factor(create_info->color_blend.dst_color_blend_factor);
        blendDesc.RenderTarget[0].BlendOp = mgfx_d3d11_get_blend_op(create_info->color_blend.color_blend_op);
        blendDesc.RenderTarget[0].SrcBlendAlpha = mgfx_d3d11_get_blend_factor(create_info->color_blend.src_alpha_blend_factor);
        blendDesc.RenderTarget[0].DestBlendAlpha = mgfx_d3d11_get_blend_factor(create_info->color_blend.dst_alpha_blend_factor);
        blendDesc.RenderTarget[0].BlendOpAlpha = mgfx_d3d11_get_blend_op(create_info->color_blend.alpha_blend_op);;
    }
    MGFX_D3D11_CALL(ctx.d3d11.device, CreateBlendState, &blendDesc, &pipeline->blend_state);
    
    pipeline->primitive_topology = mgfx_d3d11_get_primitive_topology(create_info->primitive_topology);
    
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_UNIFORMS && create_info->shader.uniform_blocks[i].size; i++)
    {
        D3D11_BUFFER_DESC buffer_desc = {
            .Usage = D3D11_USAGE_DYNAMIC,
            .ByteWidth = mgfx_stride_align(create_info->shader.uniform_blocks[i].size, 16),
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
        };
        uint32_t binding = create_info->shader.uniform_blocks[i].binding;
        MGFX_D3D11_CALL(ctx.d3d11.device, CreateBuffer, &buffer_desc, NULL, &pipeline->constant_buffers[binding]);
    }
    
    MGFX_D3D11_CALL0(vs_blob, Release);
    MGFX_D3D11_CALL0(ps_blob, Release);
    
    pipeline->type = MGFX_D3D11_PIPELINE_TYPE_GRAPHICS;
    
    return pipeline;
}

static void mgfx_d3d11_destroy_pipeline(mgfx_d3d11_pipeline *pipeline)
{
    if (pipeline->type == MGFX_D3D11_PIPELINE_TYPE_COMPUTE)
    {
        MGFX_D3D11_CALL0(pipeline->compute_shader, Release);
        free(pipeline);
        return;
    }
    
    MGFX_D3D11_CALL0(pipeline->vertex_shader, Release);
    MGFX_D3D11_CALL0(pipeline->pixel_shader, Release);
    MGFX_D3D11_CALL0(pipeline->raster_state, Release);
    MGFX_D3D11_CALL0(pipeline->depth_stencil_state, Release);
    MGFX_D3D11_CALL0(pipeline->blend_state, Release);
    if (pipeline->vertex_layout)
        MGFX_D3D11_CALL0(pipeline->vertex_layout, Release);
    for (uint32_t i = 0; i < MGFX_MAX_BINDABLE_UNIFORMS && pipeline->constant_buffers[i]; i++)
        MGFX_D3D11_CALL0(pipeline->constant_buffers[i], Release);
    free(pipeline);
}

static void mgfx_d3d11_bind_pipeline(mgfx_d3d11_pipeline *pipeline)
{
    ID3D11DeviceContext *context = ctx.d3d11.immediate_context;
    ctx.d3d11.current_pipeline = pipeline;
    
    if (pipeline->type == MGFX_D3D11_PIPELINE_TYPE_COMPUTE)
    {
        MGFX_D3D11_CALL(context, CSSetShader, pipeline->compute_shader, NULL, 0);
        MGFX_D3D11_CALL(context, CSSetConstantBuffers, 0, MGFX_MAX_BINDABLE_UNIFORMS, pipeline->constant_buffers);
        return;
    }
    
    MGFX_D3D11_CALL(context, VSSetShader, pipeline->vertex_shader, NULL, 0);
    MGFX_D3D11_CALL(context, PSSetShader, pipeline->pixel_shader, NULL, 0);
    MGFX_D3D11_CALL(context, IASetInputLayout, pipeline->vertex_layout);
    MGFX_D3D11_CALL(context, IASetPrimitiveTopology, pipeline->primitive_topology);
    MGFX_D3D11_CALL(context, RSSetState, pipeline->raster_state);
    MGFX_D3D11_CALL(context, OMSetDepthStencilState, pipeline->depth_stencil_state, 0);
    
    const float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    MGFX_D3D11_CALL(context, OMSetBlendState, pipeline->blend_state, blend_factor, 0xffffffff);
    
    MGFX_D3D11_CALL(context, VSSetConstantBuffers, 0, MGFX_MAX_BINDABLE_UNIFORMS, pipeline->constant_buffers);
    MGFX_D3D11_CALL(context, PSSetConstantBuffers, 0, MGFX_MAX_BINDABLE_UNIFORMS, pipeline->constant_buffers);
}

#endif // MGFX_D3D11

#define MGFX_PIPELINE_BIND(backend) \
do { \
ctx.init = mgfx_##backend##_init; \
ctx.shutdown   = mgfx_##backend##_shutdown; \
ctx.begin      = mgfx_##backend##_begin; \
ctx.end        = mgfx_##backend##_end; \
ctx.viewport   = mgfx_##backend##_viewport; \
ctx.scissor    = mgfx_##backend##_scissor; \
ctx.resize     = mgfx_##backend##_resize; \
ctx.vsync      = mgfx_##backend##_vsync; \
ctx.bind_uniforms  = mgfx_##backend##_bind_uniforms; \
ctx.bind_pass      = (mgfx_bind_pass_fn) mgfx_##backend##_bind_pass; \
ctx.create_pipeline = (mgfx_create_pipeline_fn) mgfx_##backend##_create_pipeline; \
ctx.destroy_pipeline= (mgfx_destroy_pipeline_fn)mgfx_##backend##_destroy_pipeline; \
ctx.bind_pipeline   = (mgfx_bind_pipeline_fn)   mgfx_##backend##_bind_pipeline; \
ctx.create_buffer  = (mgfx_create_buffer_fn)  mgfx_##backend##_create_buffer; \
ctx.destroy_buffer = (mgfx_destroy_buffer_fn) mgfx_##backend##_destroy_buffer; \
ctx.update_buffer  = (mgfx_update_buffer_fn)  mgfx_##backend##_update_buffer; \
ctx.bind_vertex_buffer = (mgfx_bind_vertex_buffer_fn) mgfx_##backend##_bind_vertex_buffer; \
ctx.bind_index_buffer  = (mgfx_bind_index_buffer_fn)  mgfx_##backend##_bind_index_buffer; \
ctx.create_image  = (mgfx_create_image_fn)  mgfx_##backend##_create_image; \
ctx.destroy_image = (mgfx_destroy_image_fn) mgfx_##backend##_destroy_image; \
ctx.update_image  = (mgfx_update_image_fn)  mgfx_##backend##_update_image; \
ctx.bind_image    = (mgfx_bind_image_fn)    mgfx_##backend##_bind_image; \
ctx.create_sampler = (mgfx_create_sampler_fn)  mgfx_##backend##_create_sampler; \
ctx.destroy_sampler  = (mgfx_destroy_sampler_fn) mgfx_##backend##_destroy_sampler; \
ctx.draw                     = mgfx_##backend##_draw; \
ctx.draw_indexed             = mgfx_##backend##_draw_indexed; \
ctx.draw_instanced           = mgfx_##backend##_draw_instanced; \
ctx.draw_indexed_instanced   = mgfx_##backend##_draw_indexed_instanced; \
ctx.dispatch                 = mgfx_##backend##_dispatch; \
} while (0)

void mgfx_init(const mgfx_init_info *init_info)
{
    MGFX_ASSERT(init_info->handle, "A platform handle must be provided to init_info.");

    mgfx_renderer_type type = init_info->type;
    
    switch (type)
    {
#if defined(MGFX_VULKAN)
        case MGFX_RENDERER_VULKAN:
        ctx.shader_lang = MGFX_SHADER_LANG_SPIRV;
        MGFX_PIPELINE_BIND(vk);
        break;
#endif
#if defined(MGFX_OPENGL)
#if !defined(__EMSCRIPTEN__)
        case MGFX_RENDERER_OPENGL:
        ctx.shader_lang = MGFX_SHADER_LANG_GLSL;
        MGFX_PIPELINE_BIND(gl);
        break;
#else
        case MGFX_RENDERER_OPENGLES:
        ctx.shader_lang = MGFX_SHADER_LANG_GLSLES;
        MGFX_PIPELINE_BIND(gl);
        break;
#endif
#endif
#if defined(MGFX_D3D11)
        case MGFX_RENDERER_D3D11:
        ctx.shader_lang = MGFX_SHADER_LANG_HLSL;
        MGFX_PIPELINE_BIND(d3d11);
        break;
#endif
    }
    
    ctx.type = type;
    ctx.init(init_info);
}

void mgfx_shutdown(void)
{
    ctx.shutdown();
}

mgfx_result mgfx_begin(void)
{
    return ctx.begin();
}

void mgfx_end(void)
{
    ctx.end();
}

void mgfx_viewport(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    ctx.viewport(x, y, width, height);
}

void mgfx_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    ctx.scissor(x, y, width, height);
}

void mgfx_resize(uint32_t width, uint32_t height)
{
    ctx.resize(width, height);
}

void mgfx_vsync(bool vsync)
{
    ctx.vsync(vsync);
}

void mgfx_bind_pass(const mgfx_pass_info *pass_info)
{
    ctx.bind_pass(pass_info);
}

void mgfx_draw(uint32_t vertex_count, uint32_t first_vertex)
{
    ctx.draw(vertex_count, first_vertex);
}

void mgfx_draw_indexed(uint32_t index_count, uint32_t first_index, int32_t vertex_offset)
{
    ctx.draw_indexed(index_count, first_index, vertex_offset);
}

void mgfx_draw_instanced(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    ctx.draw_instanced(vertex_count, first_vertex, instance_count, first_instance);
}

void mgfx_draw_indexed_instanced(uint32_t index_count, uint32_t first_index, int32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    ctx.draw_indexed_instanced(index_count, first_index, first_vertex, instance_count, first_instance);
}

void mgfx_dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    ctx.dispatch(group_count_x, group_count_y, group_count_z);
}

mgfx_pipeline mgfx_create_pipeline(const mgfx_pipeline_create_info *create_info)
{
    mgfx_pipeline pipeline;
    pipeline = ctx.create_pipeline(create_info);
    return pipeline;
}

void mgfx_destroy_pipeline(mgfx_pipeline pipeline)
{
    ctx.destroy_pipeline(pipeline);
}

void mgfx_bind_pipeline(mgfx_pipeline pipeline)
{
    ctx.bind_pipeline(pipeline);
}

mgfx_buffer mgfx_create_buffer(const mgfx_buffer_create_info *create_info)
{
    mgfx_buffer buffer;
    buffer = ctx.create_buffer(create_info);
    return buffer;
}

void mgfx_destroy_buffer(mgfx_buffer buffer)
{
    ctx.destroy_buffer(buffer);
}

void mgfx_update_buffer(mgfx_buffer buffer, size_t offset, size_t size, void *data)
{
    ctx.update_buffer(buffer, offset, size, data);
}

void mgfx_bind_vertex_buffer(mgfx_buffer buffer)
{
    ctx.bind_vertex_buffer(buffer);
}

void mgfx_bind_index_buffer(mgfx_buffer buffer, mgfx_index_type index_type)
{
    ctx.bind_index_buffer(buffer, index_type);
}

void mgfx_bind_uniforms(uint32_t binding, size_t size, void *data)
{
    ctx.bind_uniforms(binding, size, data);
}

mgfx_image mgfx_create_image(const mgfx_image_create_info *create_info)
{
    mgfx_image image;
    image = ctx.create_image(create_info);
    return image;
}

void mgfx_destroy_image(mgfx_image image)
{
    ctx.destroy_image(image);
}

void mgfx_update_image(mgfx_image image, size_t size, void *data)
{
    ctx.update_image(image, size, data);
}

void mgfx_bind_image(mgfx_image image, mgfx_sampler sampler, uint32_t binding)
{
    ctx.bind_image(image, sampler, binding);
}

mgfx_sampler mgfx_create_sampler(const mgfx_sampler_create_info *create_info)
{
    mgfx_sampler sampler;
    sampler = ctx.create_sampler(create_info);
    return sampler;
}

void mgfx_destroy_sampler(mgfx_sampler sampler)
{
    ctx.destroy_sampler(sampler);
}

mgfx_renderer_type mgfx_get_renderer_type(void)
{
    return ctx.type;
}

mgfx_shader_lang mgfx_get_shader_lang(void)
{
    return ctx.shader_lang;
}

#endif // MGFX_IMPL