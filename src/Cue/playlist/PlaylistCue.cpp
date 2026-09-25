/*
  ==============================================================================

    PlaylistCue.cpp
    Created: 3 Dec 2025 06:30:22pm
    Author:  boherm

  ==============================================================================
*/

#include "PlaylistCue.h"
#include "../audio/AudioCue.h"
#include "../../util/SPUnicode.h"
#include "../../Audio/PluginSlot.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Interface/InterfaceManager.h"
#include "../../Interface/audio/AudioOutput.h"

PlaylistFile::PlaylistFile(String name) :
    BaseItem(name)
{
    setHasCustomColor(true);
    player = std::make_unique<AudioPlayer>();
    player->transport->addChangeListener(this);
    isSelectable = false;

    // use for isPlaying status in item header for now
    itemColor->setDefaultValue(BG_COLOR);
    itemColor->isSavable = false;
    itemColor->setEnabled(false);

    audioFile = addFileParameter("File Path", "Path to the audio file", "", true);

    duration = addFloatParameter("Duration", "Duration of the audio file in seconds", 0.0);
    duration->minimumValue = 0.0f;
    duration->defaultUI = FloatParameter::TIME;
    duration->hideInRemoteControl = true;
    duration->isSavable = false;
    duration->setEnabled(false);

    isPlaying = addBoolParameter("Is Playing", "Is this audio file currently playing", false);
    isPlaying->isSavable = false;
    isPlaying->setEnabled(false);
    isPlaying->hideInEditor = true;
}

PlaylistFile::~PlaylistFile()
{
    player->stopAndClean();
    player->transport->removeChangeListener(this);
}

void PlaylistFile::parameterValueChanged(Parameter* p)
{
    BaseItem::parameterValueChanged(p);

    if (Engine::mainEngine != nullptr && Engine::mainEngine->isClearing)
        return;

    clearWarning();

    if (p == audioFile)
    {
        // macOS filenames are NFD (decomposed accents); normalize the stored path to NFC so
        // it displays correctly. File access is normalization-insensitive, so this is safe.
        const String rawPath = audioFile->stringValue();
        const String nfcPath = SPUnicode::toNFC(rawPath);
        if (nfcPath != rawPath) { audioFile->setValue(nfcPath); return; }

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

    if (p == enabled)
    {
        itemColor->setColor(enabled->boolValue() ? BG_COLOR : BG_COLOR.brighter(0.2f));
    }
}

void PlaylistFile::changeListenerCallback(ChangeBroadcaster* source)
{
    AudioTransportSource* transport = dynamic_cast<AudioTransportSource*>(source);
    if (transport != nullptr) {
        isPlaying->setValue(transport->isPlaying());
        itemColor->setColor(transport->isPlaying() ? Colours::green : BG_COLOR);
    }
}

// -----------------------------------------------------

PlaylistCue::PlaylistCue(var params) :
    Cue(params)
{
	itemDataType = "Playlist Cue";
    isFadable = true;

    retriggerStopCC->hideInEditor = false;

    duration->isSavable = false;
    duration->setEnabled(false);

    volume = addFloatParameter("Volume", "Volume for this playlist", params.getProperty("volume", 1.0), 0.0, 1.5, 0.01);

    outputTarget = addTargetParameter("Output Target", "Output target for this playlist cue", InterfaceManager::getInstance());
    outputTarget->targetType = TargetParameter::CONTAINER;
    outputTarget->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;

    loop = addBoolParameter("Loop", "If enabled, the playlist will loop when it reaches the end.", false);

    shuffle = addBoolParameter("Shuffle", "If enabled, the playlist will play items in random order.", false);

    crossfadeCC = new EnablingControllableContainer("Crossfade");
    crossfadeCC->enabled->setValue(false);
    crossfadeCC->editorIsCollapsed = true;
    addChildControllableContainer(crossfadeCC, true);

    crossfadeDuration = crossfadeCC->addFloatParameter("Duration", "Overlap duration between two consecutive tracks", 3.0, 0.0);
    crossfadeDuration->defaultUI = FloatParameter::TIME;

    pluginChainManager = new PluginChainManager();
    pluginChainManager->editorIsCollapsed = true;
    pluginChainManager->addAsyncContainerListener(this);

    filesManager.reset(new BaseManager<PlaylistFile>("Playlist Files"));
    filesManager->addAsyncContainerListener(this);

    if (filesManager->items.isEmpty()) {
        // addToUndo=false: the default file belongs to the cue, not a separate undo step
        // (a nested undo action corrupts multi-item undo/redo — see AudioCue).
        filesManager->addItemFromData(var(), false);
    }
    addChildControllableContainer(filesManager.get());
    addChildControllableContainer(pluginChainManager);

    showWarningInUI = true;
}

PlaylistCue::~PlaylistCue()
{
    stop();
    for (auto& f : filesManager->items)
        f->player->mixer->setPluginChain(nullptr);
    pluginChainManager->removeAsyncContainerListener(this);
    filesManager->removeAsyncContainerListener(this);
    delete pluginChainManager;
}

void PlaylistCue::loadJSONDataItemInternal(juce::var data)
{
    Cue::loadJSONDataItemInternal(data);
    refreshGlobalDuration();
    refreshAudioOutput();
    refreshVolume();
    updateWarnings();
}

void PlaylistCue::playInternal()
{
    askedToStop = false;
    isRetriggerStopping = false; // a fresh GO clears any leftover retrigger-stop state

    // reset playlistfile
    for (auto& playlistFile : filesManager->items)
    {
        playlistFile->canBeReorderedInEditor = false;
        playlistFile->userCanRemove = false;
        playlistFile->enabled->setEnabled(false);
        playlistFile->audioFile->setEnabled(false);
        playlistFile->player->transport->setPosition(0.0f);
        playlistFile->player->mixer->resetFade();
    }
    queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));

    for (auto& playlistFile : filesManager->items)
        playlistFile->player->mixer->setPluginChain(pluginChainManager);

    currentOrderIndex = -1;
    currentFile = nullptr;
    elapsedBeforeCurrentFile = 0.0;
    crossfadeStarted = false;
    generatePlaylistOrder();
    PlaylistFile* nextFile = getNextFileToPlay();
    if (nextFile == nullptr)
        return;
    currentFile = nextFile;
    refreshAudioOutput();
    nextFile->player->transport->addChangeListener(this);
    nextFile->player->play();

    // If another cue is currently ducking the others, start already ducked to that volume so this
    // playlist doesn't blast at full over the ducked mix (restored when the ducker ends).
    double duckGain = AudioCue::getActiveDuckGain(this);
    if (duckGain < 1.0)
        fade(duckGain, 0.0);
}

void PlaylistCue::stopInternal()
{
    askedToStop = true;
    currentFile = nullptr;
    crossfadeStarted = false;
    stopTimer();
    for (auto& playlistFile : filesManager->items)
    {
        playlistFile->player->stopAndClean();
        playlistFile->player->mixer->setPluginChain(nullptr);
    }
}

void PlaylistCue::panicInternal()
{
    askedToStop = true;
    for (auto& playlistFile : filesManager->items)
    {
        playlistFile->player->panic();
    }
}

void PlaylistCue::refreshGlobalDuration()
{
    if (!isPlaying->boolValue()) {
        double dur = 0.0;
        int enabledCount = 0;
        for (PlaylistFile* playlistFile : filesManager->items) {
            if (playlistFile->isEnabled()) {
                dur += playlistFile->duration->doubleValue();
                enabledCount++;
            }
        }

        // Each crossfade makes two tracks overlap, so the playlist is that much shorter.
        const double crossfadeTime = getCrossfadeTime();
        if (crossfadeTime > 0.0 && enabledCount > 1)
            dur = jmax(0.0, dur - crossfadeTime * (enabledCount - 1));

        duration->setValue(dur);
    }
}

double PlaylistCue::getCrossfadeTime() const
{
    if (crossfadeCC == nullptr || !crossfadeCC->enabled->boolValue())
        return 0.0;

    return jmax(0.0, crossfadeDuration->doubleValue());
}

void PlaylistCue::refreshAudioOutput()
{
    AudioOutput* audioOutput = outputTarget->getTargetContainerAs<AudioOutput>();

    for (PlaylistFile* playlistFile : filesManager->items) {
        playlistFile->player->setOutput(audioOutput);
    }
}

void PlaylistCue::refreshVolume()
{
    for (PlaylistFile* playlistFile : filesManager->items) {
        playlistFile->player->transport->setGain(volume->floatValue());
    }
}

void PlaylistCue::generatePlaylistOrder()
{
    playlistOrder.clear();

    for (int i = 0; i < filesManager->items.size(); ++i)
    {
        if (filesManager->items[i]->isEnabled())
            playlistOrder.add(i);
    }

    if (shuffle->boolValue())
    {
        Random r;
        for (int i = 0; i < playlistOrder.size(); ++i)
        {
            int swapIndex = r.nextInt(playlistOrder.size());
            int temp = playlistOrder[i];
            playlistOrder.set(i, playlistOrder[swapIndex]);
            playlistOrder.set(swapIndex, temp);
        }
    }
}

PlaylistFile* PlaylistCue::peekNextFileToPlay(int* nextOrderIndex) const
{
    if (playlistOrder.size() == 0)
        return nullptr;

    int nextIndex;
    if (currentOrderIndex + 1 < playlistOrder.size()) {
        nextIndex = currentOrderIndex + 1;
    } else {
        if (!loop->boolValue())
            return nullptr;
        nextIndex = 0;
    }

    if (nextOrderIndex != nullptr)
        *nextOrderIndex = nextIndex;

    return filesManager->items[playlistOrder[nextIndex]];
}

PlaylistFile* PlaylistCue::getNextFileToPlay(int* nextOrderIndex)
{
    int nextIndex = 0;
    PlaylistFile* nextFile = peekNextFileToPlay(&nextIndex);
    if (nextFile == nullptr)
        return nullptr;

    const bool wrapped = nextIndex == 0 && currentOrderIndex >= 0;
    currentOrderIndex = nextIndex;

    if (wrapped)
    {
        // Looping back to the top: rewind every track, except one still sounding (the
        // outgoing side of a crossfade over the wrap).
        for (auto& playlistFile : filesManager->items)
        {
            if (!playlistFile->player->transport->isPlaying())
                playlistFile->player->transport->setPosition(0.0f);
        }
    }

    if (nextOrderIndex != nullptr)
        *nextOrderIndex = nextIndex;

    return nextFile;
}

void PlaylistCue::startCrossfadeToNext()
{
    crossfadeStarted = true; // one attempt per track, whatever the outcome

    PlaylistFile* outgoing = currentFile;
    PlaylistFile* next = peekNextFileToPlay();

    // End of the playlist, or a single track looping onto itself (one transport per track,
    // so it can't overlap with itself): let the natural end drive the advance instead.
    if (outgoing == nullptr || next == nullptr || next == outgoing)
        return;

    // Bound the overlap by both tracks so a crossfade longer than a track doesn't fire at t=0.
    const double crossfadeTime = jmin(getCrossfadeTime(),
                                      outgoing->duration->doubleValue(),
                                      next->duration->doubleValue());
    if (crossfadeTime <= 0.0)
        return;

    int nextOrderIndex = 0;
    getNextFileToPlay(&nextOrderIndex);

    elapsedBeforeCurrentFile = nextOrderIndex == 0
        ? 0.0
        : elapsedBeforeCurrentFile + jmax(0.0, outgoing->duration->doubleValue() - crossfadeTime);
    currentFile = next;
    crossfadeStarted = false; // armed again for the incoming track

    // Outgoing: stop driving the cue, then fade out and stop by itself.
    outgoing->player->transport->removeChangeListener(this);
    outgoing->player->fadeOut(crossfadeTime, true);

    // Incoming: start silent and ramp up to the current global gain (ducking-aware).
    // AudioPlayer::fadeIn starts the transport itself.
    next->player->transport->addChangeListener(this);
    next->player->fadeIn(crossfadeTime, (float)jmin(1.0, AudioCue::getActiveDuckGain(this)));
}

void PlaylistCue::parameterValueChanged(Parameter* p)
{
    Cue::parameterValueChanged(p);

    if (Engine::mainEngine != nullptr && Engine::mainEngine->isClearing)
        return;

    if (p == outputTarget)
    {
        refreshAudioOutput();
        updateWarnings();
    }

    if (p == volume)
    {
        refreshVolume();
    }
}

void PlaylistCue::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    Cue::onControllableFeedbackUpdateInternal(cc, c);

    if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile)
    {
        if (crossfadeCC == cc && c == crossfadeCC->enabled)
            crossfadeCC->editorIsCollapsed = !crossfadeCC->enabled->boolValue();
        return;
    }

    // Both the toggle and the duration change how much the tracks overlap.
    if (crossfadeCC == cc)
        refreshGlobalDuration();
}

void PlaylistCue::parameterControlModeChanged(Parameter* p)
{
    if (p == volume && volume->controlMode == Parameter::ControlMode::REFERENCE)
    {
        volume->referenceTarget->setRootContainer(ProjectSettings::getInstance()->getControllableContainerByName("volumePresets"));
    }
}

void PlaylistCue::updateWarnings()
{
    StringArray warnings;

    AudioOutput* out = outputTarget->getTargetContainerAs<AudioOutput>();
    if (out == nullptr)
        warnings.add("No audio output selected");

    bool hasFiles = false;
    for (auto& pf : filesManager->items)
    {
        File f = pf->audioFile->getFile();
        if (f == File() || !f.existsAsFile())
            warnings.add("Missing audio file: " + pf->niceName);
        else
            hasFiles = true;
    }
    if (!hasFiles)
        warnings.add("No audio files in playlist");

    if (pluginChainManager->hasPluginWarnings())
        warnings.add(pluginChainManager->getPluginWarningMessage());

    if (warnings.isEmpty())
        clearWarning();
    else
        setWarningMessage(warnings.joinIntoString("\n"));
}

bool PlaylistCue::canBePlayed()
{
    return Cue::canBePlayed() && getWarningMessage().isEmpty();
}

void PlaylistCue::newMessage(const ContainerAsyncEvent& e)
{
    if (e.source == filesManager.get() && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate && (e.targetControllable->niceName == "Duration" || e.targetControllable->niceName == "Enabled"))
        refreshGlobalDuration();

    if (e.source == filesManager.get() || e.source == pluginChainManager)
        updateWarnings();
}

void PlaylistCue::timerCallback()
{
    if (isRetriggerStopping)
    {
        double elapsed = (Time::getMillisecondCounterHiRes() - retriggerFadeStartTime) / 1000.0;
        currentTime->setValue(jmin(elapsed, duration->doubleValue()));
        return;
    }

    if (currentFile == nullptr || currentFile->player->transport == nullptr)
        return;

    const double position = currentFile->player->transport->getCurrentPosition();
    currentTime->setValue(elapsedBeforeCurrentFile + position);

    // Start the next track while this one still has a crossfade worth of audio left.
    const double crossfadeTime = getCrossfadeTime();
    if (crossfadeTime > 0.0 && !crossfadeStarted && !askedToStop
        && currentFile->player->transport->isPlaying()
        && currentFile->duration->doubleValue() - position <= crossfadeTime)
    {
        startCrossfadeToNext();
    }
}

void PlaylistCue::changeListenerCallback(ChangeBroadcaster* source)
{
    if (dynamic_cast<AudioTransportSource*>(source) != nullptr)
    {
        AudioTransportSource* transport = dynamic_cast<AudioTransportSource*>(source);
        if (transport->isPlaying()) {
            isPlaying->setValue(true);
            startTimerHz(20);
        }

        if (!transport->isPlaying()) {
            // The outgoing side of a crossfade: it faded out on its own and drives nothing.
            if (currentFile != nullptr && transport != currentFile->player->transport)
                return;

            transport->removeChangeListener(this);

            if (askedToStop) {
                stopTimer();
                isPlaying->setValue(false);
                currentTime->setValue(0.0);
                currentFile = nullptr;
                refreshGlobalDuration();
                isPanicking = false;

                for (auto& playlistFile : filesManager->items)
                {
                    playlistFile->canBeReorderedInEditor = true;
                    playlistFile->userCanRemove = true;
                    playlistFile->audioFile->setEnabled(true);
                    playlistFile->enabled->setEnabled(true);
                }
                queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));

                if (isRetriggerStopping) {
                    refreshGlobalDuration();
                    endCue(); // consumes isRetriggerStopping
                } else if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
                    parentCuelist->clearCurrentCue();
                }

                return;
            }

            int nextOrderIndex = 0;
            PlaylistFile* nextFile = getNextFileToPlay(&nextOrderIndex);
            if (nextFile == nullptr) {
                stopTimer();
                isPlaying->setValue(false);
                currentTime->setValue(0.0);
                currentFile = nullptr;
                refreshGlobalDuration();
                endCue();
                return;
            }

            // Straight chaining (no crossfade, or one that couldn't run): the whole outgoing
            // track has been played.
            elapsedBeforeCurrentFile = nextOrderIndex == 0
                ? 0.0
                : elapsedBeforeCurrentFile + (currentFile != nullptr ? currentFile->duration->doubleValue() : 0.0);
            currentFile = nextFile;
            crossfadeStarted = false;

            nextFile->player->play(false);
            nextFile->player->transport->addChangeListener(this);
        }
    }
}

void PlaylistCue::retriggerStop()
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

void PlaylistCue::fadeAndStop(double fadeTime)
{
    bool anyPlaying = false;
    for (auto& f : filesManager->items)
        if (f->player->transport != nullptr && f->player->transport->isPlaying()) { anyPlaying = true; break; }

    // In a pre/post-wait, with no fade time, or with nothing sounding: hard stop (also
    // cancels the waits). Otherwise fade the current file out then stop.
    if (fadeTime <= 0.0 || preWaitActive->boolValue() || postWaitActive->boolValue() || !anyPlaying)
    {
        stop();
        return;
    }

    isRetriggerStopping = true;
    askedToStop = true;
    retriggerFadeStartTime = Time::getMillisecondCounterHiRes();
    duration->setValue(fadeTime);
    currentTime->setValue(0.0);
    fadeOut(fadeTime, true);
}

void PlaylistCue::fadeOut(double duration, bool stopAfterFade)
{
    for (auto& file : filesManager->items)
        file->player->fadeOut(duration, stopAfterFade);
}

void PlaylistCue::fade(double targetGain, double duration)
{
    for (auto& audioFile : filesManager->items)
    {
        audioFile->player->fade(targetGain, duration);
    }
}

float PlaylistCue::getFaderVolume()
{
    return volume->floatValue();
}

void PlaylistCue::setFaderVolume(float v)
{
    volume->setValue(jlimit(0.0f, 1.5f, v));
}

float PlaylistCue::getOutputLevel()
{
    // Live output level = the volume scaled by the current fade/panic multiplier of the file
    // currently playing, so the fader fill reflects ducking.
    float mult = 1.0f;
    if (currentFile != nullptr && currentFile->player->transport->isPlaying())
    {
        // During a crossfade, the incoming track (currentFile) is the one to report:
        // the outgoing one is on its way to silence.
        mult = (float)currentFile->player->getFadeMultiplier();
    }
    else
    {
        for (auto& file : filesManager->items)
        {
            if (file->player != nullptr && file->player->transport->isPlaying())
            {
                mult = (float)file->player->getFadeMultiplier();
                break;
            }
        }
    }
    return jlimit(0.0f, 1.5f, getFaderVolume() * mult);
}

StringArray PlaylistCue::getMultiEditHiddenControllableNames() const
{
    StringArray names;
    if (filesManager != nullptr) names.add(filesManager->shortName);
    return names;
}

StringArray PlaylistCue::getMultiEditUnsetProxyNames() const
{
    // Editing several cues at once, a shared Volume would show one arbitrary cue's level.
    return StringArray(volume->shortName);
}

String PlaylistCue::autoDescriptionInternal()
{
    String desc = "Playlist: ";
    desc += String(filesManager->items.size()) + " audio file(s)";
    return desc;
}

void PlaylistCue::createFromFiles(const StringArray& files)
{
    filesManager->clear();
    for (auto& f : files)
    {
        PlaylistFile* audioFile = filesManager->addItemFromData(var(), false);
        audioFile->audioFile->setValue(f);
    }
}
