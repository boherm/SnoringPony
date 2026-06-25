/*
  ==============================================================================

    ResetTimerCue.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "TimerActionCue.h"
#include "../../Timer/ShowTimer.h"

class ResetTimerCue :
    public TimerActionCue
{
public:
    ResetTimerCue(var params = var()) : TimerActionCue(params) { itemDataType = "Reset Timer"; }
    void applyTimerAction(ShowTimer* t) override { t->resetTimer(); }

    String getTypeString() const override { return "Reset Timer"; }
    String getCueType() const override { return "TimerReset"; }
    static ResetTimerCue* create(var params) { return new ResetTimerCue(params); }
};
