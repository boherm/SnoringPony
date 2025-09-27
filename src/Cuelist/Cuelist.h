/*
  ==============================================================================

    Cuelist.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class Cuelist : 
  public BaseItem {
  public:
    Cuelist();
    ~Cuelist();

    juce::String getTypeString() const override { return "Cuelist"; }
    static Cuelist *create(juce::var) { return new Cuelist(); }
};
