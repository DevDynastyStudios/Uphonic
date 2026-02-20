#pragma once
#include "Naui/Actions/IAction.h"

struct AudioTrack;

class ChangeTrackVolumeAction : public Naui::IAction
{
public:
	ChangeTrackVolumeAction(AudioTrack& track, float newVolume);
	void Do() override;
	void Undo() override;
	const char* Name() const override { return "Track Volume"; }

private:
	AudioTrack& track;
	float newVolume;
	float oldVolume;
};