/*
  ==============================================================================

    AudioFile.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioFile.h"
#include "AudioCue.h"
#include "../../Audio/AudioPlayer.h"
#include "../../Interface/InterfaceManager.h"
#include "../../Interface/audio/AudioOutput.h"

AudioFilesManager::AudioFilesManager(AudioCue* audioCue) :
    BaseManager("Audio Files"),
    audioCue(audioCue)
{
    selectItemWhenCreated = false;
}

AudioFilesManager::~AudioFilesManager()
{
}

AudioFile* AudioFilesManager::createItem()
{
	return new AudioFile(var(), audioCue);
}

void AudioFilesManager::playAll()
{
    for (int i = 0; i < items.size(); ++i)
    {
        AudioFile* audioFile = items[i];
        audioFile->player->play();
    }
}

void AudioFilesManager::stopAll()
{
    for (int i = 0; i < items.size(); ++i)
    {
        AudioFile* audioFile = items[i];
        audioFile->player->stop();
    }
}

void AudioFilesManager::panicAll()
{
    for (int i = 0; i < items.size(); ++i)
    {
        AudioFile* audioFile = items[i];
        audioFile->player->panic();
    }
}

bool AudioFilesManager::haveOnePlaying()
{
    for (int i = 0; i < items.size(); ++i)
    {
        AudioFile* audioFile = items[i];
        if (audioFile->player->transport->isPlaying())
            return true;
    }
    return false;
}

void AudioFilesManager::resetCurrentTime()
{
    for (int i = 0; i < items.size(); ++i)
    {
        AudioFile* audioFile = items[i];
        audioFile->player->transport->setPosition(0.0);
        audioFile->player->transport->start();
    }
}

//==============================================================================

AudioFile::AudioFile(var params, AudioCue* audioCue) :
    BaseItem("File 1"),
    audioCue(audioCue)
{
    player = std::make_unique<AudioPlayer>();
    player->transport->addChangeListener(audioCue);

    // TODO: add preview button to play/stop preview on a selected preview output? (set in projectsettings?)
    userCanRemove = true;
    canBeDisabled = true;
    showWarningInUI = true;

    audioFile = addFileParameter("Audio File", "Audio file to play for this cue", params.getProperty("audioFile", ""));
    audioFile->fileTypeFilter = "*.wav;*.aiff;*.mp3;*.flac;*.ogg";

    duration = addFloatParameter("Duration", "Duration of the audio file to play", params.getProperty("duration", 0.0));
    duration->minimumValue = 0.0f;
    duration->isSavable = false;
    duration->defaultUI = FloatParameter::TIME;
    duration->hideInRemoteControl = true;
    duration->setEnabled(false);

    targetAudioInterface = addTargetParameter("Audio Output", "Audio output to play this cue through", InterfaceManager::getInstance());
    targetAudioInterface->targetType = TargetParameter::CONTAINER;
    targetAudioInterface->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;

    volume = addFloatParameter("Volume", "Volume for this audio file", params.getProperty("volume", 1.0), 0.0, 1.5, 0.01);
}

AudioFile::~AudioFile()
{
    player->stopAndClean();
    player->transport->removeChangeListener(audioCue);
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

            if (!player->setFile(f)) {
                setWarningMessage("Unsupported or unreadable audio file!");
            } else {
                duration->setValue(player->duration);
            }

        } else {
            setWarningMessage("Audio file not found!");
        }
    }

    if (p == targetAudioInterface)
    {
        ControllableContainer* targetContainer = targetAudioInterface->getTargetContainer();
        AudioOutput* audioOutput = dynamic_cast<AudioOutput*>(targetContainer);
        if (audioOutput == nullptr) {
            setWarningMessage("No audio output selected");
        }

        player->setOutput(audioOutput);
    }

    if (p == volume)
        player->transport->setGain(volume->floatValue());
}

void AudioFile::parameterControlModeChanged(Parameter* p)
{
    if (p == volume && volume->controlMode == Parameter::ControlMode::REFERENCE)
    {
        volume->referenceTarget->setRootContainer(ProjectSettings::getInstance()->getControllableContainerByName("volumePresets"));
    }
}
