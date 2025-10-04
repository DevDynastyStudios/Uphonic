#pragma once

#include "input.h"

struct UphQuitEvent { };
struct UphKeyEvent { UphKey key; };
struct UphCharEvent { char ch; };
struct UphFileDropEvent { const char *path; };