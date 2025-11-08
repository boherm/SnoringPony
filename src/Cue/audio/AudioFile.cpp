/*
  ==============================================================================

    AudioFile.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioFile.h"
#include "AudioCue.h"
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
    audioFile->fileTypeFilter = "*.wav;*.aiff;*.mp3;*.flac;*.ogg";

    duration = addFloatParameter("Duration", "Duration of the audio file to play", params.getProperty("duration", 0.0));
    duration->defaultUI = FloatParameter::TIME;
    duration->setEnabled(false);

    targetAudioInterface = addTargetParameter("Audio Output", "Audio output to play this cue through", InterfaceManager::getInstance());
    targetAudioInterface->targetType = TargetParameter::CONTAINER;
    targetAudioInterface->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;
}

void AudioFile::parameterValueChanged(Parameter* p)
{
    BaseItem::parameterValueChanged(p);
    clearWarning();

    if (p == audioFile)
    {
        File f = audioFile->getFile();

        if (f.existsAsFile()) {
            if (!Engine::mainEngine->isLoadingFile)
                setNiceName(f.getFileNameWithoutExtension());

            std::unique_ptr<AudioFormatReader> reader(audioCue->formatManager.createReaderFor(f));
            if (reader != nullptr && !Engine::mainEngine->isLoadingFile) {
                double fileDuration = reader->lengthInSamples / reader->sampleRate;
                duration->setValue(fileDuration);
            } else {
                setWarningMessage("Unsupported or unreadable audio file!");
            }

        } else {
            setWarningMessage("Audio file not found!");
        }
    }
}
