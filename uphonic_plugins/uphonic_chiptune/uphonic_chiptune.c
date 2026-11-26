#include <ctype.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upl.h"

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

#define SYNTH_MAX_VOICES 16

#include "pcm.h"
#include "synth.h"
#include "note_queue.h"
#include "plugin.h"
#include "presets.h"
#include "gui.h"

#include "note_queue.c"
#include "pcm.c"
#include "synth.c"
#include "params.c"
#include "presets.c"
#include "gui_draw.c"
#include "gui_view.c"

#define GUI_NOTE_PORT 100
#define GUI_NOTE_CHANNEL 0
#define GUI_NOTE_VELOCITY 0.8

#define HOST_NOTE_PORT 0

static const char* const k_features[] = { "instrument", "synthesizer", "stereo", NULL };

#pragma region Private API

static void uct__drain_gui_notes(Plugin* plugin)
{
	QueuedNote note;

	while(note_queue_pop(&plugin->gui_notes, &note))
	{
		if(note.is_on)
			synth_note_on(plugin->synth, SYNTH_UNSPECIFIED, GUI_NOTE_PORT, GUI_NOTE_CHANNEL, note.key, GUI_NOTE_VELOCITY);
		else
			synth_note_off(plugin->synth, SYNTH_UNSPECIFIED, GUI_NOTE_PORT, GUI_NOTE_CHANNEL, note.key);
	}
}

static void* uct__create(Upl_Plugin* upl)
{
	Plugin* plugin = (Plugin*)calloc(1, sizeof(Plugin));
	if(plugin == NULL)
		return NULL;

	plugin->upl = upl;
	plugin->synth = synth_create();
	plugin->pcm_bank = pcm_bank_create();
	plugin->presets = presets_create();
	plugin->gui_view = gui_view_create(plugin);

	if(plugin->synth == NULL || plugin->pcm_bank == NULL || plugin->presets == NULL || plugin->gui_view == NULL)
	{
		gui_view_destroy(plugin->gui_view);
		presets_destroy(plugin->presets);
		pcm_bank_destroy(plugin->pcm_bank);
		synth_destroy(plugin->synth);
		free(plugin);
		return NULL;
	}

	synth_set_pcm_bank(plugin->synth, plugin->pcm_bank);
	return plugin;
}

static void uct__destroy(Upl_Plugin* upl)
{
	Plugin* plugin = (Plugin*)upl_user_data(upl);
	if(plugin == NULL)
		return;

	gui_view_destroy(plugin->gui_view);
	presets_destroy(plugin->presets);
	synth_destroy(plugin->synth);
	pcm_bank_destroy(plugin->pcm_bank);
	free(plugin);
}

static void uct__activate(Upl_Plugin* upl, double sample_rate, uint32_t max_frame_count)
{
	Plugin* plugin = (Plugin*)upl_user_data(upl);
	if(plugin == NULL)
		return;

	synth_activate(plugin->synth, sample_rate, max_frame_count);
}

static void uct__deactivate(Upl_Plugin* upl)
{
	Plugin* plugin = (Plugin*)upl_user_data(upl);
	if(plugin == NULL)
		return;

	synth_deactivate(plugin->synth);
}

static void uct__process(Upl_Plugin* upl, const Upl_AudioBlock* block)
{
	Plugin* plugin = (Plugin*)upl_user_data(upl);

	if(plugin == NULL || block->output_channels < 2)
	{
		for(uint32_t c = 0; c < block->output_channels; c++)
		{
			memset(block->outputs[c], 0, sizeof(float) * block->frame_count);
		}

		return;
	}

	SynthParams params;
	params_read_synth(plugin, &params);
	synth_set_params(plugin->synth, &params);

	uct__drain_gui_notes(plugin);

	synth_render(plugin->synth, block->outputs[0], block->outputs[1], block->frame_count);
}

static void uct__note(Upl_Plugin* upl, const Upl_NoteEvent* event)
{
	Plugin* plugin = (Plugin*)upl_user_data(upl);
	if(plugin == NULL)
		return;

	switch(event->type)
	{
		case UPL_NOTE_ON:
			synth_note_on(plugin->synth, event->note_id, HOST_NOTE_PORT, event->channel, event->key, event->velocity);
			return;

		case UPL_NOTE_OFF:
			synth_note_off(plugin->synth, event->note_id, HOST_NOTE_PORT, event->channel, event->key);
			return;

		case UPL_NOTE_CHOKE:
			synth_note_choke(plugin->synth, event->note_id, HOST_NOTE_PORT, event->channel, event->key);
			return;
	}
}

static void uct__ui(Upl_Plugin* upl)
{
	Plugin* plugin = (Plugin*)upl_user_data(upl);
	if(plugin == NULL)
		return;

	gui_view_frame(plugin->gui_view);
}

#pragma endregion

#pragma region Public API

void plugin_gui_note(Plugin* plugin, bool is_on, int key)
{
	QueuedNote note = { .key = (int16_t)key, .is_on = is_on };
	note_queue_push(&plugin->gui_notes, note);
}

const Upl_Descriptor upl_descriptor =
{
	.id = "com.ddstudios.uphonic-chiptunes",
	.name = "Uphonic Chiptune",
	.vendor = "Dev Dynasty",
	.version = "0.3.0",
	.description = "Uphonic Chiptune Instrument: pulse, triangle, saw and noise with arpeggio, sweep and lo-fi",
	.features = k_features,
	.params = k_param_infos,
	.param_count = PARAM_COUNT,
	.input_channels = 0,
	.output_channels = 2,
	.note_input = true,
	.gui_width = GUI_LOGICAL_WIDTH * GUI_BASE_PIXEL_SCALE,
	.gui_height = GUI_LOGICAL_HEIGHT * GUI_BASE_PIXEL_SCALE,
	.create = uct__create,
	.destroy = uct__destroy,
	.activate = uct__activate,
	.deactivate = uct__deactivate,
	.process = uct__process,
	.note = uct__note,
	.ui = uct__ui,
};

#pragma endregion
