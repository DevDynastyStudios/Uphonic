#include "RenameSampleAction.h"
#include "UI/Panels/SampleRack.h"
#include "Models/DataModel/Samples.h"

RenameSampleAction::RenameSampleAction(AudioSample& sample, std::string newName) : sample(sample), newName(newName), oldName(sample.name) {}

void RenameSampleAction::Do()
{ 
	size_t sampleIndex = SampleRack::GetSampleIndex(sample);
	SampleRack::RenameSample(sampleIndex, newName);
}

void RenameSampleAction::Undo()
{
	size_t sampleIndex = SampleRack::GetSampleIndex(sample);
	SampleRack::RenameSample(sampleIndex, oldName);
}