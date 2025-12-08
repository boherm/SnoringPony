/*
  ==============================================================================

    ActionCue.h
    Created: 3 Dec 2025 06:30:22pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"
#include "juce_organicui/controllable/parameter/BoolParameter.h"
#include "juce_organicui/controllable/parameter/TargetParameter.h"

class FadeCue :
    public Cue,
    public Timer
{
public:
    FadeCue(var params = var());
    virtual ~FadeCue();

    TargetParameter* targetCue;
    FloatParameter* volume;
    BoolParameter* stopAtEnd;

    String getTypeString() const override { return "Fade"; }
    String getCueType() const override { return "Fade"; }
    static FadeCue* create(var params) { return new FadeCue(params); }

    void play() override;
    void stop() override;
    void panic() override;

    void parameterValueChanged(Parameter* p) override;

private:
    void timerCallback() override;
};
