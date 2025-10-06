#pragma once
#include <stdbool.h>

enum UphRecordType 
{
    UphRecordType_None,
    UphRecordType_Layout,
    UphRecordType_Panel_Visibility,
    UphRecordType_Project_Property,
    UphRecordType_Audio_Graph,
};

void uph_record_mark(UphRecordType type, const char* subject, const char* before, const char* after);
bool uph_record_is_dirty(void);
void uph_record_clear(void);
int  uph_record_count(void);
UphRecordType uph_record_type_at(int index);
const char*   uph_record_subject_at(int index);