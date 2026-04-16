/*
  ==============================================================================

    MixerConnectionService.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MixerConnectionService.h"
#include "wing/WingProtocol.h"

MixerConnectionService::MixerConnectionService()
{
}

MixerConnectionService::~MixerConnectionService()
{
    disconnect();
}

bool MixerConnectionService::connect(const juce::String& host, int sendPort, int /*recvPort*/)
{
    disconnect();
    connected = sender.connect(host, sendPort);
    return connected;
}

void MixerConnectionService::disconnect()
{
    if (connected) sender.disconnect();
    connected = false;
}

void MixerConnectionService::send(const juce::OSCMessage& m)
{
    if (logSent) logSent(m);
    sender.send(m);
}

void MixerConnectionService::sendChannelName(int channelNum, const juce::String& name)
{
    if (!connected) return;
    if (vendor == "Wing") send(WingProtocol::channelNameMessage(channelNum, name));
}

void MixerConnectionService::sendChannelIcon(int channelNum, int icon)
{
    if (!connected) return;
    if (vendor == "Wing") send(WingProtocol::channelIconMessage(channelNum, icon));
}

void MixerConnectionService::sendDCAName(int dcaNum, const juce::String& name)
{
    if (!connected) return;
    if (vendor == "Wing") send(WingProtocol::dcaNameMessage(dcaNum, name));
}

void MixerConnectionService::applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                                                const juce::StringArray& dcaNames,
                                                const juce::Array<int>& definedChannels,
                                                const std::map<int, juce::String>& activeChannelNames,
                                                const std::map<int, std::set<int>>& channelFXBuses,
                                                const juce::Array<int>& definedBuses,
                                                const juce::Array<bool>& dcaHasFX,
                                                const std::map<int, float>& dcaForcedFaders)
{
    if (!connected) return;
    if (vendor != "Wing") return;

    for (const auto& m : WingProtocol::dcaMembershipMessages(membership, definedChannels))
        send(m);

    for (int i = 0; i < dcaNames.size(); ++i)
    {
        int channelCount = i < membership.size() ? membership[i].size() : 0;
        bool hasMembers = channelCount > 0;

        int color = WingProtocol::COLOR_OFF;
        if (channelCount == 1)      color = WingProtocol::COLOR_DCA_SINGLE;
        else if (channelCount > 1)  color = WingProtocol::COLOR_DCA_MULTI;

        send(WingProtocol::dcaNameMessage(i + 1, dcaNames[i]));
        send(WingProtocol::dcaColorMessage(i + 1, color));
        send(WingProtocol::dcaLedMessage(i + 1, hasMembers));

        auto fdrIt = dcaForcedFaders.find(i);
        if (fdrIt != dcaForcedFaders.end())
            send(WingProtocol::dcaFaderMessage(i + 1, fdrIt->second));
    }

    for (int ch : definedChannels)
    {
        int dcaChannelCount = 0;
        for (int d = 0; d < membership.size(); ++d)
        {
            if (membership[d].contains(ch)) { dcaChannelCount = membership[d].size(); break; }
        }
        bool isActive = dcaChannelCount > 0;

        int channelColor = WingProtocol::COLOR_CHANNEL_OFF;
        if (dcaChannelCount == 1)       channelColor = WingProtocol::COLOR_DCA_SINGLE;
        else if (dcaChannelCount > 1)   channelColor = WingProtocol::COLOR_DCA_MULTI;

        send(WingProtocol::channelCustomLinkMessage(ch, false));
        send(WingProtocol::channelMuteMessage(ch, !isActive));
        send(WingProtocol::channelColorMessage(ch, channelColor));
        send(WingProtocol::channelLedMessage(ch, true));

        if (isActive)
        {
            auto it = activeChannelNames.find(ch);
            if (it != activeChannelNames.end())
                send(WingProtocol::channelNameMessage(ch, it->second));
        }

        // FX sends: for each declared bus, enable if this channel has it active, disable otherwise.
        auto fxIt = channelFXBuses.find(ch);
        const std::set<int>* activeBuses = (fxIt != channelFXBuses.end()) ? &fxIt->second : nullptr;
        for (int busNum : definedBuses)
        {
            bool on = (activeBuses != nullptr) && activeBuses->count(busNum) > 0;
            send(WingProtocol::channelSendOnMessage(ch, busNum, on));
        }
    }
}
