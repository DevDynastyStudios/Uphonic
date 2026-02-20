#include "RenameTrackAction.h"
#include "DataModel/Tracks.h"
#include "UI/SongTimeline.h"

RenameTrackAction::RenameTrackAction(AudioTrack& track, std::string newName) : track(track), newName(newName), oldName(track.name) {}

void RenameTrackAction::Do()
{
	size_t trackIndex = SongTimeline::GetTrackIndex(track);
	SongTimeline::RenameTrack(trackIndex, newName);
}

void RenameTrackAction::Undo()
{
	size_t trackIndex = SongTimeline::GetTrackIndex(track);
	SongTimeline::RenameTrack(trackIndex, oldName);
}