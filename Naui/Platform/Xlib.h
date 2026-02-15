#pragma once

#include "Window.h"

#include <cstdio>
#include <X11/Xlib.h>

#include <imgui.h>
#include <imgui_impl_xlib.h>

namespace Naui
{

class PlatformXlibWindow : public PlatformWindow
{
private:
	Display *m_dpy;
    Window m_window, m_rootWindow;
    Atom m_wmDeleteWindow;
    uint32_t m_width, m_height;
    bool m_isOpen = true;

    std::function<void(uint32_t, uint32_t)> m_resizeCallback = nullptr;
    std::function<void(const char *path)> m_fileDropCallback = nullptr;

public:
    PlatformXlibWindow(int width, int height, const char *title, PlatformWindow *parent);
    ~PlatformXlibWindow(void);

    void *GetNativeHandle(void) const override { return (void*)m_window; }
    bool IsOpen(void) const override { return m_isOpen; }
    uint32_t GetWidth(void) const override { return m_width; }
    uint32_t GetHeight(void) const override { return m_height; }

    void PollEvents(void) override;
    void Show(bool value) override;
    void Close(void) override { m_isOpen = false; };

    void SetResizeEvent(std::function<void(uint32_t, uint32_t)> callback) override { m_resizeCallback = callback; }
    void SetFileDropEvent(std::function<void(const char *path)> callback) override { m_fileDropCallback = callback; }
};

PlatformXlibWindow::PlatformXlibWindow(int width, int height, const char *title, PlatformWindow *parent)
{
    m_dpy = XOpenDisplay(nullptr);
    if (!m_dpy)
    {
        fprintf(stderr, "failed to open X11 display\n");
        return;
    }

    const int screen = DefaultScreen(m_dpy);
    if (parent)
    {
        m_rootWindow = (Window)parent->GetNativeHandle();
    }
    else
        m_rootWindow = RootWindow(m_dpy, screen);

    XSetWindowAttributes attr = {};
    attr.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

    m_window = XCreateWindow(
            m_dpy, m_rootWindow,
            0, 0, width, height,
            0, CopyFromParent, InputOutput, CopyFromParent,
            CWEventMask, &attr);
    if (!m_window)
    {
        fprintf(stderr, "failed to create X11 window\n");
    }
    XStoreName(m_dpy, m_window, title);

    m_wmDeleteWindow = XInternAtom(m_dpy, "WM_DELETE_WINDOW", False);
	if (!XSetWMProtocols(m_dpy, m_window, &m_wmDeleteWindow, 1))
    {
        fprintf(stderr, "failed to get WM_DELETE_WINDOW atom\n");
        return;
    }

    XMapWindow(m_dpy, m_window);
    XFlush(m_dpy);

    ImGui_ImplXlib_Init(m_dpy, m_window);

    m_width = width, m_height = height;
}

PlatformXlibWindow::~PlatformXlibWindow(void)
{
    ImGui_ImplXlib_Shutdown();
    XDestroyWindow(m_dpy, m_window);
	//XCloseDisplay(m_dpy);
}

void PlatformXlibWindow::PollEvents(void)
{
    XEvent ev;
    while (XPending(m_dpy) > 0)
    {
        XNextEvent(m_dpy, &ev);
        ImGui_ImplXlib_ProcessEvent(&ev);
        switch (ev.type)
        {
            case ClientMessage:
            {
                XClientMessageEvent *msg_ev = (XClientMessageEvent*)&ev;
                if ((Atom)msg_ev->data.l[0] == m_wmDeleteWindow)
                {
                    if (!m_rootWindow)
                        Show(false);
                    else
                        Close();
                }
            } break;

            case ConfigureNotify:
            {
                XConfigureEvent *conf_ev = (XConfigureEvent*)&ev;
                m_width = conf_ev->width;
                m_height = conf_ev->height;

                if (m_resizeCallback)
                    m_resizeCallback(m_width, m_height);
            } break;

            // TODO(smoke): file dropping, we might have to use the Xdnd protocol

            default: break;
        }
    }
}

void PlatformXlibWindow::Show(bool value)
{
    if (value)
        XMapRaised(m_dpy, m_window);
    else
        XUnmapWindow(m_dpy, m_window);
}

}
