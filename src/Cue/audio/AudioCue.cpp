/*
  ==============================================================================

    AudioCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioCue.h"
#include "AudioFile.h"

#include "../../Interface/audio/AudioOutput.h"

AudioCue::AudioCue(var params)
{
    objectType = "Audio";
    formatManager.registerBasicFormats();
    audioManager = new AudioManager(this);
    audioManager->addAsyncContainerListener(this);
    addChildControllableContainer(audioManager);

    if (audioManager->items.isEmpty()) {
        audioManager->addItemFromData(var());
    }

    duration->setEnabled(false);
}

AudioCue::~AudioCue()
{
    // stopAllPlaybacks();
    audioManager->removeAsyncContainerListener(this);
    delete audioManager;
}

void AudioCue::newMessage(const ContainerAsyncEvent& e)
{
    if (
            e.source == audioManager && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate && e.targetControllable->niceName == "Duration" ||
            e.source == audioManager && e.type == ContainerAsyncEvent::EventType::ControllableContainerAdded ||
            e.source == audioManager && e.type == ContainerAsyncEvent::EventType::ControllableContainerRemoved
        )
    {
        double totalDuration = 0.0;
        for (auto& audioFile : audioManager->items)
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
    Logger::writeToLog("AudioCue::play: " + niceName);

//     if (audioManager == nullptr)
//     {
//         Logger::writeToLog("AudioCue::play aborted: audioManager is null");
//         return;
//     }

//     for (int i = 0; i < audioManager->items.size(); ++i)
//     {
//         AudioFile* audioItem = audioManager->items[i];
//         if (audioItem == nullptr)
//             continue;

//         if (!audioItem->isEnabled())
//             continue;

//         audioItem->clearWarning();

//         if (audioItem->audioFile == nullptr)
//         {
//             audioItem->setWarningMessage("Audio file parameter is not initialised", "audioFileParam");
//             continue;
//         }

//         File fileToPlay = audioItem->audioFile->getFile();
//         if (!fileToPlay.existsAsFile())
//         {
//             audioItem->setWarningMessage("Audio file not found: " + fileToPlay.getFullPathName(), "audioFileMissing");
//             continue;
//         }

//         if (audioItem->targetAudioInterface == nullptr)
//         {
//             audioItem->setWarningMessage("Audio output target parameter is not initialised", "audioTargetParam");
//             continue;
//         }

//         ControllableContainer* targetContainer = audioItem->targetAudioInterface->getTargetContainer();
//         AudioOutput* audioOutput = dynamic_cast<AudioOutput*>(targetContainer);

//         if (audioOutput == nullptr)
//         {
//             audioItem->setWarningMessage("No audio output selected", "audioOutputMissing");
//             continue;
//         }

//         if (!audioOutput->isEnabled())
//         {
//             audioItem->setWarningMessage("Selected audio output is disabled", "audioOutputDisabled");
//             continue;
//         }

//         if (audioOutput->am.getCurrentAudioDevice() == nullptr)
//         {
//             audioItem->setWarningMessage("Selected audio output has no active device", "audioOutputDevice");
//             continue;
//         }

//         std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(fileToPlay));
//         if (reader == nullptr)
//         {
//             audioItem->setWarningMessage("Unsupported or unreadable audio file: " + fileToPlay.getFullPathName(), "audioReader");
//             continue;
//         }

//         OutputPlaybackContext* context = getOrCreateOutputContext(audioOutput);
//         if (context == nullptr)
//         {
//             audioItem->setWarningMessage("Unable to initialise audio playback for output " + audioOutput->niceName, "audioOutputInit");
//             continue;
//         }

//         auto playback = std::make_unique<PlaybackInstance>();
//         playback->context = context;
//         playback->readerSource.reset(new AudioFormatReaderSource(reader.release(), true));
//         playback->transport = std::make_unique<AudioTransportSource>();
//         playback->transport->addChangeListener(this);

//         double sourceSampleRate = playback->readerSource->getAudioFormatReader()->sampleRate;
//         playback->transport->setSource(playback->readerSource.get(), 0, nullptr, sourceSampleRate);

//         context->mixer->addInputSource(playback->transport.get(), false);
//         context->activeTransports += 1;

//         playback->transport->start();

//         activePlaybacks.push_back(std::move(playback));
//     }
}

// AudioCue::OutputPlaybackContext* AudioCue::getOrCreateOutputContext(AudioOutput* output)
// {
//     if (output == nullptr)
//         return nullptr;

//     auto it = outputContexts.find(output);
//     if (it != outputContexts.end())
//         return it->second.get();

//     auto context = std::make_unique<OutputPlaybackContext>();
//     context->output = output;
//     context->mixer = std::make_unique<MixerAudioSource>();
//     context->player = std::make_unique<AudioSourcePlayer>();
//     context->player->setSource(context->mixer.get());

//     output->am.addAudioCallback(context->player.get());

//     OutputPlaybackContext* result = context.get();
//     outputContexts.emplace(output, std::move(context));
//     return result;
// }

// void AudioCue::releaseOutputContext(AudioOutput* output)
// {
//     auto it = outputContexts.find(output);
//     if (it == outputContexts.end())
//         return;

//     std::unique_ptr<OutputPlaybackContext> context = std::move(it->second);
//     outputContexts.erase(it);

//     if (context->player != nullptr)
//     {
//         if (context->output != nullptr)
//             context->output->am.removeAudioCallback(context->player.get());

//         context->player->setSource(nullptr);
//     }

//     if (context->mixer != nullptr)
//         context->mixer->removeAllInputs();
// }

// void AudioCue::stopAllPlaybacks()
// {
//     for (auto& playback : activePlaybacks)
//     {
//         if (playback->context != nullptr && playback->context->mixer != nullptr && playback->transport != nullptr)
//             playback->context->mixer->removeInputSource(playback->transport.get());

//         if (playback->transport != nullptr)
//         {
//             playback->transport->removeChangeListener(this);
//             playback->transport->stop();
//             playback->transport->setSource(nullptr);
//         }
//     }

//     activePlaybacks.clear();

//     while (!outputContexts.empty())
//     {
//         auto it = outputContexts.begin();
//         releaseOutputContext(it->first);
//     }
// }

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
