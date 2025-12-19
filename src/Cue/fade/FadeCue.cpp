/*
  ==============================================================================

    FadeCue.cpp
    Created: 3 Dec 2025 06:30:22pm
    Author:  boherm

  ==============================================================================
*/

#include "FadeCue.h"
#include "../../Cuelist/CuelistManager.h"
#include "../audio/AudioCue.h"

FadeCue::FadeCue(var params) :
    Cue(params)
{
	itemDataType = "Fade Cue";
    targetCue = addTargetParameter("Target Cue", "Define the targeted cue");
    targetCue->targetType = TargetParameter::CONTAINER;
    targetCue->customGetTargetContainerFunc = &CuelistManager::showMenuForTargetCue;

    volume = addFloatParameter("Volume", "Volume for this audio file", 1.0, 0.0, 1.5, 0.01);

    stopAtEnd = addBoolParameter("Stop at the end", "Stop the cue at the end", false);
}

FadeCue::~FadeCue()
{
}

bool FadeCue::canBePlayed()
{
    if (!Cue::canBePlayed() || getWarningMessage().isNotEmpty())
        return false;
    return true;
}

void FadeCue::playInternal()
{
    Cue* target = targetCue->getTargetContainerAs<Cue>();
    if (target == nullptr || !target->isFadable) {
        endCue();
        return;
    }
    target->fade(volume->doubleValue(), duration->doubleValue());
    currentTime->setValue(0.0);
    startTimer(50);
}

void FadeCue::stopInternal()
{
    stopTimer();
    currentTime->setValue(0.0);
    isPlaying->setValue(false);
    isPanicking = false;
}

void FadeCue::panicInternal()
{
    stopInternal();
}

void FadeCue::timerCallback()
{
    ControllableContainer* targetContainer = targetCue->getTargetContainer();
    Cue* target = dynamic_cast<Cue*>(targetContainer);

    if (target->isFadable) {
        double time = currentTime->floatValue();
        time += 0.05f;
        currentTime->setValue(time);
        isPlaying->setValue(true);

        if (time > duration->floatValue())
        {
            stopTimer();
            currentTime->setValue(0.0);
            isPlaying->setValue(false);

            if (stopAtEnd->boolValue()) {
                target->stop();
            }
            endCue();
        }
    } else {
        stopTimer();
        currentTime->setValue(0.0);
        isPlaying->setValue(false);
        endCue();
    }
}

void FadeCue::parameterValueChanged(Parameter* p)
{
    Cue::parameterValueChanged(p);

    clearWarning();

    if (p == targetCue) {
        Cue* target = targetCue->getTargetContainerAs<Cue>();

        if (target == nullptr || !target->isFadable) {
            setWarningMessage("Target Cue is not valid");
        }
    }
    notifyWarningChanged();
}

String FadeCue::autoDescriptionInternal()
{
    String description = "Fade";

    if (stopAtEnd->boolValue()) {
        description += " and Stop";
    }
    description += ": ";

    Cue* target = targetCue->getTargetContainerAs<Cue>();

    if (target != nullptr) {
        description += target->niceName + " - " + target->getDescription();
    } else {
        description += "(No Target)";
    }

    return description;
}
