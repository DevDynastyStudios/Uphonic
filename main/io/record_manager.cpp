#include "record_manager.h"
#include "project_manager.h"
#include <vector>
#include <string>

struct UphRecordEvent 
{
    UphRecordType type;
    const char* subject;
    const char* before;
    const char* after;
};

static std::vector<UphRecordEvent> g_events;

void uph_record_mark(UphRecordType type, const char* subject, const char* before, const char* after) 
{
    uph_project_set_dirty(true);
    UphRecordEvent evt;
    evt.type    = type;
    evt.subject = subject ? subject : "";
    evt.before  = before  ? before  : "";
    evt.after   = after   ? after   : "";

    g_events.push_back(std::move(evt));

}

bool uph_record_is_dirty(void) 
{
    return uph_project_is_dirty();
}

void uph_record_clear(void) 
{
    uph_project_set_dirty(false);
    g_events.clear();
}

int uph_record_count(void) 
{
    return (int) g_events.size();
}

UphRecordType uph_record_type_at(int index) 
{
    if (index < 0 || index >= (int) g_events.size())
        return UphRecordType_None;

    return g_events[index].type;
}

const char* uph_record_subject_at(int index) 
{
    if (index < 0 || index >= (int) g_events.size())
        return "";

    return g_events[index].subject;
}