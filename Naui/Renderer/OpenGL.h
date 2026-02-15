#pragma once

#include "Base.h"
#include "Renderer.h"
#include "Platform/Window.h"

#include <cstdio>

#include <X11/Xlib.h>
#include <EGL/egl.h>

#include <imgui.h>
#include <imgui_impl_xlib.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>

namespace Naui
{

class OpenGLImage : public Image
{
public:
    OpenGLImage(uint32_t width, uint32_t height);
    ~OpenGLImage(void);

    uint32_t GetWidth(void) const override { return m_width; }
    uint32_t GetHeight(void) const override { return m_height; }
    void *GetNativeHandle(void) const override { return (void*)&m_texId; }

private:
    uint32_t m_width, m_height;
    uint32_t m_texId;
};

OpenGLImage::OpenGLImage(uint32_t width, uint32_t height)
    : m_width(width), m_height(height)
{
    glGenTextures(1, &m_texId);
}

OpenGLImage::~OpenGLImage(void)
{
    glDeleteTextures(1, &m_texId);
}

class OpenGLRenderer : public Renderer
{
public:
    OpenGLRenderer(const PlatformWindow &window);
    ~OpenGLRenderer(void);

    void Begin(void) override;
    void End(void) override;
    
    void Resize(uint32_t width, uint32_t height) override;

    Image *CreateImage(uint32_t width, uint32_t height, const uint8_t *data) override;
    void DestroyImage(Image *image) override;

private:
    bool CreateEGLContext(Window win);
    void CleanupEGLContext(void);

	Display *m_xdpy;
    EGLDisplay m_dpy;
    EGLSurface m_surface;
    EGLContext m_glContext;
};

OpenGLRenderer::OpenGLRenderer(const PlatformWindow &window)
{
	Display *m_xdpy = XOpenDisplay(nullptr);
	if (!m_xdpy)
	{
		fprintf(stderr, "failed to open X11 display\n");
		return;
	}
    if (!CreateEGLContext((Window)window.GetNativeHandle()))
    {
        CleanupEGLContext();
        return;
    }
    ImGui_ImplOpenGL3_Init("#version 130");
}

OpenGLRenderer::~OpenGLRenderer(void)
{
    ImGui_ImplOpenGL3_Shutdown();
    CleanupEGLContext();
	//XCloseDisplay(m_xdpy);
}

bool OpenGLRenderer::CreateEGLContext(Window win)
{
    m_dpy = eglGetDisplay((EGLNativeDisplayType)m_xdpy);
    eglInitialize(m_dpy, nullptr, nullptr);

    EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(m_dpy, configAttribs, &config, 1, &numConfigs);

    m_surface = eglCreateWindowSurface(m_dpy, config, (EGLNativeWindowType)win, NULL);
    if (!m_surface)
    {
        fprintf(stderr, "failed to create EGL surface\n");
        return false;
    }
    
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_API);
    m_glContext = eglCreateContext(m_dpy, config, EGL_NO_CONTEXT, contextAttribs);
    if (!m_glContext)
    {
        fprintf(stderr, "failed to create EGL context\n");
        return false;
    }
    eglMakeCurrent(m_dpy, m_surface, m_surface, m_glContext);

    eglSwapInterval(m_dpy, 1); // vsync

    return true;
}

void OpenGLRenderer::CleanupEGLContext(void)
{
    eglMakeCurrent(m_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_dpy, m_glContext);
    eglDestroySurface(m_dpy, m_surface);
    eglTerminate(m_dpy);
}

void OpenGLRenderer::Begin(void)
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplXlib_NewFrame();
}

void OpenGLRenderer::End(void)
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(m_dpy, m_surface);
}

void OpenGLRenderer::Resize(uint32_t width, uint32_t height)
{
    glViewport(0, 0, width, height);
}

Image *OpenGLRenderer::CreateImage(uint32_t width, uint32_t height, const uint8_t *data)
{
    OpenGLImage *image = new OpenGLImage(width, height);

    uint32_t texId = *(uint32_t*)image->GetNativeHandle();
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 0x2601); // GL_LINEAR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 0x2601); // GL_LINEAR
    glTexParameteri(GL_TEXTURE_2D, 0x2802, 0x2901); // GL_TEXTURE_WRAP_S, GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, 0x2803, 0x2901); // GL_TEXTURE_WRAP_T, GL_REPEAT
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return image;
}

void OpenGLRenderer::DestroyImage(Image *image)
{
    delete image;
}

}
