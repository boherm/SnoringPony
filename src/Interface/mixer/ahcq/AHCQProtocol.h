/*
  ==============================================================================

    AHCQProtocol.h
    Created: 22 Apr 2026
    Author:  boherm

    Allen & Heath CQ series MIDI-over-TCP protocol helpers.
    The CQ uses raw MIDI bytes over a TCP socket on port 51325.
    All parameter control is done via NRPN messages on MIDI channel 0.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cstdint>

class AHCQProtocol
{
public:
    static constexpr int DEFAULT_TCP_PORT = 51325;
    static constexpr int MAX_INPUT_CHANNELS = 24;  // CQ-20B: 16 mono + 4 stereo + USB + BT
    static constexpr int MAX_DCAS = 4;

    // -----------------------------------------------------------------------
    // NRPN message builder (12 bytes: CC99, CC98, CC6, CC38 on MIDI ch 0)
    // -----------------------------------------------------------------------
    static std::vector<uint8_t> nrpnSet(uint8_t msb, uint8_t lsb,
                                         uint8_t vc, uint8_t vf);

    // -----------------------------------------------------------------------
    // Mute / unmute  (NRPN data value: 0x01 = muted, 0x00 = unmuted)
    // -----------------------------------------------------------------------

    // Input channel mute.  channelNum is 1-based.
    static std::vector<uint8_t> inputMuteMessage(int channelNum, bool muted);

private:
    // NRPN addresses (MSB, LSB) for input channel mutes.
    // Input N (0-indexed): MSB=0x00, LSB=N
    static constexpr uint8_t MUTE_INPUT_MSB = 0x00;
};
