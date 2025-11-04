/*
  ==============================================================================

    AudioCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioCue.h"

AudioCue::AudioCue(var params):
    audioManager(this)
{
    objectType = "Audio";
    addChildControllableContainer(&audioManager);

    if (audioManager.items.isEmpty()) {
        audioManager.addItemFromData(var());
    }
}

AudioCue::~AudioCue()
{
}

void AudioCue::play()
{
    Logger::writeToLog("AudioCue::play");
}
