/*
  ==============================================================================

    OSCCurrentCueFeedback.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "OSCFeedbackItem.h"

class OSCCurrentCueFeedback : public OSCFeedbackItem
{
public:
    OSCCurrentCueFeedback();
    void sendFeedback() override;

    String getTypeString() const override { return "Current Cue"; }
    static OSCCurrentCueFeedback* create(var params) { return new OSCCurrentCueFeedback(); }
};
