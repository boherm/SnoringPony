/*
  ==============================================================================

    PauseTimerCue.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "TimerActionCue.h"
#include "../../Timer/ShowTimer.h"

class PauseTimerCue :
    public TimerActionCue
{
public:
    PauseTimerCue(var params = var()) : TimerActionCue(params) { itemDataType = "Pause Timer"; }
    void applyTimerAction(ShowTimer* t) override { t->pause(); }

    String getTypeString() const override { return "Pause Timer"; }
    String getCueType() const override { return "TimerPause"; }
    static PauseTimerCue* create(var params) { return new PauseTimerCue(params); }
};
