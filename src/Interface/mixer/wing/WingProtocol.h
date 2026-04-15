/*
  ==============================================================================

    WingProtocol.h
    Created: 14 Apr 2026
    Author:  boherm

    Behringer Wing OSC addresses and helpers.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class WingProtocol
{
public:
    static constexpr int DEFAULT_SEND_PORT = 2223;
    static constexpr int DEFAULT_RECV_PORT = 2222;
    static constexpr int MAX_CHANNELS = 40;
    static constexpr int MAX_DCAS = 16;

    static juce::OSCMessage channelNameMessage(int channelNum, const juce::String& name);
    static juce::OSCMessage dcaNameMessage(int dcaNum, const juce::String& name);
    static juce::OSCMessage dcaColorMessage(int dcaNum, int color);
    static juce::OSCMessage dcaLedMessage(int dcaNum, bool on);

    // Build one OSC message per channel in `channels` encoding its DCA membership
    // as a comma-separated string of tags ("#D1,#D2" for channels in DCA 1 and 2, empty if none).
    // membership[i] = list of channel numbers in DCA i+1 (1-indexed)
    static juce::Array<juce::OSCMessage> dcaMembershipMessages(
        const juce::Array<juce::Array<int>>& membership,
        const juce::Array<int>& channels);
};
