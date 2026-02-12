/*
  ==============================================================================

    AudioSlices.h
Created: 22 Dec 2025 04:26:23am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "juce_organicui/controllable/ControllableContainer.h"

class AudioCue;

class AudioSlice:
    public BaseItem
{
public:
    AudioSlice(var params = var(), AudioCue* audioCue = nullptr);
    virtual ~AudioSlice();

    AudioCue* audioCue;
    int playedRepetitions = 0;
    bool skipRepetitions = false;

    FloatParameter* startTime;
    FloatParameter* endTime;
    FloatParameter* duration;
    IntParameter* repetitions;
    BoolParameter* loopSlice;
    // BoolParameter* skipSlice; // TODO: could be useful?

    String getTypeString() const override { return "AudioSlice"; }
    static AudioSlice* create(var params) { return new AudioSlice(params); }

    void parameterValueChanged(Parameter* p) override;

    void resetPlayedRepetitions();

};

// ------------------------------------------------------------------------------

class AudioSlicesManager:
    public BaseManager<AudioSlice>
{
public:
    AudioSlicesManager(AudioCue* audioCue);
    virtual ~AudioSlicesManager();

    AudioCue* audioCue;
    FloatParameter* startTime;
    FloatParameter* endTime;
    FloatParameter* fadeInDuration;
    FloatParameter* fadeOutDuration;

    bool fadeOutTriggered = false;

    double getTotalDuration();
    double processTime(double realCurrentTime);
    bool hasLoopingSlice();
    void resetSlices();

    AudioSlice* createItem() override;

    void parameterValueChanged(Parameter* p) override;
};
