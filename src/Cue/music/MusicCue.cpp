/*
  ==============================================================================

    MusicCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "MusicCue.h"
#include "../../Interface/InterfaceManager.h"

MusicCue::MusicCue(var params)
{
    objectType = "MusicCue";

    audioFile = addFileParameter("Audio File", "Audio file to play for this cue", params.getProperty("audioFile", ""));
    targetAudioInterface = addTargetParameter("Audio Interface", "Audio interface to play this cue through", InterfaceManager::getInstance());
    targetAudioInterface->targetType = TargetParameter::CONTAINER;
    targetAudioInterface->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;
}

MusicCue::~MusicCue()
{
}

void MusicCue::play()
{
    Logger::writeToLog("MusicCue::play");
}
