/*
  ==============================================================================

    ColorPresets.cpp
    Created: 04 Oct 2025 01:02:00pm
    Author:  boherm

  ==============================================================================
*/

#include "ColorPresets.h"

ColorPresets::ColorPresets() :
    ControllableContainer("Color Presets")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;
    userCanAddControllables = true;
    userAddControllablesFilters.add(ColorParameter::getTypeStringStatic());
}

juce_ImplementSingleton(ColorPresets);
