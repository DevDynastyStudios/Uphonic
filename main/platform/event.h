#pragma once

#include "base.h"

#include <functional>

typedef std::function<void(void *data)> UphEvent;

enum class UphSystemEventCode : uint8_t
{
    Quit,
    KeyPressed,
    KeyReleased,
    Char,
    FileDropped,
    MAX
};

void uph_event_connect(const UphSystemEventCode code, UphEvent on_event);
void uph_event_call(const UphSystemEventCode code, void *data);