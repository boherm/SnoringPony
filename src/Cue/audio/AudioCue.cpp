/*
  ==============================================================================

    AudioCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioCue.h"
#include "AudioFile.h"
#include "../../Audio/AudioPlayer.h"

AudioCue::AudioCue(var params)
{
    objectType = "Audio";
    formatManager.registerBasicFormats();
    filesManager = new AudioFilesManager(this);
    filesManager->addAsyncContainerListener(this);
    addChildControllableContainer(filesManager);

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

void AudioCue::play()
{
    if (isPlaying->boolValue())
        return;
    filesManager->playAll();
    resetRelativeGlobalGainForFade();
}

void AudioCue::stop()
{
    filesManager->stopAll();
}

void AudioCue::panic()
{
    filesManager->panicAll();
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
        }
    }
}

void AudioCue::setRelativeGlobalGainForFade(float gain, float percent)
{
    // Clamp t
    float t = percent;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Curve on t (gamma < 1 = slow at the end, gamma > 1 = slow at the beginning)
    float tCurve = std::pow(t, 2.7f);

    // Make the gain transition in all audio files
    for (auto& audioFile : filesManager->items)
    {
        // Set start and end gain, clamp to avoid log(0)
        float startGain = audioFile->volume->floatValue() * savedRelativeGain;
        float endGain   = gain;
        if (startGain <= 0.0f) startGain = 0.000001f;
        if (endGain   <= 0.0f) endGain   = 0.000001f;

        // Interpolate new gain
        float newGain = std::pow(
                10.0f,
                ( (1.0f - tCurve) * 20.0f * std::log10(startGain)
                  +        tCurve  * 20.0f * std::log10(endGain) ) / 20.0f
                );
        audioFile->player->transport->setGain(newGain);
    }
}

void AudioCue::resetRelativeGlobalGainForFade()
{
    savedRelativeGain = 1.0f;
    setRelativeGlobalGainForFade(1.0f, 1.0f);
}
