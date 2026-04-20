/*
  ==============================================================================

    OSCIsPanickingFeedback.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "OSCFeedbackItem.h"

class OSCIsPanickingFeedback : public OSCFeedbackItem
{
public:
    OSCIsPanickingFeedback();
    void sendFeedback() override;

    String getTypeString() const override { return "Is Panicking"; }
    static OSCIsPanickingFeedback* create(var params) { return new OSCIsPanickingFeedback(); }
};
