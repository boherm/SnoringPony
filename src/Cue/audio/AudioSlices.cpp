/*
  ==============================================================================

    AudioSlices.cpp
Created: 22 Dec 2025 04:26:23am
    Author:  boherm

  ==============================================================================
*/

#include "AudioSlices.h"
#include "AudioCue.h"
#include "AudioFile.h"
#include "juce_core/juce_core.h"

AudioSlice::AudioSlice(var params, AudioCue* audioCue) :
    BaseItem("Slice 1")
{
    this->audioCue = audioCue;
    isSelectable = false;
    // canBeDisabled = false;
    canBeReorderedInEditor = false;
    editorIsCollapsed = true;
    setHasCustomColor(true);

    startTime = addFloatParameter("Start Time", "The start time of the audio slice in seconds.", 0.0f, 0.0f);
    startTime->defaultUI = FloatParameter::TIME;

    endTime = addFloatParameter("End Time", "The end time of the audio slice in seconds.", 0.0f, 0.0f);
    endTime->defaultUI = FloatParameter::TIME;

    duration = addFloatParameter("Duration", "The duration of the audio slice in seconds.", 0.0f, 0.0f);
    duration->defaultUI = FloatParameter::TIME;
    duration->setEnabled(false);

    repetitions = addIntParameter("Repetitions", "Number of times to repeat the slice.", 1, 1, 999);
    loopSlice = addBoolParameter("Loop Slice", "If enabled, the slice will loop when it reaches the end.", false);
}

AudioSlice::~AudioSlice()
{
}

void AudioSlice::parameterValueChanged(Parameter* parameter)
{
    if (parameter == startTime || parameter == endTime)
    {
        double dur = endTime->doubleValue() - startTime->doubleValue();
        duration->setValue(juce::jmax(dur, .0));

        if (startTime->doubleValue() < 0.0)
            startTime->setValue(0.0);
        if (endTime->doubleValue() < startTime->doubleValue())
            endTime->setValue(startTime->doubleValue());
        if (endTime->doubleValue() > audioCue->initialDuration->doubleValue())
            endTime->setValue(audioCue->initialDuration->doubleValue());
        if (startTime->doubleValue() > audioCue->initialDuration->doubleValue())
            startTime->setValue(audioCue->initialDuration->doubleValue());
        if (startTime->doubleValue() < audioCue->slicesManager->startTime->doubleValue())
            startTime->setValue(audioCue->slicesManager->startTime->doubleValue());
        if (endTime->doubleValue() > audioCue->slicesManager->endTime->doubleValue())
            endTime->setValue(audioCue->slicesManager->endTime->doubleValue());
    }
    BaseItem::parameterValueChanged(parameter);
}

void AudioSlice::resetPlayedRepetitions()
{
    playedRepetitions = 0;
    skipRepetitions = false;
}

// ------------------------------------------------------------------------------

AudioSlicesManager::AudioSlicesManager(AudioCue* audioCue) :
    BaseManager<AudioSlice>("Slices")
{
    this->audioCue = audioCue;

    if (this->audioCue == nullptr)
        return;

    startTime = addFloatParameter("Start Time", "Start audio at", 0.0f, 0.0f, this->audioCue->initialDuration->doubleValue());
    startTime->defaultUI = FloatParameter::TIME;
    startTime->defaultValue = 0.0f;

    fadeInDuration = addFloatParameter("Fade In", "Fade in duration in seconds.", 0.0f, 0.0f);
    fadeInDuration->defaultUI = FloatParameter::TIME;

    endTime = addFloatParameter("End Time", "End audio at", this->audioCue->initialDuration->doubleValue(), 0.0f, this->audioCue->initialDuration->doubleValue());
    endTime->defaultUI = FloatParameter::TIME;
    endTime->defaultValue = this->audioCue->initialDuration->doubleValue();

    fadeOutDuration = addFloatParameter("Fade Out", "Fade out duration in seconds.", 0.0f, 0.0f);
    fadeOutDuration->defaultUI = FloatParameter::TIME;

    this->audioCue->initialDuration->addParameterListener(this);
}

AudioSlicesManager::~AudioSlicesManager()
{
    this->audioCue->initialDuration->removeParameterListener(this);
}

double AudioSlicesManager::getTotalDuration()
{
    double totalDuration = 0.0;
    totalDuration = endTime->doubleValue() - startTime->doubleValue();

    // Add slices durations (with more than one repetition and if not looped!)
    for (int i = 0 ; i < items.size() ; i++)
    {
        AudioSlice* slice = items.getUnchecked(i);

        if (!slice->isEnabled() || slice->loopSlice->boolValue() || slice->repetitions->intValue() <= 1)
            break;

        totalDuration += slice->duration->doubleValue() * (slice->repetitions->intValue() - 1);
    }

    return totalDuration;
}

double AudioSlicesManager::processTime(double realCurrentTime)
{
    for (int i = 0; i < items.size(); i++)
    {
        AudioSlice* slice = items.getUnchecked(i);

        if (!slice->isEnabled())
            continue;

        // if (realCurrentTime < slice->startTime->doubleValue())
        //     break;

        double sliceDuration = slice->duration->doubleValue();
        int repetitions = slice->repetitions->intValue();

        if (realCurrentTime >= slice->endTime->doubleValue())
        {
            bool shouldLoop = slice->loopSlice->boolValue() || slice->playedRepetitions < slice->repetitions->intValue() - 1;
            if (shouldLoop) {
                audioCue->filesManager->setCurrentTime(slice->startTime->doubleValue());
                slice->playedRepetitions += 1;
                realCurrentTime = slice->startTime->doubleValue();
            }
        }

        if (!slice->loopSlice->boolValue())
            realCurrentTime += slice->playedRepetitions * slice->duration->doubleValue();
    }

    realCurrentTime -= startTime->doubleValue();
    return realCurrentTime;
}

bool AudioSlicesManager::hasLoopingSlice()
{
    for (int i = 0; i < items.size(); i++)
    {
        AudioSlice* slice = items.getUnchecked(i);
        if (slice->isEnabled() && slice->loopSlice->boolValue())
            return true;
    }
    return false;
}

void AudioSlicesManager::resetSlices()
{
    fadeOutTriggered = false;
    for (int i = 0 ; i < items.size() ; i++)
    {
        AudioSlice* slice = items.getUnchecked(i);
        slice->resetPlayedRepetitions();
    }
}

AudioSlice* AudioSlicesManager::createItem()
{
    return new AudioSlice(var(), audioCue);
}

void AudioSlicesManager::parameterValueChanged(Parameter* p)
{
    if (this->audioCue->initialDuration == p) {
        startTime->maximumValue = this->audioCue->initialDuration->doubleValue();
        endTime->maximumValue = this->audioCue->initialDuration->doubleValue();
        endTime->defaultValue = this->audioCue->initialDuration->doubleValue();
    }

    if (Engine::mainEngine->isLoadingFile)
        return;

    if (this->startTime == p)
    {
        if (startTime->doubleValue() < 0.0)
            startTime->setValue(0.0);
        if (startTime->doubleValue() > endTime->doubleValue())
            startTime->setValue(endTime->doubleValue());

        // Adjust slices
        for (int i = 0 ; i < items.size() ; i++)
        {
            AudioSlice* slice = items.getUnchecked(i);
            if (slice->startTime->doubleValue() < startTime->doubleValue())
                slice->startTime->setValue(startTime->doubleValue());
            if (slice->endTime->doubleValue() < startTime->doubleValue())
                slice->endTime->setValue(startTime->doubleValue());
        }
    }

    if (this->endTime == p)
    {
        if (endTime->doubleValue() > this->audioCue->initialDuration->doubleValue())
            endTime->setValue(this->audioCue->initialDuration->doubleValue());
        if (endTime->doubleValue() < startTime->doubleValue())
            endTime->setValue(startTime->doubleValue());

        // Adjust slices
        for (int i = 0 ; i < items.size() ; i++)
        {
            AudioSlice* slice = items.getUnchecked(i);
            if (slice->endTime->doubleValue() > endTime->doubleValue())
                slice->endTime->setValue(endTime->doubleValue());
            if (slice->startTime->doubleValue() > endTime->doubleValue())
                slice->startTime->setValue(endTime->doubleValue());
        }
    }

    BaseManager<AudioSlice>::parameterValueChanged(p);
}
