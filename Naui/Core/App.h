#pragma once

#include "Base.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"

namespace Naui
{

class NAUI_API App
{
public:
    virtual ~App(void) = default;
	virtual void OnMenuBar() { }

    void Run(void);

    PlatformWindow *GetPlatformWindow(void) const { return m_window; }

private:
    virtual void OnEnter(void) { }
    virtual void OnExit(void) { }
    virtual void OnRender(void) { }
    virtual void OnFileDrop(const char *path) { }

    void Render(void);

    PlatformWindow *m_window = nullptr;
    Renderer *m_renderer = nullptr;
};

}