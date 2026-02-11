/*
  ==============================================================================

    AudioSettings.cpp
    Created: 30 Dec 2025 01:43:20pm
    Author:  boherm

  ==============================================================================
*/

#include "AudioSettings.h"
#include "../Interface/InterfaceManager.h"

AudioSettings::AudioSettings() :
    ControllableContainer("Audio Settings"),
    previewOutput(nullptr)
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;

    previewOutput = addTargetParameter("Preview output", "Audio output to route previews", InterfaceManager::getInstance());
    previewOutput->targetType = TargetParameter::CONTAINER;
    previewOutput->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;
}

void AudioSettings::clear()
{
    previewOutput->resetValue();
}
