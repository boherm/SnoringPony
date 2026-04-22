/*
  ==============================================================================

    AHCQMixerSettings.h
    Created: 22 Apr 2026
    Author:  boherm

    Allen & Heath CQ series mixer settings.
    Communicates via raw MIDI bytes over a TCP socket (port 51325).

    Partial support: only channel mute/unmute is available via the CQ
    MIDI protocol.  DCA management (names, faders, membership) and FX
    send routing are not exposed by the protocol.

  ==============================================================================
*/

#pragma once

#include "../MixerSettings.h"
#include "AHCQProtocol.h"
#include <mutex>

class AHCQMixerSettings : public MixerSettings
{
public:
    AHCQMixerSettings();
    ~AHCQMixerSettings() override;

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
    // Send raw bytes over the TCP socket. Thread-safe via socketMutex.
    void sendMidi(const std::vector<uint8_t>& data);

    std::unique_ptr<juce::StreamingSocket> socket;
    std::mutex socketMutex;
    bool connected = false;
    std::atomic<bool> connecting { false };

    // Background connection thread
    class ConnectThread : public juce::Thread
    {
    public:
        ConnectThread(AHCQMixerSettings& o) : juce::Thread("AHCQConnect"), owner(o) {}
        void run() override;
    private:
        AHCQMixerSettings& owner;
    };
    std::unique_ptr<ConnectThread> connectThread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AHCQMixerSettings)
};
