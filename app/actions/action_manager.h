#define UPH_ACTION_PATTERN_CREATE "PatternCreate"
#define UPH_ACTION_TRACK_CREATE "TrackCreate"
#define UPH_ACTION_TRACK_RENAME "TrackRename"
#define UPH_ACTION_AUTOMATION_CREATE "AutomationCreate"

typedef struct
{
	uint32_t resource_index;
	bool resource_exists;
} Uph_ActionResourceCreate;

void uph_action_initialize(void);
void uph_action_shutdown(void);