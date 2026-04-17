/*
  ==============================================================================

    WingMixerSettings.h
    Created: 17 Apr 2026
    Author:  boherm

    Behringer Wing specific mixer settings and OSC protocol implementation.

  ==============================================================================
*/

#pragma once

#include "../MixerSettings.h"
#include "WingProtocol.h"

class WingMixerSettings : public MixerSettings
{
public:
    WingMixerSettings();
    ~WingMixerSettings() override;

    // Wing-specific UI parameters
    StringParameter* remoteHost;
    IntParameter* remotePort;
    IntParameter* numDCAs;
    BoolParameter* connectedParam;

    // --- MixerSettings interface ---
    int getNumDCAs() const override { return numDCAs->intValue(); }

    void connect() override;
    void disconnect() override;
    bool isConnected() const override;
    bool shouldReconnectOnChange(Controllable* c) const override;

    void sendChannelName(int channelNum, const juce::String& name) override;
    void sendChannelIcon(int channelNum, int icon) override;

    void applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                            const juce::StringArray& dcaNames,
                            const juce::Array<int>& definedChannels,
                            const std::map<int, juce::String>& activeChannelNames,
                            const std::map<int, std::set<int>>& channelFXBuses,
                            const juce::Array<int>& definedBuses,
                            const juce::Array<bool>& dcaHasFX,
                            const std::map<int, float>& dcaForcedFaders) override;

private:
    void send(const juce::OSCMessage& m);

    juce::OSCSender sender;
    bool connected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingMixerSettings)
};
