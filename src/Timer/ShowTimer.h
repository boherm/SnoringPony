/*
  ==============================================================================

    ShowTimer.h
    Created: 25 Jun 2026
    Author:  boherm

    A ShowTimer chronometers a part of the show. Its configuration (name, color,
    "general" flag, type and countdown duration) is saved; its running value is NOT.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class ShowTimer :
    public BaseItem
{
public:
    ShowTimer(var params = var());
    virtual ~ShowTimer();

    enum TimerType { CHRONO, COUNTDOWN };

    // Saved configuration
    BoolParameter* isGeneral;          // include in the "general" total (e.g. one per act)
    EnumParameter* timerType;          // Chrono (count up) / Countdown (count down)
    FloatParameter* countdownDuration; // start value for a countdown, in seconds

    // Inspector actions
    Trigger* playTrigger;
    Trigger* pauseTrigger;
    Trigger* resetTrigger;

    // Runtime state (never saved)
    bool running = false;
    double accumulatedSec = 0.0;   // time accumulated while paused
    double startCounterMs = 0.0;   // hi-res ms counter at last start

    void start();
    void pause();
    void resetTimer();
    bool isRunning() const { return running; }

    TimerType getTimerType() const { return (TimerType)(int)timerType->getValueData(); }
    // Countdowns can never be "general" by definition.
    bool isGeneralTimer() const { return isGeneral->boolValue() && getTimerType() == CHRONO; }

    double getElapsedSeconds() const;  // raw elapsed since first start
    double getDisplaySeconds() const;  // chrono: elapsed; countdown: remaining (negative = overrun)
    bool isOverrun() const;            // countdown that has passed zero
    String getDisplayString(bool withHundredths = false) const; // formatted, with "+" when overrun

    void onContainerTriggerTriggered(Trigger* t) override;
    void onContainerParameterChangedInternal(Parameter* p) override;
    void parameterControlModeChanged(Parameter* p) override;

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables) override;

    String getTypeString() const override { return "Timer"; }
    static ShowTimer* create(var params) { return new ShowTimer(params); }

    static String secondsToString(double seconds, bool withHundredths = false);
};
