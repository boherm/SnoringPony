/*
  ==============================================================================

    OSCFeedbackFactory.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "OSCFeedbackItem.h"

class OSCFeedbackFactory
{
public:
    juce_DeclareSingleton(OSCFeedbackFactory, true)

    OSCFeedbackFactory();
    ~OSCFeedbackFactory();

    Factory<OSCFeedbackItem> factory;
};
