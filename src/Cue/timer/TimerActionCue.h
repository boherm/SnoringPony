/*
  ==============================================================================

    TimerActionCue.h
    Created: 25 Jun 2026
    Author:  boherm

    Base for cues that act on a ShowTimer (start / pause / reset). The three concrete
    cue types live in their own classes so each has a stable factory type.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class ShowTimer;

class TimerActionCue :
    public Cue
{
public:
    TimerActionCue(var params = var());
    virtual ~TimerActionCue();

    TargetParameter* targetTimer;

    virtual void applyTimerAction(ShowTimer* t) = 0;

    void playInternal() override;
    void parameterValueChanged(Parameter* p) override;
    void checkWarning(); // warn when no timer is selected, clear it otherwise

    String getCueType() const override { return "Timer"; }

    String autoDescriptionInternal() override;
};
