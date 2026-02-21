#pragma once

#include "Window.h"

#include <cstdio>
#include <vector>
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
    bool m_isChild = false;

    std::vector<PlatformXlibWindow*> m_children;

    std::function<void(uint32_t, uint32_t)> m_resizeCallback = nullptr;
    std::function<void(const char *path)> m_fileDropCallback = nullptr;

    void ProcessEvent(XEvent &ev);

public:
    PlatformXlibWindow(int width, int height, const char *title, PlatformWindow *parent);
    ~PlatformXlibWindow(void);

    void *GetNativeHandle(void) const override { return (void*)m_window; }
    bool IsOpen(void) const override { return m_isOpen; }
    uint32_t GetWidth(void) const override { return m_width; }
    uint32_t GetHeight(void) const override { return m_height; }

    void PollEvents(void) override;
    void Show(bool value) override;
    void Close(void) override { m_isOpen = false; }

    void SetResizeEvent(std::function<void(uint32_t, uint32_t)> callback) override { m_resizeCallback = callback; }
    void SetFileDropEvent(std::function<void(const char *path)> callback) override { m_fileDropCallback = callback; }
};

PlatformXlibWindow::PlatformXlibWindow(int width, int height, const char *title, PlatformWindow *parent)
{
    PlatformXlibWindow *parentXlibWindow = (PlatformXlibWindow*)parent;
    m_dpy = parent ? parentXlibWindow->m_dpy : XOpenDisplay(nullptr);

    if (!m_dpy)
    {
        fprintf(stderr, "failed to open X11 display\n");
        return;
    }

    const int screen = DefaultScreen(m_dpy);
    m_rootWindow = RootWindow(m_dpy, screen);

    m_isChild = (bool)parent;

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
        return;
    }

    XStoreName(m_dpy, m_window, title);

    m_wmDeleteWindow = XInternAtom(m_dpy, "WM_DELETE_WINDOW", False);
    if (!XSetWMProtocols(m_dpy, m_window, &m_wmDeleteWindow, 1))
    {
        fprintf(stderr, "failed to get WM_DELETE_WINDOW atom\n");
        return;
    }

    if (m_isChild)
    {
        XSetTransientForHint(m_dpy, m_window, (Window)parent->GetNativeHandle());
        parentXlibWindow->m_children.push_back(this);
    }

    XMapWindow(m_dpy, m_window);
    XFlush(m_dpy);

    if (!m_isChild)
        ImGui_ImplXlib_Init(m_dpy, m_window);

    m_width = width;
    m_height = height;
}

PlatformXlibWindow::~PlatformXlibWindow(void)
{
    if (!m_isChild)
        ImGui_ImplXlib_Shutdown();
    XDestroyWindow(m_dpy, m_window);
    if (!m_isChild)
        XCloseDisplay(m_dpy);
}

void PlatformXlibWindow::ProcessEvent(XEvent &ev)
{
    switch (ev.type)
    {
        case ClientMessage:
        {
            XClientMessageEvent *msg_ev = (XClientMessageEvent*)&ev;
            if ((Atom)msg_ev->data.l[0] == m_wmDeleteWindow)
            {
                if (!m_isChild)
                    Close();
                else
                    Show(false);
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

void PlatformXlibWindow::PollEvents(void)
{
    // Only the root (non-child) window drives the event loop, since all
    // windows on the same Display share a single event queue.
    if (m_isChild)
        return;

    XEvent ev;
    while (XPending(m_dpy) > 0)
    {
        XNextEvent(m_dpy, &ev);

        // Let ImGui see every event first
        if (!m_isChild)
            ImGui_ImplXlib_ProcessEvent(&ev);

        if (ev.xany.window == m_window)
        {
            ProcessEvent(ev);
        }
        else
        {
            for (PlatformXlibWindow *child : m_children)
            {
                if (ev.xany.window == child->m_window)
                {
                    child->ProcessEvent(ev);
                    break;
                }
            }
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