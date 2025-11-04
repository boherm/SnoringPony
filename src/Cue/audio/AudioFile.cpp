/*
  ==============================================================================

    AudioFile.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioFile.h"
#include "../../Interface/InterfaceManager.h"

AudioManager::AudioManager(AudioCue* audioCue) :
    BaseManager("Audio Files"),
    audioCue(audioCue)
{
    selectItemWhenCreated = false;
}

AudioFile* AudioManager::createItem()
{
	return new AudioFile(var(), audioCue);
}

//==============================================================================

AudioFile::AudioFile(var params, AudioCue* audioCue) :
    BaseItem("File 1"),
    audioCue(audioCue)
{
    // TODO: add preview button on selected preview output? (set in projectsettings?)
    userCanRemove = true;
    canBeDisabled = true;
    showWarningInUI = true;

    audioFile = addFileParameter("Audio File", "Audio file to play for this cue", params.getProperty("audioFile", ""));
    targetAudioInterface = addTargetParameter("Audio Output", "Audio output to play this cue through", InterfaceManager::getInstance());
    targetAudioInterface->targetType = TargetParameter::CONTAINER;
    targetAudioInterface->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;
}
