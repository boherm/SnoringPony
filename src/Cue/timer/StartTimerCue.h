/*
  ==============================================================================

    StartTimerCue.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "TimerActionCue.h"
#include "../../Timer/ShowTimer.h"

class StartTimerCue :
    public TimerActionCue
{
public:
    StartTimerCue(var params = var()) : TimerActionCue(params) { itemDataType = "Start Timer"; }
    void applyTimerAction(ShowTimer* t) override { t->start(); }

    String getTypeString() const override { return "Start Timer"; }
    String getCueType() const override { return "TimerStart"; }
    static StartTimerCue* create(var params) { return new StartTimerCue(params); }
};
