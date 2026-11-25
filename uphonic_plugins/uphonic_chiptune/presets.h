#pragma once

// The built-in patches. Every entry is a complete set of parameter values, so
// applying one never depends on what the knobs were before. Main thread only.

#define PRESET_MAX 32
#define PRESET_NAME_SIZE 24

typedef struct PresetEntry
{
	char name[PRESET_NAME_SIZE];
	double values[PARAM_COUNT];
	char sample_name[PCM_NAME_SIZE]; // The PCM sample by name, so the patch keeps it if the built-in list is reordered. May be empty.
} PresetEntry;

struct PresetList
{
	int count;
	PresetEntry entries[PRESET_MAX];
};

PresetList* presets_create(void);
void presets_destroy(PresetList* list);

// The PCM sample index the entry asks for, or -1 to leave it.
int presets_resolve_sample(const PresetEntry* entry, const PcmBank* bank);
