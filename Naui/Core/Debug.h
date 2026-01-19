#pragma once

#include "Base.h"

namespace Naui
{

class NAUI_API Debug
{
public:
    static void Error(const char *format, ...);

private:
    static void Render(void);
friend class App;
};

}