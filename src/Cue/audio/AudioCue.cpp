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
    isPlaying->setValue(true);
}

void AudioCue::stop()
{
    filesManager->stopAll();
    isPlaying->setValue(false);
}


// void AudioCue::changeListenerCallback(ChangeBroadcaster* source)
// {
    // Logger::writeToLog("AudioCue::changeListenerCallback called");
    // auto* transport = dynamic_cast<AudioTransportSource*>(source);
    // if (transport == nullptr)
    //     return;

    // if (transport->isPlaying())
    //     return;

    // for (auto it = activePlaybacks.begin(); it != activePlaybacks.end(); ++it)
    // {
    //     if ((*it)->transport.get() != transport)
    //         continue;

    //     std::unique_ptr<PlaybackInstance> finished = std::move(*it);
    //     activePlaybacks.erase(it);

    //     if (finished->context != nullptr && finished->context->mixer != nullptr)
    //         finished->context->mixer->removeInputSource(finished->transport.get());

    //     transport->removeChangeListener(this);
    //     transport->stop();
    //     transport->setSource(nullptr);

    //     finished->readerSource.reset();
    //     finished->transport.reset();

    //     if (finished->context != nullptr)
    //     {
    //         auto* ctx = finished->context;
    //         if (ctx->activeTransports > 0)
    //             --ctx->activeTransports;

    //         if (ctx->activeTransports == 0)
    //             releaseOutputContext(ctx->output);

    //         finished->context = nullptr;
    //     }

    //     return;
    // }
// }
