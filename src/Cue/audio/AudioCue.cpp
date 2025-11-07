/*
  ==============================================================================

    AudioCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioCue.h"
#include "AudioFile.h"

AudioCue::AudioCue(var params)
{
    objectType = "Audio";
    audioManager = new AudioManager(this);
    addChildControllableContainer(audioManager);

    if (audioManager->items.isEmpty()) {
        audioManager->addItemFromData(var());
    }
}

AudioCue::~AudioCue()
{
    delete audioManager;
}

void AudioCue::play()
{
    Logger::writeToLog("AudioCue::play");
}
