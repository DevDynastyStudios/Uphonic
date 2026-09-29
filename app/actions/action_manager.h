#define UPH_ACTION_PATTERN_CREATE "Pattern Create"
#define UPH_ACTION_TRACK_CREATE "Track Create"
#define UPH_ACTION_TRACK_RENAME "Track Rename"
#define UPH_ACTION_AUTOMATION_CREATE "Automation Create"

typedef struct
{
	uint32_t resource_index;
	bool resource_exists;
} Uph_ActionResourceCreate;

void uph_action_initialize(void);
void uph_action_shutdown(void);