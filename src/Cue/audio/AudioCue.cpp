/*
  ==============================================================================

    AudioCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioCue.h"
#include "../../Interface/InterfaceManager.h"

AudioCue::AudioCue(var params)
{
    objectType = "Audio";

    audioFile = addFileParameter("Audio File", "Audio file to play for this cue", params.getProperty("audioFile", ""));
    targetAudioInterface = addTargetParameter("Audio Interface", "Audio interface to play this cue through", InterfaceManager::getInstance());
    targetAudioInterface->targetType = TargetParameter::CONTAINER;
    targetAudioInterface->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;
}

AudioCue::~AudioCue()
{
}

void AudioCue::play()
{
    Logger::writeToLog("AudioCue::play");
}
