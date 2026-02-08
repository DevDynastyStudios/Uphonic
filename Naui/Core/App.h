#pragma once
#include "Base.h"
#include <string>

namespace Naui
{

class PlatformWindow;
class Renderer;

class NAUI_API App
{
public:
    virtual ~App(void) = default;
	virtual void OnMenuBar() { }

    void Run(std::string title, int width = 1280, int height = 720);

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