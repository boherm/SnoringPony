/*
  ==============================================================================

    MixerFX.h
    Created: 15 Apr 2026
    Author:  boherm

    A MixerFX maps a user-friendly name to a bus number on the mixing
    console. Characters in a DCA assignment can be routed to a MixerFX;
    at play time, a "send on/off" is pushed for every declared channel
    toward every declared FX bus.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class MixerFX :
    public BaseItem
{
public:
    MixerFX(var params = var());
    virtual ~MixerFX();

    IntParameter* busNumber;

    String getTypeString() const override { return "FX"; }
    static MixerFX* create(var params) { return new MixerFX(params); }
};
