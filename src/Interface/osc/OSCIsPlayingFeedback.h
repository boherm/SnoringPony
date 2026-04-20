/*
  ==============================================================================

    OSCIsPlayingFeedback.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "OSCFeedbackItem.h"

class OSCIsPlayingFeedback : public OSCFeedbackItem
{
public:
    OSCIsPlayingFeedback();
    void sendFeedback() override;

    String getTypeString() const override { return "Is Playing"; }
    static OSCIsPlayingFeedback* create(var params) { return new OSCIsPlayingFeedback(); }
};
