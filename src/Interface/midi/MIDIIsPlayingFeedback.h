/*
  ==============================================================================

    MIDIIsPlayingFeedback.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MIDIFeedbackItem.h"

class MIDIIsPlayingFeedback : public MIDIFeedbackItem
{
public:
    MIDIIsPlayingFeedback();
    void sendFeedback() override;

    String getTypeString() const override { return "Is Playing"; }
    static MIDIIsPlayingFeedback* create(var params) { return new MIDIIsPlayingFeedback(); }
};
