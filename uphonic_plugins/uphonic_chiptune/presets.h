#pragma once

#define PRESET_MAX 32
#define PRESET_NAME_SIZE 24

typedef struct PresetEntry
{
	char name[PRESET_NAME_SIZE];
	double values[PARAM_COUNT];
	char sample_name[PCM_NAME_SIZE];
} PresetEntry;

struct PresetList
{
	int count;
	PresetEntry entries[PRESET_MAX];
};

PresetList* presets_create(void);
void presets_destroy(PresetList* list);

// Returns: PCM sample index or -1.
int presets_resolve_sample(const PresetEntry* entry, const PcmBank* bank);
