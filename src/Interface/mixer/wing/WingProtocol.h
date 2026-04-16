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

    // Wing colour palette values.
    static constexpr int COLOR_OFF = 0;
    static constexpr int COLOR_BLUE = 2;
    static constexpr int COLOR_GREEN = 5;
    static constexpr int COLOR_YELLOW = 7;
    static constexpr int COLOR_RED = 9;
    static constexpr int COLOR_LIGHT_BLUE = 14;
    static constexpr int COLOR_DCA_SINGLE = COLOR_LIGHT_BLUE;   // one channel in the DCA
    static constexpr int COLOR_DCA_MULTI  = COLOR_GREEN;        // multiple channels in the DCA
    static constexpr int COLOR_CHANNEL_OFF = COLOR_YELLOW;      // declared but not used in the cue

    static juce::OSCMessage channelNameMessage(int channelNum, const juce::String& name);
    static juce::OSCMessage channelMuteMessage(int channelNum, bool muted);
    static juce::OSCMessage channelColorMessage(int channelNum, int color);
    static juce::OSCMessage channelLedMessage(int channelNum, bool on);
    static juce::OSCMessage channelIconMessage(int channelNum, int icon);
    static juce::OSCMessage channelCustomLinkMessage(int channelNum, bool linked);
    static juce::OSCMessage channelSendOnMessage(int channelNum, int busNum, bool on);
    static juce::OSCMessage dcaNameMessage(int dcaNum, const juce::String& name);
    static juce::OSCMessage dcaColorMessage(int dcaNum, int color);
    static juce::OSCMessage dcaLedMessage(int dcaNum, bool on);
    static juce::OSCMessage dcaFaderMessage(int dcaNum, float dB);

    // Build one OSC message per channel in `channels` encoding its DCA membership
    // as a comma-separated string of tags ("#D1,#D2" for channels in DCA 1 and 2, empty if none).
    // membership[i] = list of channel numbers in DCA i+1 (1-indexed)
    static juce::Array<juce::OSCMessage> dcaMembershipMessages(
        const juce::Array<juce::Array<int>>& membership,
        const juce::Array<int>& channels);
};
