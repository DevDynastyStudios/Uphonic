#pragma once

#include "Base.h"
#include "Image.h"

class PlatformWindow;

namespace Naui
{

class NAUI_API Renderer
{
public:
    virtual ~Renderer(void) = default;
    virtual void Begin(void) = 0;
    virtual void End(void) = 0;
    
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual Image *CreateImage(uint32_t width, uint32_t height, const uint8_t *data) = 0;
    virtual void DestroyImage(Image *image) = 0;
};

Renderer *CreateRenderer(const PlatformWindow &window);

}