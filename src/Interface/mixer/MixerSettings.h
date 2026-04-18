/*
  ==============================================================================

    MixerSettings.h
    Created: 17 Apr 2026
    Author:  boherm

    Base class for mixer-specific settings. Each mixing console type
    (Wing, X32, etc.) subclasses this to expose its own parameters
    and protocol logic. The transport layer (OSC, MIDI, TCP, etc.)
    is entirely owned by the subclass.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include <map>
#include <set>

class MixerSettings : public ControllableContainer
{
public:
    MixerSettings(const juce::String& name) : ControllableContainer(name) {}
    virtual ~MixerSettings() {}

    // Logging callback — set by MixerInterface so subclasses can report
    // outgoing data in a transport-agnostic way.
    std::function<void(const juce::String&)> logOutgoing;

    // Called (on the message thread) after an async connection attempt completes.
    std::function<void(bool success)> onConnectionResult;

    // --- Configuration ---
    virtual int getNumDCAs() const = 0;

    // --- Connection lifecycle (transport-agnostic) ---
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Returns true if changing the given controllable should trigger a reconnect.
    virtual bool shouldReconnectOnChange(Controllable* c) const = 0;

    // --- Channel operations ---
    virtual void sendChannelName(int channelNum, const juce::String& name) = 0;
    virtual void sendChannelIcon(int channelNum, int icon) = 0;

    // --- DCA membership & state ---
    // membership[i]      = channel numbers assigned to DCA i+1
    // dcaNames[i]        = display name for DCA i+1
    // definedChannels    = all channel numbers declared in the mixer
    // activeChannelNames = channel number → active character name
    // channelFXBuses     = channel number → set of active FX bus numbers
    // definedBuses       = all FX bus numbers declared in the mixer
    // dcaHasFX[i]        = true if DCA i+1 has at least one character with FX
    // dcaForcedFaders    = DCA index → forced fader level (dB)
    virtual void applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                                    const juce::StringArray& dcaNames,
                                    const juce::Array<int>& definedChannels,
                                    const std::map<int, juce::String>& activeChannelNames,
                                    const std::map<int, std::set<int>>& channelFXBuses,
                                    const juce::Array<int>& definedBuses,
                                    const juce::Array<bool>& dcaHasFX,
                                    const std::map<int, float>& dcaForcedFaders) = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerSettings)
};
