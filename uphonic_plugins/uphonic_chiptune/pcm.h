#pragma once

// The sample channel's data: a fixed set of built-in 8-bit sounds. A bank
// never changes once built, so the audio thread can read it at any time.

#define PCM_MAX_SAMPLES 32
#define PCM_NAME_SIZE 24
#define PCM_DEFAULT_ROOT_KEY 60

typedef struct PcmSample
{
	char name[PCM_NAME_SIZE];
	float* data; // Mono, -1..1.
	uint32_t length;
	uint32_t rate; // The rate the sample was made at, not the host's.
	int root_key; // The key that plays it at its own speed.
} PcmSample;

typedef struct PcmBank
{
	int count;
	PcmSample samples[PCM_MAX_SAMPLES];
} PcmBank;

PcmBank* pcm_bank_create(void);
void pcm_bank_destroy(PcmBank* bank);

const PcmSample* pcm_bank_get(const PcmBank* bank, int index);
int pcm_bank_find(const PcmBank* bank, const char* name);
