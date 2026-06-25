/*
  ==============================================================================

    TimerActionCue.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "TimerActionCue.h"
#include "../../Timer/ShowTimer.h"
#include "../../Timer/ShowTimerManager.h"

TimerActionCue::TimerActionCue(var params) :
    Cue(params)
{
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    targetTimer = addTargetParameter("Target Timer", "Timer to act on", ShowTimerManager::getInstance());
    targetTimer->targetType = TargetParameter::CONTAINER;
    targetTimer->maxDefaultSearchLevel = 0;

    checkWarning();
}

TimerActionCue::~TimerActionCue()
{
}

void TimerActionCue::playInternal()
{
    if (ShowTimer* t = targetTimer->getTargetContainerAs<ShowTimer>())
        applyTimerAction(t);
    endCue();
}

void TimerActionCue::parameterValueChanged(Parameter* p)
{
    Cue::parameterValueChanged(p);

    if (p == targetTimer)
        checkWarning();
}

void TimerActionCue::checkWarning()
{
    const String msg = "No timer selected";
    if (targetTimer->getTargetContainerAs<ShowTimer>() == nullptr)
        setWarningMessage(msg);
    else if (getWarningMessage() == msg) // only clear our own warning
        clearWarning();

    notifyWarningChanged();
}

String TimerActionCue::autoDescriptionInternal()
{
    ShowTimer* t = targetTimer->getTargetContainerAs<ShowTimer>();
    String name = (t != nullptr) ? t->niceName : String("(no timer)");
    return getTypeString() + " -> " + name;
}
