/*
  ==============================================================================

    ShowTimer.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "ShowTimer.h"
#include "ui/ShowTimerEditor.h"

ShowTimer::ShowTimer(var params) :
    BaseItem(params.getProperty("name", "Timer"))
{
    itemDataType = "Timer";
    canBeDisabled = false;

    setHasCustomColor(true);
    itemColor->hideInEditor = false;
    itemColor->setDefaultValue(BG_COLOR.brighter(.2f));
    itemColor->setControlMode(Parameter::ControlMode::REFERENCE);

    timerType = addEnumParameter("Type", "Chrono counts up; Countdown counts down from a duration");
    timerType->addOption("Chrono", CHRONO)->addOption("Countdown", COUNTDOWN);

    countdownDuration = addFloatParameter("Countdown Duration", "Start value for a countdown timer", 600.0, 0.0);
    countdownDuration->defaultUI = FloatParameter::TIME;

    isGeneral = addBoolParameter("General", "Include this timer in the general total (e.g. one per act)", true);

    marksManager.reset(new MarksManager());
    marksManager->hideInEditor = true; // the editor embeds a dedicated MarksManagerUI instead
    addChildControllableContainer(marksManager.get());

    playTrigger = addTrigger("Play", "Start / resume the timer");
    pauseTrigger = addTrigger("Pause", "Pause the timer");
    resetTrigger = addTrigger("Reset", "Reset the timer to zero");
    markTrigger = addTrigger("Mark", "Add a mark at the current time");

    // Triggers are exposed through the custom editor's header instead of the body.
    playTrigger->hideInEditor = true;
    pauseTrigger->hideInEditor = true;
    resetTrigger->hideInEditor = true;
    markTrigger->hideInEditor = true;

    countdownDuration->hideInEditor = (getTimerType() != COUNTDOWN);
}

ShowTimer::~ShowTimer()
{
}

double ShowTimer::getElapsedSeconds() const
{
    if (!running) return accumulatedSec;
    double now = juce::Time::getMillisecondCounterHiRes();
    return accumulatedSec + (now - startCounterMs) / 1000.0;
}

double ShowTimer::getDisplaySeconds() const
{
    if (getTimerType() == COUNTDOWN)
        return countdownDuration->doubleValue() - getElapsedSeconds(); // signed: negative = overrun
    return getElapsedSeconds();
}

bool ShowTimer::isOverrun() const
{
    return getTimerType() == COUNTDOWN && getDisplaySeconds() < 0.0;
}

String ShowTimer::getDisplayString(bool withHundredths) const
{
    double v = getDisplaySeconds();
    bool over = (getTimerType() == COUNTDOWN && v < 0.0);
    return (over ? "+" : "") + secondsToString(over ? -v : v, withHundredths);
}

void ShowTimer::start()
{
    if (running) return;
    startCounterMs = juce::Time::getMillisecondCounterHiRes();
    running = true;
}

void ShowTimer::pause()
{
    if (!running) return;
    accumulatedSec = getElapsedSeconds();
    running = false;
}

void ShowTimer::resetTimer()
{
    running = false;
    accumulatedSec = 0.0;
    startCounterMs = juce::Time::getMillisecondCounterHiRes();
    clearMarks();
}

double ShowTimer::getMarksTotalSeconds() const
{
    double total = 0.0;
    for (auto* m : marksManager->items) total += m->duration->doubleValue();
    return total;
}

void ShowTimer::addMark(const String& label)
{
    // The new mark spans from the previous mark (i.e. the elapsed time already covered
    // by existing marks) up to the current elapsed time.
    double cumulative = getElapsedSeconds();
    double duration = cumulative - getMarksTotalSeconds();
    if (duration < 0.0) duration = 0.0;

    String l = label.trim();
    if (l.isEmpty()) l = "Mark " + String(marksManager->items.size() + 1);

    marksManager->addMark(l, duration, cumulative);
}

void ShowTimer::onContainerTriggerTriggered(Trigger* t)
{
    if (t == playTrigger) start();
    else if (t == pauseTrigger) pause();
    else if (t == resetTrigger) resetTimer();
    else if (t == markTrigger) addMark();
}

void ShowTimer::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == timerType)
    {
        bool countdown = (getTimerType() == COUNTDOWN);

        // Show the start-value field only for countdowns.
        countdownDuration->hideInEditor = !countdown;

        // Countdowns can never be "general": disable and clear the flag.
        isGeneral->setEnabled(!countdown);
        if (countdown) isGeneral->setValue(false);

        // Rebuild the open editor so the changes are reflected live.
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}

void ShowTimer::parameterControlModeChanged(Parameter* p)
{
    if (p == itemColor && itemColor->controlMode == Parameter::ControlMode::REFERENCE)
        itemColor->referenceTarget->setRootContainer(ProjectSettings::getInstance()->getControllableContainerByName("colorPresets"));
}

InspectableEditor* ShowTimer::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new ShowTimerEditor(this, isRoot);
}

String ShowTimer::secondsToString(double seconds, bool withHundredths)
{
    if (seconds < 0.0) seconds = 0.0;
    int total = (int)seconds;
    int h = total / 3600;
    int m = (total % 3600) / 60;
    int s = total % 60;
    if (withHundredths)
    {
        int cc = (int)((seconds - total) * 100.0);
        return String::formatted("%02d:%02d:%02d.%02d", h, m, s, cc);
    }
    return String::formatted("%02d:%02d:%02d", h, m, s);
}
