/*
  ==============================================================================

    AHCQProtocol.cpp
    Created: 22 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "AHCQProtocol.h"

std::vector<uint8_t> AHCQProtocol::nrpnSet(uint8_t msb, uint8_t lsb,
                                             uint8_t vc, uint8_t vf)
{
    return {
        0xB0, 0x63, msb,   // CC 99 = NRPN Parameter MSB
        0xB0, 0x62, lsb,   // CC 98 = NRPN Parameter LSB
        0xB0, 0x06, vc,    // CC  6 = Data Entry MSB (coarse)
        0xB0, 0x26, vf     // CC 38 = Data Entry LSB (fine)
    };
}

std::vector<uint8_t> AHCQProtocol::inputMuteMessage(int channelNum, bool muted)
{
    uint8_t lsb = static_cast<uint8_t>(channelNum - 1);
    return nrpnSet(MUTE_INPUT_MSB, lsb, 0x00, muted ? 0x01 : 0x00);
}
