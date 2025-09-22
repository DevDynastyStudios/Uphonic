#include "event.h"

static UphEvent registered_events[(uint8_t)UphSystemEventCode::MAX] = { nullptr };

void uph_event_connect(UphSystemEventCode code, UphEvent on_event)
{
    registered_events[(uint8_t)code] = on_event;
}

void uph_event_call(UphSystemEventCode code, void *data)
{
    if (registered_events[(uint8_t)code])
        registered_events[(uint8_t)code](data);
}