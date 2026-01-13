#pragma once

#include "Base.h"

#include <cstdint>
#include <functional>

namespace Naui
{

class NAUI_API PlatformWindow
{
public:
    virtual ~PlatformWindow(void) = default;

    virtual void *GetNativeHandle(void) const = 0;
    virtual bool IsOpen(void) const = 0;
    virtual void Close(void) = 0;

    virtual void PollEvents(void) = 0;

    virtual uint32_t GetWidth(void) const = 0;
    virtual uint32_t GetHeight(void) const = 0;

    virtual void Show(bool value) = 0;
    virtual void SetResizeEvent(std::function<void(uint32_t, uint32_t)> callback) = 0;
    virtual void SetFileDropEvent(std::function<void(const char *path)> callback) = 0;
};

NAUI_API PlatformWindow *CreatePlatformWindow(int width, int height, const char *title, PlatformWindow *parent = nullptr);

}
