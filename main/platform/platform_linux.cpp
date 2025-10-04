#include "platform.h"

#if UPH_PLATFORM_LINUX

// NOTE(smoke): using SDL2 for this since
// ImGui doesn't have an Xlib (or xcb) backend...
// another excuse for me not having to deal with
// Xlib and modern opengl contexts using GLX ;)

// NOTE(smoke): me using 4coder, if u don't like
// the style of anything plz change it :)

#include "event.h"
#include "event_types.h"

#include <assert.h>
#include <cstdint>
#include <time.h>
#include <dlfcn.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

// NOTE(smoke): no need for GLAD, ImGui comes with
// it's own opengl loader (imgui_impl_opengl3_loader.h)

typedef struct UphSdl2Platform
{
    SDL_Window *window;
    uint32_t window_width, window_height;
    
    SDL_GLContext gl_context;
}
UphSdl2Platform;

static UphSdl2Platform *platform;
static double clock_frequency = 0.0000000001;
static struct timespec start_time;

void uph_platform_initialize(const UphPlatformCreateInfo *create_info)
{
    platform = (UphSdl2Platform*)malloc(sizeof(*platform));
    
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0 && "SDL2_Init failed");
    
    // NOTE(smoke): should we use an older opengl version
    // for compatiblity with older devices? afterall ImGui
    // is the one handling the graphics, not us.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    platform->window = SDL_CreateWindow(create_info->title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(create_info->width * main_scale), (int)(create_info->height * main_scale), window_flags);
    assert(window && "SDL_CreateWindow failed");
    platform->window_width = create_info->width;
    platform->window_height = create_info->height;
    
    platform->gl_context = SDL_GL_CreateContext(platform->window);
    assert(platform->gl_context && "SDL_GL_CreateContext failed");
    SDL_GL_MakeCurrent(platform->window, platform->gl_context);
    SDL_GL_SetSwapInterval(1);
    
    ImGui_ImplSDL2_InitForOpenGL(platform->window, platform->gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    
    // time
    clock_gettime(CLOCK_MONOTONIC, &start_time);
}

void uph_platform_shutdown(void)
{
    ImGui_ImplSDL2_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    
    SDL_GL_DeleteContext(platform->gl_context);
    SDL_DestroyWindow(platform->window);
    SDL_Quit();
    
    free(platform);
}

void uph_platform_begin(void)
{
    // NOTE(smoke): move event handling into a seperate function???
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            {
                UphQuitEvent data;
                uph_event_call(UphSystemEventCode::Quit, (void*)&data);
            }
            break;
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    platform->window_width = event.window.data1;
                    platform->window_height = event.window.data2;
                    glViewport(0, 0, platform->window_width, platform->window_height);
                }
            }
            break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                bool pressed = event.key.state == SDL_PRESSED;
                
                UphKeyEvent data;
                data.key = (UphKey)event.key.keysym.sym;
                uph_event_call(pressed ? UphSystemEventCode::KeyPressed : UphSystemEventCode::KeyReleased, (void*)&data);
            }
            break;
            
            // TODO(smoke): support for UphCharEvent
            default: break;
        }
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void uph_platform_end(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    SDL_GL_SwapWindow(platform->window);
}

// TODO(smoke): implement child windows
UphChildWindow uph_create_child_window(const UphChildWindowCreateInfo *create_info)
{
}

void uph_destroy_child_window(const UphChildWindow *window)
{
}

UphLibrary uph_load_library(const char *path)
{
    return (UphLibrary)dlopen(path, RTLD_LAZY);
}

UphProcAddress uph_get_proc_address(UphLibrary library, const char *name)
{
    return (UphProcAddress)dlsym((void*)library, name);
}

double uph_get_time(void)
{
    struct timespec now_time;
    clock_gettime(CLOCK_MONOTONIC, &now_time);
    return (double)(now_time.tv_sec - start_time.tv_sec) + (now_time.tv_nsec - start_time.tv_nsec) * clock_frequency;
}

#endif