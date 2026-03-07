#include "ChangeTrackVolumeAction.h"
#include "Models/DataModel/Tracks.h"

ChangeTrackVolumeAction::ChangeTrackVolumeAction(AudioTrack& track, float newVolume) : track(track), newVolume(newVolume), oldVolume(track.volume) {}

void ChangeTrackVolumeAction::Do()
{ 
	track.volume = newVolume;
}

void ChangeTrackVolumeAction::Undo()
{
	track.volume = oldVolume;
}