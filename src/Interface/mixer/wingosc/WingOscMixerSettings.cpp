/*
  ==============================================================================

    WingOscMixerSettings.cpp
    Created: 17 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "WingOscMixerSettings.h"

WingOscMixerSettings::WingOscMixerSettings() : MixerSettings("Mixer Settings")
{
    remoteHost = addStringParameter("Remote Host", "IP address of the mixing console", "192.168.1.10");
    remoteHost->autoTrim = true;

    remotePort = addIntParameter("Remote Port", "OSC send port on the console",
                                 WingOscProtocol::DEFAULT_SEND_PORT, 1, 65535);

    numDCAs = addIntParameter("Num DCAs", "Number of DCAs to expose for mixing cues",
                              8, 1, WingOscProtocol::MAX_DCAS);

    connectedParam = addBoolParameter("Connected", "Connection status", false);
    connectedParam->setControllableFeedbackOnly(true);
}

WingOscMixerSettings::~WingOscMixerSettings()
{
    disconnect();
}

void WingOscMixerSettings::connect()
{
    disconnect();
    connected = sender.connect(remoteHost->stringValue(), remotePort->intValue());
    connectedParam->setValue(connected);
}

void WingOscMixerSettings::disconnect()
{
    if (connected) sender.disconnect();
    connected = false;
    connectedParam->setValue(false);
}

bool WingOscMixerSettings::isConnected() const
{
    return connected;
}

bool WingOscMixerSettings::shouldReconnectOnChange(Controllable* c) const
{
    return c == remoteHost || c == remotePort;
}

void WingOscMixerSettings::send(const juce::OSCMessage& m)
{
    if (!connected) return;

    if (logOutgoing)
    {
        juce::String s;
        for (auto& a : m) s += " " + OSCHelpers::getStringArg(a);
        logOutgoing(m.getAddressPattern().toString() + " :" + s);
    }

    sender.send(m);
}

void WingOscMixerSettings::sendChannelName(int channelNum, const juce::String& name)
{
    send(WingOscProtocol::channelNameMessage(channelNum, name));
}

void WingOscMixerSettings::sendChannelIcon(int channelNum, int icon)
{
    send(WingOscProtocol::channelIconMessage(channelNum, icon));
}

void WingOscMixerSettings::applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                                           const juce::StringArray& dcaNames,
                                           const juce::Array<int>& definedChannels,
                                           const std::map<int, juce::String>& activeChannelNames,
                                           const std::map<int, std::set<int>>& channelFXBuses,
                                           const juce::Array<int>& definedBuses,
                                           const juce::Array<bool>& dcaHasFX,
                                           const std::map<int, float>& dcaForcedFaders)
{
    if (!connected) return;

    // DCA membership tags per channel
    for (const auto& m : WingOscProtocol::dcaMembershipMessages(membership, definedChannels))
        send(m);

    // DCA names, colors, LEDs, and forced faders
    for (int i = 0; i < dcaNames.size(); ++i)
    {
        int channelCount = i < membership.size() ? membership[i].size() : 0;
        bool hasMembers = channelCount > 0;

        int color = WingOscProtocol::COLOR_OFF;
        if (channelCount == 1)      color = WingOscProtocol::COLOR_DCA_SINGLE;
        else if (channelCount > 1)  color = WingOscProtocol::COLOR_DCA_MULTI;

        send(WingOscProtocol::dcaNameMessage(i + 1, dcaNames[i]));
        send(WingOscProtocol::dcaColorMessage(i + 1, color));
        send(WingOscProtocol::dcaLedMessage(i + 1, hasMembers));

        auto fdrIt = dcaForcedFaders.find(i);
        if (fdrIt != dcaForcedFaders.end())
            send(WingOscProtocol::dcaFaderMessage(i + 1, fdrIt->second));
    }

    // Channel state: mute, color, LED, name, FX sends
    for (int ch : definedChannels)
    {
        int dcaChannelCount = 0;
        for (int d = 0; d < membership.size(); ++d)
        {
            if (membership[d].contains(ch)) { dcaChannelCount = membership[d].size(); break; }
        }
        bool isActive = dcaChannelCount > 0;

        int channelColor = WingOscProtocol::COLOR_CHANNEL_OFF;
        if (dcaChannelCount == 1)       channelColor = WingOscProtocol::COLOR_DCA_SINGLE;
        else if (dcaChannelCount > 1)   channelColor = WingOscProtocol::COLOR_DCA_MULTI;

        send(WingOscProtocol::channelCustomLinkMessage(ch, false));
        send(WingOscProtocol::channelMuteMessage(ch, !isActive));
        send(WingOscProtocol::channelColorMessage(ch, channelColor));
        send(WingOscProtocol::channelLedMessage(ch, true));

        if (isActive)
        {
            auto it = activeChannelNames.find(ch);
            if (it != activeChannelNames.end())
                send(WingOscProtocol::channelNameMessage(ch, it->second));
        }

        auto fxIt = channelFXBuses.find(ch);
        const std::set<int>* activeBuses = (fxIt != channelFXBuses.end()) ? &fxIt->second : nullptr;
        for (int busNum : definedBuses)
        {
            bool on = (activeBuses != nullptr) && activeBuses->count(busNum) > 0;
            send(WingOscProtocol::channelSendOnMessage(ch, busNum, on));
        }
    }
}
