/*
  ==============================================================================

    RedundancyStateSync.h
    Created: 19 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class RedundancyManager;
class Cue;

// Playback-state replication for the Main/Backup feature. When this instance is the Main, it
// periodically broadcasts a full snapshot of every cuelist's currentCue/nextCue and active-cue set
// over the redundancy link (self-healing against UDP loss and late joins). When this instance is a
// Backup, it applies incoming snapshots — mirroring current/next and the active flags directly,
// WITHOUT ever calling play() (no audio, no interface output; interfaces are in standby). Everything
// is keyed by control address, which is stable across two instances loading the same show.
class RedundancyStateSync :
    public juce::Timer
{
public:
    explicit RedundancyStateSync(RedundancyManager* owner);
    ~RedundancyStateSync() override;

    void start(); // begin periodic broadcast (only actually sends while Main)
    void stop();

    // Called (on the message thread) when a state message arrives from a peer.
    void handleIncoming(const juce::String& payload);

    void timerCallback() override;

private:
    RedundancyManager* rm;
    int snapshotCounter = 0; // full snapshot every N progress ticks

    // Full state (cuelists, current/next, active flags + times, main cuelist) — sent at ~5 Hz.
    juce::String serializeSnapshot() const;
    // Just the active cues' progress times — sent at ~40 Hz for smooth bars.
    juce::String serializeProgress() const;

    void applyState(const juce::var& data);    // full snapshot
    void applyProgress(const juce::var& data); // times only

    static Cue* resolveCue(const juce::String& address);
};
