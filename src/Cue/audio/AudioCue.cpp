/*
  ==============================================================================

    AudioCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioCue.h"
#include "AudioFile.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Audio/AudioPlayer.h"

AudioCue::AudioCue(var params)
{
	itemDataType = "Audio Cue";
    loop = addBoolParameter("Loop", "If enabled, audio files will loop when they reach the end.", false);

    formatManager.registerBasicFormats();
    filesManager = new AudioFilesManager(this);
    filesManager->addAsyncContainerListener(this);
    addChildControllableContainer(filesManager);
    isFadable = true;

    if (filesManager->items.isEmpty()) {
        filesManager->addItemFromData(var());
    }

    duration->hideInRemoteControl = true;
    duration->setEnabled(false);
}

AudioCue::~AudioCue()
{
    filesManager->stopAll();
    filesManager->removeAsyncContainerListener(this);
    delete filesManager;
}

void AudioCue::newMessage(const ContainerAsyncEvent& e)
{
    if (
            e.source == filesManager && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate && e.targetControllable->niceName == "Duration" ||
            e.source == filesManager && e.type == ContainerAsyncEvent::EventType::ControllableContainerAdded ||
            e.source == filesManager && e.type == ContainerAsyncEvent::EventType::ControllableContainerRemoved
        )
    {
        double totalDuration = 0.0;
        for (auto& audioFile : filesManager->items)
        {
            if (totalDuration < audioFile->duration->doubleValue()) {
                totalDuration = audioFile->duration->doubleValue();
            }
        }
        duration->setValue(totalDuration);
    }
}

void AudioCue::playInternal()
{
    askedToStop = false;
    filesManager->playAll();
}

void AudioCue::stopInternal()
{
    filesManager->stopAll();
    askedToStop = true;
}

void AudioCue::panicInternal()
{
    filesManager->panicAll();
    askedToStop = true;
}

void AudioCue::timerCallback()
{
    double maxCurrentTime = 0.0;
    for (auto& audioFile : filesManager->items)
    {
        if (audioFile->player->transport != nullptr)
        {
            double currentTime = audioFile->player->transport->getCurrentPosition();
            maxCurrentTime = jmax(maxCurrentTime, currentTime);
        }
    }
    currentTime->setValue(maxCurrentTime);
}

void AudioCue::changeListenerCallback(ChangeBroadcaster* source)
{
    if (dynamic_cast<AudioTransportSource*>(source) != nullptr)
    {
        AudioTransportSource* transport = dynamic_cast<AudioTransportSource*>(source);
        if (transport->isPlaying()) {
            isPlaying->setValue(true);
            startTimerHz(10);
        }

        if (!transport->isPlaying() && !filesManager->haveOnePlaying()) {
            isPlaying->setValue(false);
            stopTimer();
            currentTime->setValue(0.0);

            if (loop->boolValue() && !askedToStop)
                filesManager->resetCurrentTime();

            if (!loop->boolValue() && !askedToStop)
                endCue();

            if (askedToStop && parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
                parentCuelist->currentCue->resetValue();
            }
        }
    }
}

void AudioCue::fade(double targetGain, double duration)
{
    for (auto& audioFile : filesManager->items)
    {
        audioFile->player->fade(targetGain, duration);
    }
}

String AudioCue::autoDescriptionInternal()
{
    String description = "Play: ";

    for (int i = 0; i < filesManager->items.size(); i++)
    {
        AudioFile* audioFile = filesManager->items[i];
        if (i > 0)
            description += ", ";
        description += audioFile->niceName;
    }

    return description;
}
