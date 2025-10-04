/*
  ==============================================================================

    ColorPresets.h
    Created: 04 Oct 2025 01:02:00pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class ColorPresets :
    public ControllableContainer
{
    public:
        juce_DeclareSingleton(ColorPresets, true);
        ColorPresets();
};
