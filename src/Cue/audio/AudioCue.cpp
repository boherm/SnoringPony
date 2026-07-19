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
#include "../../Cuelist/CuelistManager.h"
#include "../CueManager.h"
#include "../playlist/PlaylistCue.h"
#include "../../Audio/AudioPlayer.h"
#include "../../Audio/PluginSlot.h"
#include "../../Interface/audio/AudioOutput.h"
#include "../../Interface/midi/MTCSender.h"
#include "ui/AudioMultiFilesEditor.h"

AudioCue::AudioCue(var params)
{
	itemDataType = "Audio Cue";
    formatManager.registerBasicFormats();

    retriggerStopCC->hideInEditor = false;

    loop = addBoolParameter("Loop", "If enabled, audio files will loop when they reach the end.", false);

    volume = addFloatParameter("Volume", "Master volume for this cue, combined with each file's own volume", 1.0, 0.0, 1.5, 0.01);

    initialDuration = addFloatParameter("Initial Duration", "Initial duration for new audio files added to this cue.", 0.0f);
    initialDuration->defaultUI = FloatParameter::TIME;
    initialDuration->setEnabled(false);
    initialDuration->hideInRemoteControl = true;
    initialDuration->hideInEditor = true;

    // MTC: emit this cue's audio transport position as SMPTE.
    mtcSender.reset(new MTCSender(this, [this] { return filesManager->getCurrentTime(); }));
    addChildControllableContainer(mtcSender->createContainer(), true);

    // --- Duck others ---
    duckPrePlayTimer = new Cue::CueTimer(this);
    duckPostPlayTimer = new Cue::CueTimer(this);
    duckFadeInTimer = new Cue::CueTimer(this);

    duckOthersCC = new EnablingControllableContainer("Duck others");
    duckOthersCC->enabled->setValue(false);
    duckOthersCC->editorIsCollapsed = true;
    // Place it right below "Post-wait" (added by the base Cue constructor) rather than at the
    // bottom of the container list.
    addChildControllableContainer(duckOthersCC, true, controllableContainers.indexOf(postWaitCC) + 1);

    duckVolume = duckOthersCC->addFloatParameter("Volume", "Gain the other cues are ducked down to (1 = unchanged, 0 = silent)", 0.1, 0.0, 1.0);
    duckFadeOutDuration = duckOthersCC->addFloatParameter("Fade Out", "Fade out duration applied to the other playing cues when this cue starts", 5.0, 0.0);
    duckPrePlayDuration = duckOthersCC->addFloatParameter("Pre-play", "Delay before this cue's audio starts", 5.0, 0.0);
    duckPostPlayDuration = duckOthersCC->addFloatParameter("Post-play", "Delay after this cue's audio ends, before the others fade back in", 5.0, 0.0);
    duckFadeInDuration = duckOthersCC->addFloatParameter("Fade In", "Fade in duration used to bring the other cues back", 5.0, 0.0);
    for (auto* p : { duckFadeOutDuration, duckPrePlayDuration, duckPostPlayDuration, duckFadeInDuration })
        p->defaultUI = FloatParameter::TIME;

    duckPrePlayCurrentTime = duckOthersCC->addFloatParameter("Pre-play current time", "Current time of the pre-play delay", 0.0, 0.0);
    duckPostPlayCurrentTime = duckOthersCC->addFloatParameter("Post-play current time", "Current time of the post-play delay", 0.0, 0.0);
    duckFadeInCurrentTime = duckOthersCC->addFloatParameter("Fade-in current time", "Current time of the fade-in (restore) phase", 0.0, 0.0);
    for (auto* p : { duckPrePlayCurrentTime, duckPostPlayCurrentTime, duckFadeInCurrentTime })
    {
        p->defaultUI = FloatParameter::TIME;
        p->setEnabled(false);
        p->isSavable = false;
        p->hideInEditor = true;
    }

    duckActive = duckOthersCC->addBoolParameter("Active", "Duck sequence currently active", false);
    duckPrePlayActive = duckOthersCC->addBoolParameter("Pre-play active", "Pre-play delay currently running", false);
    duckPostPlayActive = duckOthersCC->addBoolParameter("Post-play active", "Post-play delay currently running", false);
    duckFadeInActive = duckOthersCC->addBoolParameter("Fade-in active", "Fade-in (restore) phase currently running", false);
    for (auto* p : { duckActive, duckPrePlayActive, duckPostPlayActive, duckFadeInActive })
    {
        p->setEnabled(false);
        p->hideInEditor = true;
        p->isSavable = false;
    }

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
        // addToUndo=false: the default file is part of building the cue, not its own undo
        // step. (With undo it fires a nested undo action, which corrupts a multi-item
        // undo transaction during undo/redo — e.g. deleting several cues at once.)
        filesManager->addItemFromData(var(), false);
    }

    duration->hideInRemoteControl = true;
    duration->setEnabled(false);
}

AudioCue::~AudioCue()
{
    duckPrePlayTimer->stop();
    duckPostPlayTimer->stop();
    duckFadeInTimer->stop();
    delete duckPrePlayTimer;
    delete duckPostPlayTimer;
    delete duckFadeInTimer;

    // Stop MTC before the rest of the teardown so its high-res timer can't fire mid-destruction.
    if (mtcSender != nullptr)
        mtcSender->stop();

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
    // When "Duck others" is enabled, launching the cue first fades the other cues out and waits
    // the pre-play delay before this cue's audio actually starts.
    if (duckOthersCC->enabled->boolValue())
    {
        startDuckSequence();
        return;
    }

    startAudioPlayback();
}

void AudioCue::startAudioPlayback()
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

    mtcSender->start();

    // If another cue is currently ducking the others, start already ducked to that volume so this
    // cue doesn't blast at full over the ducked mix. It's restored with the rest when the ducker ends.
    double duckGain = getActiveDuckGain(this);
    if (duckGain < 1.0)
        fade(duckGain, 0.0);
}

Array<Cue*> AudioCue::getOtherPlayingAudioCues()
{
    Array<Cue*> result;
    for (Cuelist* cl : CuelistManager::getInstance()->items)
    {
        if (cl == nullptr || cl->cues == nullptr) continue;
        for (Cue* c : cl->cues->items)
        {
            if (c == nullptr || c == this) continue;
            if (!c->isPlaying->boolValue()) continue;
            if (dynamic_cast<AudioCue*>(c) != nullptr || dynamic_cast<PlaylistCue*>(c) != nullptr)
                result.add(c);
        }
    }
    return result;
}

double AudioCue::getActiveDuckGain(Cue* except)
{
    double gain = 1.0;
    for (Cuelist* cl : CuelistManager::getInstance()->items)
    {
        if (cl == nullptr || cl->cues == nullptr) continue;
        for (Cue* c : cl->cues->items)
        {
            auto* ac = dynamic_cast<AudioCue*>(c);
            if (ac == nullptr || ac == except) continue;
            // A cue that's actively ducking others (but not yet restoring them) holds them down.
            if (ac->duckOthersCC->enabled->boolValue()
                && ac->duckActive->boolValue()
                && !ac->duckFadeInActive->boolValue())
                gain = jmin(gain, ac->duckVolume->doubleValue());
        }
    }
    return gain;
}

void AudioCue::startDuckSequence()
{
    duckActive->setValue(true);

    // Fade the other cues down to the duck volume, running in parallel with the pre-play delay.
    double fadeOutTime = duckFadeOutDuration->doubleValue();
    double target = duckVolume->doubleValue();
    for (Cue* c : getOtherPlayingAudioCues())
        c->fade(target, fadeOutTime);

    // Pre-play gates the start of this cue's audio.
    double prePlay = duckPrePlayDuration->doubleValue();
    if (prePlay > 0.0)
    {
        duckPrePlayActive->setValue(true);
        duckPrePlayTimer->start(prePlay, duckPrePlayCurrentTime);
    }
    else
        startAudioPlayback();
}

void AudioCue::startDuckRestore()
{
    // Called when this cue's audio finished naturally: wait post-play, then fade the others in.
    double postPlay = duckPostPlayDuration->doubleValue();
    if (postPlay > 0.0)
    {
        duckPostPlayActive->setValue(true);
        duckPostPlayTimer->start(postPlay, duckPostPlayCurrentTime);
    }
    else
    {
        startFadeInPhase();
    }
}

void AudioCue::fadeOthersBackIn()
{
    double fadeInTime = duckFadeInDuration->doubleValue();
    for (Cue* c : getOtherPlayingAudioCues())
        c->fade(1.0, fadeInTime);
}

void AudioCue::startFadeInPhase()
{
    // Fade the ducked cues back up, tracking the fade-in duration as its own phase so the Active
    // Cues panel can show a "Restore ducked volume" progress bar. The duck ends once it completes.
    fadeOthersBackIn();

    double fadeIn = duckFadeInDuration->doubleValue();
    if (fadeIn > 0.0)
    {
        duckFadeInActive->setValue(true);
        duckFadeInTimer->start(fadeIn, duckFadeInCurrentTime);
    }
    else
    {
        duckActive->setValue(false);
    }
}

void AudioCue::cancelDuckSequence(bool fadeInImmediately)
{
    duckPrePlayTimer->stop();
    duckPostPlayTimer->stop();
    duckFadeInTimer->stop();
    duckPrePlayActive->setValue(false);
    duckPostPlayActive->setValue(false);
    duckFadeInActive->setValue(false);
    duckPrePlayCurrentTime->setValue(0.0f);
    duckPostPlayCurrentTime->setValue(0.0f);
    duckFadeInCurrentTime->setValue(0.0f);

    if (duckActive->boolValue() && fadeInImmediately)
        fadeOthersBackIn();

    duckActive->setValue(false);
}

void AudioCue::onCueTimerFinished(Cue::CueTimer* timer)
{
    if (timer == duckPrePlayTimer)
    {
        duckPrePlayActive->setValue(false);
        duckPrePlayCurrentTime->setValue(0.0f);
        startAudioPlayback();
        return;
    }
    if (timer == duckPostPlayTimer)
    {
        duckPostPlayActive->setValue(false);
        duckPostPlayCurrentTime->setValue(0.0f);
        startFadeInPhase();
        return;
    }
    if (timer == duckFadeInTimer)
    {
        duckFadeInActive->setValue(false);
        duckFadeInCurrentTime->setValue(0.0f);
        duckActive->setValue(false);
        return;
    }

    Cue::onCueTimerFinished(timer); // pre-wait / post-wait
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
    // If the duck pre-play delay is still running, no transport was ever started, so the
    // change-listener that normally resets the playing state won't fire.
    bool wasAudioPlaying = filesManager->haveOnePlaying();

    // Duck others: abort the sequence and fade the ducked cues back in immediately.
    cancelDuckSequence(true);

    mtcSender->stop();
    filesManager->stopAll();
    for (auto& audioFile : filesManager->items)
        audioFile->player->mixer->setPluginChain(nullptr);
    slicesManager->resetSlices();
    askedToStop = true;

    if (!wasAudioPlaying)
    {
        isPlaying->setValue(false);
        if (parentCuelist != nullptr && parentCuelist->currentCue->getTargetContainerAs<Cue>() == this)
            parentCuelist->clearCurrentCue();
    }
}

bool AudioCue::canSeekTo() const
{
    return isPlaying->boolValue() && !preWaitActive->boolValue() && duration->doubleValue() > 0.0;
}

void AudioCue::seekToTime(double t)
{
    double total = duration->doubleValue();
    if (total <= 0.0) return;

    // Map the [0, duration] time onto the cue's playable window, then seek every file there.
    double frac = jlimit(0.0, 1.0, t / total);
    double winStart = slicesManager->startTime->doubleValue();
    double winEnd = slicesManager->endTime->doubleValue();
    double target = winStart + frac * (winEnd - winStart);

    slicesManager->resetSlices(); // drop stale repetition counters so the bar matches
    filesManager->setCurrentTime(target);
}

void AudioCue::retriggerStop()
{
    cancelDuckSequence(true);
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

void AudioCue::fadeAndStop(double fadeTime)
{
    // In a pre/post-wait, with no fade time, or with nothing actually sounding, a hard stop
    // is right (it also cancels the waits). Otherwise fade the transports out then stop —
    // same as retriggerStop but over the caller-supplied (group) fade time.
    if (fadeTime <= 0.0 || preWaitActive->boolValue() || postWaitActive->boolValue()
        || !filesManager->haveOnePlaying())
    {
        stop();
        return;
    }

    cancelDuckSequence(true);
    isRetriggerStopping = true;
    askedToStop = true;
    retriggerFadeStartTime = Time::getMillisecondCounterHiRes();
    duration->setValue(fadeTime);
    currentTime->setValue(0.0);
    fadeOut(fadeTime, true);
}

void AudioCue::panicInternal()
{
    bool wasAudioPlaying = filesManager->haveOnePlaying();

    cancelDuckSequence(true);

    filesManager->panicAll();
    askedToStop = true;

    if (!wasAudioPlaying)
    {
        isPlaying->setValue(false);
        if (parentCuelist != nullptr && parentCuelist->currentCue->getTargetContainerAs<Cue>() == this)
            parentCuelist->clearCurrentCue();
    }
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

    mtcSender->updateDisplay();

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
            mtcSender->stop();
            currentTime->setValue(0.0);
            isPanicking = false;
            isPreviewing = false;
            slicesManager->resetSlices();

            if (loop->boolValue() && !askedToStop)
                filesManager->resetCurrentTime();

            if (!loop->boolValue() && !askedToStop && !isPreviewing)
            {
                endCue();

                // Duck others: the audio finished on its own → wait post-play, then fade the
                // other cues back in.
                if (duckOthersCC->enabled->boolValue() && duckActive->boolValue())
                    startDuckRestore();
            }

            if (askedToStop) {
                if (isRetriggerStopping) {
                    isRetriggerStopping = false;
                    duration->setValue(slicesManager->getTotalDuration());
                    endCue();
                } else if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
                    parentCuelist->clearCurrentCue();
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

void AudioCue::refreshVolume()
{
    for (auto& audioFile : filesManager->items)
        audioFile->updateGain();
}

void AudioCue::parameterValueChanged(Parameter* p)
{
    Cue::parameterValueChanged(p);

    if (p == volume)
        refreshVolume();
}

float AudioCue::getFaderVolume()
{
    return volume->floatValue();
}

void AudioCue::setFaderVolume(float v)
{
    volume->setValue(jlimit(0.0f, 1.5f, v));
}

float AudioCue::getOutputLevel()
{
    // Live output level = the master volume scaled by the current fade/panic multiplier of a
    // playing file, so the fader fill drops and recovers while ducking.
    float mult = 1.0f;
    for (auto& audioFile : filesManager->items)
    {
        if (audioFile->player != nullptr && audioFile->player->transport->isPlaying())
        {
            mult = (float)audioFile->player->getFadeMultiplier();
            break;
        }
    }
    return jlimit(0.0f, 1.5f, getFaderVolume() * mult);
}

StringArray AudioCue::getMultiEditHiddenControllableNames() const
{
    StringArray names;
    if (filesManager != nullptr) names.add(filesManager->shortName);
    if (audioSlicer != nullptr) names.add(audioSlicer->shortName);
    return names;
}

Component* AudioCue::createMultiEditExtraEditor(const Array<Cue*>& scopeCues)
{
    return new AudioMultiFilesEditor(scopeCues);
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
        // addToUndo=false: these files belong to the cue being created, not separate undo steps.
        AudioFile* audioFile = filesManager->addItemFromData(var(), false);
        audioFile->audioFile->setValue(f);
    }
}

// -------------------------------------------------------
// MTC
// -------------------------------------------------------

void AudioCue::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    Cue::onControllableFeedbackUpdateInternal(cc, c);

    if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile)
    {
        if (duckOthersCC == cc && c == duckOthersCC->enabled)
            duckOthersCC->editorIsCollapsed = !duckOthersCC->enabled->boolValue();
        return;
    }

    // Allow turning MTC on/off (or choosing its interface) while the cue is already
    // playing: start it at the cue's current position, or stop it.
    mtcSender->handleControllableUpdate(c, isPlaying->boolValue() && !isPreviewing);
}
