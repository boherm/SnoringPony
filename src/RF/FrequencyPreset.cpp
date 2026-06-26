/*
  ==============================================================================

    FrequencyPreset.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "FrequencyPreset.h"

FrequencyPreset::FrequencyPreset(var params) :
    BaseItem(params.getProperty("name", "Mode"))
{
    itemDataType = "Frequency Preset";
    canBeDisabled = false;

    freqMHz = addFloatParameter("Frequency", "Carrier frequency of this preset, in MHz", 500.0f, 1.0f, 6000.0f);
}

FrequencyPreset::~FrequencyPreset()
{
}
