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
#include "../../Interface/audio/AudioOutput.h"

AudioFilesManager::AudioFilesManager(AudioCue* audioCue) :
    BaseManager("Audio Files"),
    audioCue(audioCue)
{
    selectItemWhenCreated = false;
}

AudioFile* AudioFilesManager::createItem()
{
	return new AudioFile(var(), audioCue);
}

void AudioFilesManager::playAll()
{
    Logger::writeToLog("AudioFilesManager::playAll");
    for (int i = 0; i < items.size(); ++i)
    {
        AudioFile* audioFile = items[i];
        audioFile->clearWarning();
        if (!audioFile->isEnabled())
            continue;

        File fileToPlay = audioFile->audioFile->getFile();
        if (!fileToPlay.existsAsFile()) {
            audioFile->setWarningMessage("Audio file not found!");
            continue;
        }

        ControllableContainer* targetContainer = audioFile->targetAudioInterface->getTargetContainer();
        AudioOutput* audioOutput = dynamic_cast<AudioOutput*>(targetContainer);
        if (audioOutput == nullptr) {
            audioFile->setWarningMessage("No audio output selected");
            continue;
        }
        if (!audioOutput->isEnabled()) {
            audioFile->setWarningMessage("Audio output disabled");
            continue;
        }

        std::unique_ptr<AudioFormatReader> reader(audioCue->formatManager.createReaderFor(fileToPlay));
        if (reader == nullptr) {
            audioFile->setWarningMessage("Unsupported or unreadable audio file");
            continue;
        }

        OutputPlaybackContext* context = getOrCreateOutputContext(audioOutput);
        if (context == nullptr) {
            audioFile->setWarningMessage("Unable to initialise audio playback for output");
            continue;
        }

        auto playback = std::make_unique<PlaybackInstance>();
        playback->context = context;
        playback->readerSource.reset(new AudioFormatReaderSource(reader.release(), true));
        playback->transport = std::make_unique<AudioTransportSource>();
        // playback->transport->addChangeListener(this);

        double sourceSampleRate = playback->readerSource->getAudioFormatReader()->sampleRate;
        playback->transport->setSource(playback->readerSource.get(), 0, nullptr, sourceSampleRate);

        context->mixer->addInputSource(playback->transport.get(), false);
        context->activeTransports += 1;

        playback->transport->start();

        activePlaybacks.push_back(std::move(playback));
    }
}

void AudioFilesManager::stopAll()
{
    for (auto& playback : activePlaybacks)
    {
        if (playback->context != nullptr && playback->context->mixer != nullptr && playback->transport != nullptr)
            playback->context->mixer->removeInputSource(playback->transport.get());

        if (playback->transport != nullptr)
        {
            // playback->transport->removeChangeListener(this);
            playback->transport->stop();
            playback->transport->setSource(nullptr);
        }
    }

    activePlaybacks.clear();

    while (!outputContexts.empty())
    {
        auto it = outputContexts.begin();
        releaseOutputContext(it->first);
    }
}

AudioFilesManager::OutputPlaybackContext* AudioFilesManager::getOrCreateOutputContext(AudioOutput* output)
{
    if (output == nullptr)
        return nullptr;

    auto it = outputContexts.find(output);
    if (it != outputContexts.end())
        return it->second.get();

    auto context = std::make_unique<OutputPlaybackContext>();
    context->output = output;
    context->mixer = std::make_unique<MixerAudioSource>();
    context->player = std::make_unique<AudioSourcePlayer>();
    context->player->setSource(context->mixer.get());

    output->am.addAudioCallback(context->player.get());

    OutputPlaybackContext* result = context.get();
    outputContexts.emplace(output, std::move(context));
    return result;
}

void AudioFilesManager::releaseOutputContext(AudioOutput* output)
{
    auto it = outputContexts.find(output);
    if (it == outputContexts.end())
        return;

    std::unique_ptr<OutputPlaybackContext> context = std::move(it->second);
    outputContexts.erase(it);

    if (context->player != nullptr)
    {
        if (context->output != nullptr)
            context->output->am.removeAudioCallback(context->player.get());

        context->player->setSource(nullptr);
    }

    if (context->mixer != nullptr)
        context->mixer->removeAllInputs();
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
    duration->hideInRemoteControl = true;
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
