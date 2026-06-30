/*
  ==============================================================================

    MarkTimerCue.h
    Created: 30 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "TimerActionCue.h"
#include "../../Timer/ShowTimer.h"

class MarkTimerCue :
    public TimerActionCue
{
public:
    MarkTimerCue(var params = var()) : TimerActionCue(params)
    {
        itemDataType = "Mark Timer";
        markLabel = addStringParameter("Mark Label", "Label of the created mark (leave empty to auto-number)", "");
    }

    StringParameter* markLabel;

    void applyTimerAction(ShowTimer* t) override { t->addMark(markLabel->stringValue()); }

    String getTypeString() const override { return "Mark Timer"; }
    String getCueType() const override { return "TimerMark"; }
    static MarkTimerCue* create(var params) { return new MarkTimerCue(params); }
};
