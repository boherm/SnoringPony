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
    targetCue = addTargetParameter("Target Cue", "Define the targeted cue");
    targetCue->targetType = TargetParameter::CONTAINER;
    targetCue->customGetTargetContainerFunc = &CuelistManager::showMenuForTargetCue;

    volume = addFloatParameter("Volume", "Volume for this audio file", 1.0, 0.0, 1.5, 0.01);

    stopAtEnd = addBoolParameter("Stop at the end", "Stop the cue at the end", false);
}

FadeCue::~FadeCue()
{
}

void FadeCue::play()
{
    if (getWarningMessage().isNotEmpty()) {
        return;
    }
    currentTime->setValue(0.0);
    startTimer(50);
}

void FadeCue::stop()
{
    if (getWarningMessage().isNotEmpty()) {
        return;
    }
    currentTime->setValue(0.0);
}

void FadeCue::panic()
{
    if (getWarningMessage().isNotEmpty()) {
        return;
    }
    stopTimer();
    currentTime->setValue(0.0);
    isPlaying->setValue(false);
}

void FadeCue::timerCallback()
{
    ControllableContainer* targetContainer = targetCue->getTargetContainer();
    AudioCue* target = dynamic_cast<AudioCue*>(targetContainer);

    double time = currentTime->floatValue();
    time += 0.05f;
    currentTime->setValue(time);
    isPlaying->setValue(true);

    if (target) {
        target->setRelativeGlobalGainForFade(volume->floatValue(), currentTime->floatValue() / duration->floatValue());
    }

    if (time > duration->floatValue())
    {
        stopTimer();
        currentTime->setValue(0.0);
        isPlaying->setValue(false);
        target->savedRelativeGain = volume->floatValue();

        if (stopAtEnd->boolValue()) {
            target->stop();
        }
    }
}

void FadeCue::parameterValueChanged(Parameter* p)
{
    Cue::parameterValueChanged(p);

    if (getWarningMessage().isNotEmpty()) {
        clearWarning();
    }

    if (p == targetCue) {
        ControllableContainer* targetContainer = targetCue->getTargetContainer();
        Cue* target = dynamic_cast<Cue*>(targetContainer);

        if (target == nullptr || !target->isFadable) {
            setWarningMessage("Target Cue is not valid");
        }
    }
}
