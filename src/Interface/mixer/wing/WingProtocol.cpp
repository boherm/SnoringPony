/*
  ==============================================================================

    WingProtocol.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "WingProtocol.h"

juce::OSCMessage WingProtocol::channelNameMessage(int channelNum, const juce::String& name)
{
    juce::OSCMessage m("/ch/" + juce::String(channelNum) + "/name");
    m.addString(name);
    return m;
}

juce::OSCMessage WingProtocol::channelMuteMessage(int channelNum, bool muted)
{
    juce::OSCMessage m("/ch/" + juce::String(channelNum) + "/mute");
    m.addInt32(muted ? 1 : 0);
    return m;
}

juce::OSCMessage WingProtocol::channelColorMessage(int channelNum, int color)
{
    juce::OSCMessage m("/ch/" + juce::String(channelNum) + "/col");
    m.addInt32(color);
    return m;
}

juce::OSCMessage WingProtocol::channelLedMessage(int channelNum, bool on)
{
    juce::OSCMessage m("/ch/" + juce::String(channelNum) + "/led");
    m.addInt32(on ? 1 : 0);
    return m;
}

juce::OSCMessage WingProtocol::channelIconMessage(int channelNum, int icon)
{
    juce::OSCMessage m("/ch/" + juce::String(channelNum) + "/icon");
    m.addInt32(icon);
    return m;
}

juce::OSCMessage WingProtocol::channelCustomLinkMessage(int channelNum, bool linked)
{
    juce::OSCMessage m("/ch/" + juce::String(channelNum) + "/clink");
    m.addInt32(linked ? 1 : 0);
    return m;
}

juce::OSCMessage WingProtocol::dcaNameMessage(int dcaNum, const juce::String& name)
{
    juce::OSCMessage m("/dca/" + juce::String(dcaNum) + "/name");
    m.addString(name);
    return m;
}

juce::OSCMessage WingProtocol::dcaColorMessage(int dcaNum, int color)
{
    juce::OSCMessage m("/dca/" + juce::String(dcaNum) + "/col");
    m.addInt32(color);
    return m;
}

juce::OSCMessage WingProtocol::dcaLedMessage(int dcaNum, bool on)
{
    juce::OSCMessage m("/dca/" + juce::String(dcaNum) + "/led");
    m.addInt32(on ? 1 : 0);
    return m;
}

juce::Array<juce::OSCMessage> WingProtocol::dcaMembershipMessages(
    const juce::Array<juce::Array<int>>& membership,
    const juce::Array<int>& channels)
{
    juce::Array<juce::OSCMessage> result;

    for (int ch : channels)
    {
        juce::StringArray tags;
        for (int d = 0; d < membership.size(); ++d)
        {
            if (membership[d].contains(ch))
                tags.add("#D" + juce::String(d + 1));
        }

        juce::OSCMessage m("/ch/" + juce::String(ch) + "/tags");
        m.addString(tags.joinIntoString(","));
        result.add(m);
    }

    return result;
}
