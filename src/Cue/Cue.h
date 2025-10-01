/*
  ==============================================================================

    Cue.h
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
// #include "../Cuelist/Cuelist.h"

class Cue:
    public BaseItem
{
public:
    Cue(var params = var());
    virtual ~Cue();

    FloatParameter* id;

    // Cuelist* parentCuelist = nullptr;

    StringParameter* description;

    juce::String getTypeString() const override { return "Cue"; }
    static Cue* create(var params) { return new Cue(params); }
};
