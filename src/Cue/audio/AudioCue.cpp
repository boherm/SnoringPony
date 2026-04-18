/*
  ==============================================================================

    AudioCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioCue.h"
#include "AudioFile.h"
#include "AudioSlices.h"
#include "AudioWaveformSlicer.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Audio/AudioPlayer.h"
#include "../../Audio/PluginSlot.h"
#include "../../Interface/audio/AudioOutput.h"

AudioCue::AudioCue(var params)
{
	itemDataType = "Audio Cue";
    formatManager.registerBasicFormats();

    loop = addBoolParameter("Loop", "If enabled, audio files will loop when they reach the end.", false);

    initialDuration = addFloatParameter("Initial Duration", "Initial duration for new audio files added to this cue.", 0.0f);
    initialDuration->defaultUI = FloatParameter::TIME;
    initialDuration->setEnabled(false);
    initialDuration->hideInRemoteControl = true;
    initialDuration->hideInEditor = true;

    audioSlicer.reset(new ControllableContainer("Audio Slicer"));
    audioSlicer->editorIsCollapsed = true;
    audioSlicer->saveAndLoadRecursiveData = true;
    addChildControllableContainer(audioSlicer.get());

    slicesManager.reset(new AudioSlicesManager(this));
    waveformSlicer.reset(new AudioWaveformSlicer(this));
    slicesManager->addAsyncContainerListener(this);
    audioSlicer->addChildControllableContainer(waveformSlicer.get());
    audioSlicer->addChildControllableContainer(slicesManager.get());

    pluginChainManager = new PluginChainManager();
    pluginChainManager->editorIsCollapsed = true;
    pluginChainManager->addAsyncContainerListener(this);

    filesManager = new AudioFilesManager(this);
    filesManager->addAsyncContainerListener(this);
    addChildControllableContainer(filesManager);
    addChildControllableContainer(pluginChainManager);

    showWarningInUI = true;
    isFadable = true;

    if (filesManager->items.isEmpty()) {
        filesManager->addItemFromData(var());
    }

    duration->hideInRemoteControl = true;
    duration->setEnabled(false);
}

AudioCue::~AudioCue()
{
    pluginChainManager->removeAsyncContainerListener(this);
    slicesManager->removeAsyncContainerListener(this);
    filesManager->stopAll();
    for (auto& audioFile : filesManager->items)
        audioFile->player->mixer->setPluginChain(nullptr);
    filesManager->removeAsyncContainerListener(this);
    delete filesManager;
    delete pluginChainManager;
}

void AudioCue::updateWarnings()
{
    StringArray warnings;

    for (auto& af : filesManager->items)
    {
        File f = af->audioFile->getFile();
        if (f == File() || !f.existsAsFile())
            warnings.add("Missing audio file: " + af->niceName);

        AudioOutput* out = af->targetAudioInterface->getTargetContainerAs<AudioOutput>();
        if (out == nullptr)
            warnings.add("No audio output: " + af->niceName);
    }

    if (pluginChainManager->hasPluginWarnings())
        warnings.add(pluginChainManager->getPluginWarningMessage());

    if (warnings.isEmpty())
        clearWarning();
    else
        setWarningMessage(warnings.joinIntoString("\n"));
}

bool AudioCue::canBePlayed()
{
    return Cue::canBePlayed() && getWarningMessage().isEmpty();
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
        initialDuration->setValue(totalDuration);
        duration->setValue(slicesManager->getTotalDuration());
    }

    if (e.source == slicesManager.get()) {
        duration->setValue(slicesManager->getTotalDuration());
    }

    if (e.source == filesManager || e.source == pluginChainManager)
        updateWarnings();
}

void AudioCue::playInternal()
{
    if (isPreviewing) {
        filesManager->stopAll();
        isPreviewing = false;
    }

    askedToStop = false;
    slicesManager->resetSlices();

    for (auto& audioFile : filesManager->items)
        audioFile->player->mixer->setPluginChain(pluginChainManager);

    double fadeInTime = slicesManager->fadeInDuration->doubleValue();
    if (fadeInTime > 0.0)
    {
        for (auto& audioFile : filesManager->items)
            audioFile->player->mixer->fadeIn(fadeInTime);
        filesManager->playAll(false);
    }
    else
    {
        filesManager->playAll();
    }
}

void AudioCue::previewInternal()
{
    if (isPreviewing) {
        askedToStop = true;
        filesManager->stopAll();
        isPreviewing = false;
    } else {
        askedToStop = false;
        isPreviewing = true;

        for (auto& audioFile : filesManager->items)
            audioFile->player->mixer->setPluginChain(pluginChainManager);

        double fadeInTime = slicesManager->fadeInDuration->doubleValue();
        if (fadeInTime > 0.0)
        {
            for (auto& audioFile : filesManager->items)
                audioFile->player->mixer->fadeIn(fadeInTime);
            filesManager->previewAll(false);
        }
        else
        {
            filesManager->previewAll();
        }
    }
}

void AudioCue::stopInternal()
{
    filesManager->stopAll();
    for (auto& audioFile : filesManager->items)
        audioFile->player->mixer->setPluginChain(nullptr);
    slicesManager->resetSlices();
    askedToStop = true;
}

void AudioCue::retriggerStop()
{
    isRetriggerStopping = true;
    double fadeOutTime = retriggerStopFadeOut->doubleValue();
    if (fadeOutTime > 0.0)
    {
        askedToStop = true;
        retriggerFadeStartTime = Time::getMillisecondCounterHiRes();
        duration->setValue(fadeOutTime);
        currentTime->setValue(0.0);
        fadeOut(fadeOutTime, true);
    }
    else
    {
        stop();
    }
}

void AudioCue::panicInternal()
{
    filesManager->panicAll();
    askedToStop = true;
}

void AudioCue::timerCallback()
{
    if (isRetriggerStopping)
    {
        double elapsed = (Time::getMillisecondCounterHiRes() - retriggerFadeStartTime) / 1000.0;
        currentTime->setValue(jmin(elapsed, duration->doubleValue()));
        return;
    }

    double time = slicesManager->processTime(filesManager->getCurrentTime());
    currentTime->setValue(jmax(0.0, time));

    bool isLooping = slicesManager->hasLoopingSlice();

    if (!isLooping)
    {
        double fadeOutTime = slicesManager->fadeOutDuration->doubleValue();
        double totalDuration = duration->doubleValue();

        if (fadeOutTime > 0.0 && !slicesManager->fadeOutTriggered && time >= totalDuration - fadeOutTime)
        {
            slicesManager->fadeOutTriggered = true;
            fadeOut(fadeOutTime, false);
        }

        if (time >= totalDuration) {
            if (isPreviewing) {
                askedToStop = true;
            }
            filesManager->stopAll();
            filesManager->resetCurrentTime();
            slicesManager->resetSlices();
        }
    }
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
            isPanicking = false;
            isPreviewing = false;
            slicesManager->resetSlices();

            if (loop->boolValue() && !askedToStop)
                filesManager->resetCurrentTime();

            if (!loop->boolValue() && !askedToStop && !isPreviewing)
                endCue();

            if (askedToStop) {
                if (isRetriggerStopping) {
                    isRetriggerStopping = false;
                    duration->setValue(slicesManager->getTotalDuration());
                    endCue();
                } else if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
                    parentCuelist->currentCue->resetValue();
                }
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

void AudioCue::fadeIn(double duration)
{
    for (auto& audioFile : filesManager->items)
    {
        audioFile->player->fadeIn(duration);
    }
}

void AudioCue::fadeOut(double duration, bool stopAfterFade)
{
    for (auto& audioFile : filesManager->items)
    {
        audioFile->player->fadeOut(duration, stopAfterFade);
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

void AudioCue::createFromFiles(const StringArray& files)
{
    filesManager->clear();
    for (auto& f : files)
    {
        AudioFile* audioFile = filesManager->addItemFromData(var());
        audioFile->audioFile->setValue(f);
    }
}
