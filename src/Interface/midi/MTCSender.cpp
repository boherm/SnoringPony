/*
  ==============================================================================

    MTCSender.cpp
    Created: 05 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "MTCSender.h"
#include "MIDIInterface.h"
#include "../InterfaceManager.h"
#include "../../Cue/audio/ui/MTCContainerEditor.h"

juce::HashMap<MIDIInterface*, MTCSender*> MTCSender::activeSenders;

MTCSender::MTCSender(Cue* owner, std::function<double()> timeProvider) :
    owner(owner),
    timeProvider(std::move(timeProvider))
{
}

MTCSender::~MTCSender()
{
    stop();
}

EnablingControllableContainer* MTCSender::createContainer()
{
    cc = new EnablingControllableContainer("MTC");
    cc->enabled->setValue(false);
    cc->editorIsCollapsed = true;

    midiInterface = cc->addTargetParameter("MIDI Interface", "MIDI interface to send MTC through", InterfaceManager::getInstance());
    midiInterface->targetType = TargetParameter::CONTAINER;
    midiInterface->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetMIDIInterface;

    offset = cc->addFloatParameter("Offset", "Time offset added to MTC timecode", 0.0);
    offset->defaultUI = FloatParameter::TIME;

    frameRate = cc->addEnumParameter("Frame Rate", "MTC frame rate");
    frameRate->addOption("24 fps", MTCEncoder::FPS_24);
    frameRate->addOption("25 fps", MTCEncoder::FPS_25);
    frameRate->addOption("29.97 df", MTCEncoder::FPS_30_DROP);
    frameRate->addOption("30 fps", MTCEncoder::FPS_30);

    timecodeDisplay = cc->addStringParameter("Timecode", "Current MTC timecode being sent", "00:00:00:00");
    timecodeDisplay->setEnabled(false);
    timecodeDisplay->hideInEditor = true;

    cc->customGetEditorFunc = &MTCContainerEditor::create;

    return cc;
}

MIDIInterface* MTCSender::getInterface() const
{
    return midiInterface->getTargetContainerAs<MIDIInterface>();
}

bool MTCSender::isEnabled() const
{
    return cc != nullptr && cc->enabled->boolValue();
}

void MTCSender::start()
{
    if (!isEnabled()) return;

    MIDIInterface* iface = getInterface();
    if (iface == nullptr) return;

    // Stop any other sender currently sending MTC on this interface (one sender per interface).
    if (activeSenders.contains(iface))
    {
        MTCSender* previous = activeSenders[iface];
        if (previous != this)
            previous->stop();
    }

    activeSenders.set(iface, this);
    activeInterface = iface;

    MTCEncoder::FrameRate rate = (MTCEncoder::FrameRate)(int)frameRate->getValueData();
    // Start from the cue's current playback position (not just the offset), so enabling MTC
    // mid-playback sends the correct timecode for where the cue actually is.
    double startTime = timeProvider() + offset->doubleValue();

    iface->sendMessage(MTCEncoder::createFullFrameMessage(startTime, rate));

    currentPiece = 0;
    lastFrameTime = 0.0;
    int intervalMs = juce::jmax(1, (int)(MTCEncoder::getQuarterFrameInterval(rate) * 1000.0));
    startTimer(intervalMs);
}

void MTCSender::stop()
{
    if (isTimerRunning())
        stopTimer();

    if (timecodeDisplay != nullptr)
        timecodeDisplay->setValue("00:00:00:00");

    // Remove from active senders, using the interface we actually started on (the selected
    // one may have just changed, which is exactly why we track it).
    if (activeInterface != nullptr
        && activeSenders.contains(activeInterface)
        && activeSenders[activeInterface] == this)
        activeSenders.remove(activeInterface);

    activeInterface = nullptr;
}

void MTCSender::updateDisplay()
{
    if (!isEnabled() || !isTimerRunning())
        return;

    double playbackTime = timeProvider() + offset->doubleValue();
    MTCEncoder::FrameRate rate = (MTCEncoder::FrameRate)(int)frameRate->getValueData();
    timecodeDisplay->setValue(MTCEncoder::secondsToSMPTEString(playbackTime, rate));
}

void MTCSender::handleControllableUpdate(Controllable* c, bool playing)
{
    // Allow turning MTC on/off (or choosing its interface) while the cue is already playing:
    // start it at the cue's current position, or stop it.
    if (c == cc->enabled)
    {
        if (cc->enabled->boolValue())
        {
            if (playing)
                start();
        }
        else
        {
            stop();
        }
    }
    else if (c == midiInterface)
    {
        if (playing)
        {
            // Release the previous interface, then re-arm on the newly chosen one (no-op if
            // the interface was cleared).
            stop();
            if (cc->enabled->boolValue())
                start();
        }
    }
}

void MTCSender::hiResTimerCallback()
{
    MIDIInterface* iface = getInterface();
    if (iface == nullptr) return;

    double playbackTime = timeProvider() + offset->doubleValue();
    MTCEncoder::FrameRate rate = (MTCEncoder::FrameRate)(int)frameRate->getValueData();

    if (currentPiece == 0)
        lastFrameTime = playbackTime;

    iface->sendMessage(MTCEncoder::createQuarterFrameMessage(currentPiece, lastFrameTime, rate));

    currentPiece = (currentPiece + 1) % 8;
}
