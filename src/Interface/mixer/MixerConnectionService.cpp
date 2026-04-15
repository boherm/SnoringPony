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

void MixerConnectionService::sendDCAName(int dcaNum, const juce::String& name)
{
    if (!connected) return;
    if (vendor == "Wing") send(WingProtocol::dcaNameMessage(dcaNum, name));
}

void MixerConnectionService::applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                                                const juce::StringArray& dcaNames,
                                                const juce::Array<int>& definedChannels)
{
    if (!connected) return;
    if (vendor != "Wing") return;

    for (const auto& m : WingProtocol::dcaMembershipMessages(membership, definedChannels))
        send(m);

    for (int i = 0; i < dcaNames.size(); ++i)
    {
        bool hasMembers = i < membership.size() && !membership[i].isEmpty();

        send(WingProtocol::dcaNameMessage(i + 1, dcaNames[i]));
        send(WingProtocol::dcaColorMessage(i + 1, hasMembers ? 2 : 0));
        send(WingProtocol::dcaLedMessage(i + 1, hasMembers));
    }
}
