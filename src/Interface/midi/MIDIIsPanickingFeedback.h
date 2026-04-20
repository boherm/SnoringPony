/*
  ==============================================================================

    MIDIIsPanickingFeedback.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MIDIFeedbackItem.h"

class MIDIIsPanickingFeedback : public MIDIFeedbackItem
{
public:
    MIDIIsPanickingFeedback();
    void sendFeedback() override;

    String getTypeString() const override { return "Is Panicking"; }
    static MIDIIsPanickingFeedback* create(var params) { return new MIDIIsPanickingFeedback(); }
};
