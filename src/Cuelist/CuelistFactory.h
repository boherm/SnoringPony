/*
  ==============================================================================

    CuelistFactory.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "Cuelist.h"

class CuelistFactory :
    public Factory<Cuelist>
{
public:
    juce_DeclareSingleton(CuelistFactory, true)

    CuelistFactory();
    ~CuelistFactory() {}

    // Backward-compat: old saves used the type "Cuelist" before renaming to "Playback Cuelist".
    Cuelist* create(const juce::String& type) override;
};
