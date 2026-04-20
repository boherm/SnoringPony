/*
  ==============================================================================

    MIDIFeedbackFactory.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MIDIFeedbackItem.h"

class MIDIFeedbackFactory
{
public:
    juce_DeclareSingleton(MIDIFeedbackFactory, true)

    MIDIFeedbackFactory();
    ~MIDIFeedbackFactory();

    Factory<MIDIFeedbackItem> factory;
};
