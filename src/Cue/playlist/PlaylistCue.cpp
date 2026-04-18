/*
  ==============================================================================

    PlaylistCue.cpp
    Created: 3 Dec 2025 06:30:22pm
    Author:  boherm

  ==============================================================================
*/

#include "PlaylistCue.h"
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
    duration->isSavable = false;
    duration->setEnabled(false);

    volume = addFloatParameter("Volume", "Volume for this playlist", params.getProperty("volume", 1.0), 0.0, 1.5, 0.01);

    outputTarget = addTargetParameter("Output Target", "Output target for this playlist cue", InterfaceManager::getInstance());
    outputTarget->targetType = TargetParameter::CONTAINER;
    outputTarget->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;

    loop = addBoolParameter("Loop", "If enabled, the playlist will loop when it reaches the end.", false);

    shuffle = addBoolParameter("Shuffle", "If enabled, the playlist will play items in random order.", false);

    filesManager.reset(new BaseManager<PlaylistFile>("Playlist Files"));
    filesManager->addAsyncContainerListener(this);

    if (filesManager->items.isEmpty()) {
        filesManager->addItemFromData(var());
    }
    addChildControllableContainer(filesManager.get());

    pluginChainManager = new PluginChainManager();
    pluginChainManager->editorIsCollapsed = true;
    pluginChainManager->addAsyncContainerListener(this);
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
}

void PlaylistCue::playInternal()
{
    askedToStop = false;

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
    generatePlaylistOrder();
    PlaylistFile* nextFile = getNextFileToPlay();
    if (nextFile == nullptr)
        return;
    refreshAudioOutput();
    nextFile->player->transport->addChangeListener(this);
    nextFile->player->play();
}

void PlaylistCue::stopInternal()
{
    askedToStop = true;
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
        for (PlaylistFile* playlistFile : filesManager->items) {
            if (playlistFile->isEnabled()) {
                dur += playlistFile->duration->doubleValue();
            }
        }
        duration->setValue(dur);
    }
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

PlaylistFile* PlaylistCue::getNextFileToPlay()
{
    if (playlistOrder.size() == 0)
        return nullptr;

    if (currentOrderIndex + 1 < playlistOrder.size()) {
        currentOrderIndex++;
    } else {
        if (!loop->boolValue())
            return nullptr;
        currentOrderIndex = 0;

        // reset all positions
        for (auto& playlistFile : filesManager->items)
        {
            playlistFile->player->transport->setPosition(0.0f);
        }
    }

    int fileIndex = playlistOrder[currentOrderIndex];
    return filesManager->items[fileIndex];

}

void PlaylistCue::parameterValueChanged(Parameter* p)
{
    Cue::parameterValueChanged(p);

    if (p == outputTarget)
    {
        refreshAudioOutput();
    }

    if (p == volume)
    {
        refreshVolume();
    }
}

void PlaylistCue::parameterControlModeChanged(Parameter* p)
{
    if (p == volume && volume->controlMode == Parameter::ControlMode::REFERENCE)
    {
        volume->referenceTarget->setRootContainer(ProjectSettings::getInstance()->getControllableContainerByName("volumePresets"));
    }
}

void PlaylistCue::newMessage(const ContainerAsyncEvent& e)
{
    if (e.source == filesManager.get() && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate && (e.targetControllable->niceName == "Duration" || e.targetControllable->niceName == "Enabled"))
        refreshGlobalDuration();

    if (e.source == pluginChainManager)
    {
        if (pluginChainManager->hasPluginWarnings())
            setWarningMessage(pluginChainManager->getPluginWarningMessage());
        else
            clearWarning();
    }
}

void PlaylistCue::timerCallback()
{
    if (isRetriggerStopping)
    {
        double elapsed = (Time::getMillisecondCounterHiRes() - retriggerFadeStartTime) / 1000.0;
        currentTime->setValue(jmin(elapsed, duration->doubleValue()));
        return;
    }

    double maxCurrentTime = 0.0;
    for (auto& audioFile : filesManager->items)
    {
        if (audioFile->player->transport != nullptr)
        {
            maxCurrentTime += audioFile->player->transport->getCurrentPosition();
        }
    }
    currentTime->setValue(maxCurrentTime);
}

void PlaylistCue::changeListenerCallback(ChangeBroadcaster* source)
{
    if (dynamic_cast<AudioTransportSource*>(source) != nullptr)
    {
        AudioTransportSource* transport = dynamic_cast<AudioTransportSource*>(source);
        if (transport->isPlaying()) {
            isPlaying->setValue(true);
            startTimerHz(10);
        }

        if (!transport->isPlaying()) {
            transport->removeChangeListener(this);

            if (askedToStop) {
                stopTimer();
                isPlaying->setValue(false);
                currentTime->setValue(0.0);
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
                    isRetriggerStopping = false;
                    refreshGlobalDuration();
                    endCue();
                } else if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
                    parentCuelist->currentCue->resetValue();
                }

                return;
            }

            PlaylistFile* nextFile = getNextFileToPlay();
            if (nextFile == nullptr) {
                stopTimer();
                isPlaying->setValue(false);
                currentTime->setValue(0.0);
                refreshGlobalDuration();
                endCue();
                return;
            }
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
        PlaylistFile* audioFile = filesManager->addItemFromData(var());
        audioFile->audioFile->setValue(f);
    }
}
