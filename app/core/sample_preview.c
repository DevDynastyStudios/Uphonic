typedef struct
{
	Uph_TimelineBlock block;
	Uph_ResourceIndex sample_index;
	double position;
	double buffer_position;
	bool playing;
	bool loop;
}
Uph_SamplePreviewData;

static Uph_SamplePreviewData uph_sample_preview_data;

#pragma region Main thread control

void uph_sample_preview_play(Uph_ResourceIndex sample_index)
{
	uph_sample_preview_data.sample_index = sample_index;
	uph_sample_preview_data.playing = true;
}

void uph_sample_preview_pause(void)
{
	uph_sample_preview_data.playing = false;
}

void uph_sample_preview_stop(void)
{
	uph_sample_preview_pause();
	uph_sample_preview_seek(0.0);
}

void uph_sample_preview_seek(double beat)
{
	uph_sample_preview_data.position = fmax(0.0, beat);
}

void uph_sample_preview_set_loop(bool loop)
{
	uph_sample_preview_data.loop = loop;
}

bool uph_sample_preview_looping(void)
{
	return uph_sample_preview_data.loop;
}

bool uph_sample_preview_playing(void)
{
	return uph_sample_preview_data.playing;
}

double uph_sample_preview_position(void)
{
	return uph_sample_preview_data.position;
}

double uph_sample_preview_length(const Uph_Sample *sample, const Uph_SampleData *data)
{
	return uph_sample_playback_length_beats(sample, data, uph_state.project.bpm);
}

#pragma endregion

#pragma region Audio thread

void uph_sample_preview_mix(float *out, uint32_t frame_count, uint32_t sample_rate, float bpm, bool timeline_playing)
{
	Uph_SamplePreviewData *p = &uph_sample_preview_data;

	if (p->playing && timeline_playing)
		p->playing = false;

	if (!p->playing || sample_rate == 0 || frame_count == 0)
		return;

	Uph_Project *project = &uph_state.project;
	if (p->sample_index >= (uint32_t)naui_list_len(project->samples))
	{
		p->playing = false;
		return;
	}

	const Uph_Sample *sample = &project->samples[p->sample_index];
	if (sample->data_index >= (uint32_t)naui_list_len(project->sample_data))
	{
		p->playing = false;
		return;
	}

	const Uph_SampleData *data = &project->sample_data[sample->data_index];
	const double length = uph_sample_preview_length(sample, data);
	if (!data->frames || length <= 0.0)
	{
		p->playing = false;
		return;
	}

	bool region = p->loop && uph_loop_region_valid(&sample->loop_region) && data->frame_count > 0;
	double loop_start = 0.0;
	double loop_end = length;
	if (region)
	{
		const double seconds_to_beats = (double)sample_rate / (double)data->frame_count * length;
		loop_start = NAUI_CLAMP(sample->loop_region.start * seconds_to_beats, 0.0, length);
		loop_end = NAUI_CLAMP(sample->loop_region.end * seconds_to_beats, 0.0, length);
		region = loop_end > loop_start;
	}

	p->buffer_position = p->position;
	double start = p->buffer_position;
	if (region)
	{
		if (start >= loop_end && start >= length)
			start = loop_start;
	}
	else if (start >= length)
		start = 0.0;

	const double wrap_end = region && start < loop_end ? loop_end : length;

	p->block = (Uph_TimelineBlock){
		.start_beat = 0.0,
		.length_beats = wrap_end,
		.resource_index = p->sample_index,
		.type = UPH_RESOURCE_SAMPLE
	};

	float *left = alloca(frame_count * sizeof(float));
	float *right = alloca(frame_count * sizeof(float));
	memset(left, 0, frame_count * sizeof(float));
	memset(right, 0, frame_count * sizeof(float));

	const float preview_bpm = project->bpm > 0.0f ? project->bpm : 120.0f;
	uph_render_sample_block(&p->block, sample, data, start, frame_count, sample_rate, preview_bpm, left, right);

	for (uint32_t f = 0; f < frame_count; f++)
	{
		out[f * 2 + 0] += left[f];
		out[f * 2 + 1] += right[f];
	}

	double next = start + uph_seconds_to_beats((double)frame_count / (double)sample_rate, preview_bpm);
	if (next >= wrap_end)
	{
		if (p->loop)
			next = region ? loop_start : 0.0;
		else
		{
			next = 0.0;
			p->playing = false;
		}
	}

	if (p->position == p->buffer_position)
		p->position = next;
}

#pragma endregion
