/*
  ==============================================================================

    MTCSender.h
    Created: 05 Jul 2026
    Author:  boherm

    Reusable MIDI Time Code sender, shared by any cue that can emit MTC (Audio and
    Group cues). It owns the "MTC" container and its parameters, a high-resolution
    quarter-frame timer, and a global registry that keeps at most one sender per MIDI
    interface. The only per-cue difference is the playback time source, injected as a
    `timeProvider` (e.g. AudioCue's transport time, or GroupCue's wall-clock currentTime).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "MTCEncoder.h"

#include <functional>

class MIDIInterface;
class Cue;

class MTCSender : public juce::HighResolutionTimer
{
public:
    // owner: the cue that owns this sender (used only for its niceName in the MTC monitor).
    // timeProvider: returns the current playback position in seconds; the Offset parameter
    // is added on top internally.
    MTCSender(Cue* owner, std::function<double()> timeProvider);
    ~MTCSender() override;

    // Builds the "MTC" container and its parameters and returns it. The owner must add it to
    // its own child containers, typically with addChildControllableContainer(cc, true), which
    // also gives automatic serialization.
    EnablingControllableContainer* createContainer();

    EnablingControllableContainer* cc = nullptr;
    TargetParameter* midiInterface = nullptr;
    FloatParameter* offset = nullptr;
    EnumParameter* frameRate = nullptr;
    StringParameter* timecodeDisplay = nullptr;

    Cue* owner;
    std::function<double()> timeProvider;

    MIDIInterface* getInterface() const;
    bool isEnabled() const;
    bool isRunning() const { return isTimerRunning(); }

    void start();
    void stop();

    // Refresh the read-only timecode display from the current time. Call from the owner's
    // playback timer; it no-ops unless the sender is actually running.
    void updateDisplay();

    // React to the "Enabled" toggle or the MIDI interface being changed while the cue plays.
    // `playing` = the owner is currently playing (and not previewing).
    void handleControllableUpdate(Controllable* c, bool playing);

    void hiResTimerCallback() override;

    // The interface this sender is currently sending on (may differ from the selected one
    // right after an interface change); used to clean up the right registry entry on stop.
    MIDIInterface* activeInterface = nullptr;

    // One sender per interface: starting on an interface evicts any previous one.
    static juce::HashMap<MIDIInterface*, MTCSender*> activeSenders;

private:
    int currentPiece = 0;
    double lastFrameTime = 0.0;
};
