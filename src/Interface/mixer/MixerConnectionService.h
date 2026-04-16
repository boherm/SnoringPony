/*
  ==============================================================================

    MixerConnectionService.h
    Created: 14 Apr 2026
    Author:  boherm

    Handles OSC connectivity to a mixing console (Behringer Wing for now).
    Vendor-aware dispatch for address formats.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <map>
#include <set>

class MixerConnectionService
{
public:
    MixerConnectionService();
    ~MixerConnectionService();

    void setVendor(const juce::String& vendor) { this->vendor = vendor; }

    // Optional callback, fired just before each message is sent on the wire.
    std::function<void(const juce::OSCMessage&)> logSent;

    bool connect(const juce::String& host, int sendPort, int recvPort);
    void disconnect();
    bool isConnected() const { return connected; }

    void sendChannelName(int channelNum, const juce::String& name);
    void sendChannelIcon(int channelNum, int icon);
    void sendDCAName(int dcaNum, const juce::String& name);

    // membership[i]      = channel numbers assigned to DCA i+1
    // dcaNames[i]        = display name to push to /dca/<i+1>/name
    // definedChannels    = list of channel numbers to push tags for (those configured in the mixer)
    // activeChannelNames = channel number → character name for channels used in any DCA.
    //                      Unused declared channels are muted; used ones are unmuted, coloured
    //                      red, and renamed to the active character.
    void applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                            const juce::StringArray& dcaNames,
                            const juce::Array<int>& definedChannels,
                            const std::map<int, juce::String>& activeChannelNames,
                            const std::map<int, std::set<int>>& channelFXBuses,
                            const juce::Array<int>& definedBuses,
                            const juce::Array<bool>& dcaHasFX,
                            const std::map<int, float>& dcaForcedFaders);

private:
    void send(const juce::OSCMessage& m);

    juce::OSCSender sender;
    juce::String vendor = "Wing";
    bool connected = false;
};
