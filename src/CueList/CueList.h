/*
  ==============================================================================

    CueList.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class CueList : 
  public BaseItem {
  public:
    CueList();
    ~CueList();

    juce::String getTypeString() const override { return "CueList"; }
    static CueList *create(juce::var) { return new CueList(); }
};
