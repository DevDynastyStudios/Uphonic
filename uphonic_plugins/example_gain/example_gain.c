#include <math.h>

#include "upl.h"

enum
{
	GAIN_PARAM_GAIN,
	GAIN_PARAM_PAN,
	GAIN_PARAM_BYPASS,
	GAIN_PARAM_COUNT
};

static const Upl_ParamInfo gain__params[GAIN_PARAM_COUNT] = {
	[GAIN_PARAM_GAIN] = { .name = "Gain", .unit = "dB", .min = -60.0, .max = 12.0, .default_value = 0.0 },
	[GAIN_PARAM_PAN] = { .name = "Pan", .min = -1.0, .max = 1.0, .default_value = 0.0 },
	[GAIN_PARAM_BYPASS] = { .name = "Bypass", .min = 0.0, .max = 1.0, .default_value = 0.0, .flags = UPL_PARAM_BYPASS }
};

static const char *const gain__features[] = { "audio-effect", "utility", "stereo", NULL };

static void gain__process(Upl_Plugin *plugin, const Upl_AudioBlock *block)
{
	const bool bypass = upl_param(plugin, GAIN_PARAM_BYPASS) >= 0.5;
	const float gain = bypass ? 1.0f : powf(10.0f, (float)upl_param(plugin, GAIN_PARAM_GAIN) / 20.0f);
	const float pan = bypass ? 0.0f : (float)upl_param(plugin, GAIN_PARAM_PAN);
	const float channel_gain[2] = { gain * fminf(1.0f, 1.0f - pan), gain * fminf(1.0f, 1.0f + pan) };

	for (uint32_t c = 0; c < block->output_channels; c++)
	{
		const float *input = c < block->input_channels ? block->inputs[c] : (block->input_channels > 0 ? block->inputs[0] : NULL);
		float *output = block->outputs[c];
		const float g = block->output_channels == 2 ? channel_gain[c] : gain;

		for (uint32_t i = 0; i < block->frame_count; i++)
			output[i] = input ? input[i] * g : 0.0f;
	}
}

static void gain__control(uint32_t param, const char *label)
{
	const Upl_Theme *theme = upl_theme();
	leaf({
		.direction = LEAF_DIRECTION_VERTICAL,
		.size = { .width = LEAF_SIZE_FIT, .height = LEAF_SIZE_FIT },
		.child_alignment = { .x = LEAF_ALIGN_X_CENTER },
		.child_gap = (int32_t)UPL_DPI(8)
	})
	{
		leaf_text(label, { .color = theme->text_dim, .font_size = LEAF_SIZE_FIXED((int32_t)UPL_DPI(13)) });

		if (param == GAIN_PARAM_PAN)
			upl_slider(param, (int32_t)UPL_DPI(120), (int32_t)UPL_DPI(18));
		else
			upl_knob(param, (int32_t)UPL_DPI(72));

		upl_param_label(param, theme->text, (int32_t)UPL_DPI(13));
	}
}

static void gain__ui(Upl_Plugin *plugin)
{
	(void)plugin;
	const Upl_Theme *theme = upl_theme();

	leaf({
		.direction = LEAF_DIRECTION_VERTICAL,
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.padding = LEAF_PADDING_ALL((int32_t)UPL_DPI(16)),
		.child_gap = (int32_t)UPL_DPI(16),
		.color = theme->background
	})
	{
		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_FIT },
			.child_alignment = { .y = LEAF_ALIGN_Y_CENTER }
		})
		{
			leaf_text("Gain", { .color = theme->text, .font_size = LEAF_SIZE_FIXED((int32_t)UPL_DPI(18)) });
			leaf({ .size = { .width = LEAF_SIZE_GROW } }) {}
			leaf_text("Bypass", { .color = theme->text_dim, .font_size = LEAF_SIZE_FIXED((int32_t)UPL_DPI(13)) });
			leaf({ .size = { .width = LEAF_SIZE_FIXED((int32_t)UPL_DPI(8)) } }) {}
			upl_toggle(GAIN_PARAM_BYPASS, (int32_t)UPL_DPI(36), (int32_t)UPL_DPI(20));
		}

		leaf({
			.direction = LEAF_DIRECTION_HORIZONTAL,
			.size = { .width = LEAF_SIZE_GROW, .height = LEAF_SIZE_GROW },
			.padding = LEAF_PADDING_ALL((int32_t)UPL_DPI(16)),
			.child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
			.child_gap = (int32_t)UPL_DPI(40),
			.color = theme->panel,
			.rounding = LEAF_ROUNDING_FIXED((int32_t)UPL_DPI(theme->rounding), LEAF_CORNER_ALL)
		})
		{
			gain__control(GAIN_PARAM_GAIN, "Gain");
			gain__control(GAIN_PARAM_PAN, "Pan");
		}
	}
}

const Upl_Descriptor upl_descriptor = {
	.id = "com.uphonic.example-gain",
	.name = "Example Gain",
	.vendor = "Uphonic",
	.version = "1.0.0",
	.description = "Example plugin built with the Uphonic plugin lib",
	.features = gain__features,
	.params = gain__params,
	.param_count = GAIN_PARAM_COUNT,
	.input_channels = 2,
	.output_channels = 2,
	.gui_width = 360,
	.gui_height = 240,
	.process = gain__process,
	.ui = gain__ui
};
