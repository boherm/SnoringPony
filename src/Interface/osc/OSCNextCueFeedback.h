/*
  ==============================================================================

    OSCNextCueFeedback.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "OSCFeedbackItem.h"

class OSCNextCueFeedback : public OSCFeedbackItem
{
public:
    OSCNextCueFeedback();
    void sendFeedback() override;

    String getTypeString() const override { return "Next Cue"; }
    static OSCNextCueFeedback* create(var params) { return new OSCNextCueFeedback(); }
};
