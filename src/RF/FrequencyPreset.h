/*
  ==============================================================================

    FrequencyPreset.h
    Created: 26 Jun 2026
    Author:  boherm

    One selectable frequency of an RF device declared in "Presets" mode (the
    "mode a / mode b ..." of a wireless mic or IEM). Just a name + a frequency.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class FrequencyPreset :
    public BaseItem
{
public:
    FrequencyPreset(var params = var());
    virtual ~FrequencyPreset();

    FloatParameter* freqMHz;   // carrier frequency, MHz

    String getTypeString() const override { return "Frequency Preset"; }
    static FrequencyPreset* create(var params) { return new FrequencyPreset(params); }
};
