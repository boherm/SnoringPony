/*
  ==============================================================================

    Cue.h
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class Cuelist;

class Cue:
    public BaseItem,
    public ChangeBroadcaster,
    public ParameterListener
{
public:
    Cue(var params = var());
    virtual ~Cue();

    String objectType;
    var objectData;

    Cuelist* parentCuelist;

    FloatParameter* id;
    StringParameter* description;
    StringParameter* notes;

    Trigger* setNextBtn;

    String getTypeString() const override { return "Cue"; }
    virtual String getCueType() const { return "Cue"; }
    static Cue* create(var params) { return new Cue(params); }

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);

    void parameterValueChanged(Parameter* p) override;
    void parameterControlModeChanged(Parameter* p) override;

    void triggerTriggered(Trigger* t) override;

    virtual void play() {}
    void setGoNext();

// private:
//     AudioFormatManager formatManager;
//     std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
//     AudioTransportSource transportSource;
//     AudioSourcePlayer sourcePlayer;
//     AudioDeviceManager deviceManager;

};
