/*
  ==============================================================================

    Cue.h
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "editor/CueEditor.h"

class Cuelist;

class Cue:
    public BaseItem
    // public ParameterListener
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

    String getTypeString() const override { return "Cue"; }
    virtual String getCueType() const { return "Cue"; }
    static Cue* create(var params) { return new Cue(params); }
    // void parameterValueChanged(Parameter* parameter) override;
    CueEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);

    void parameterValueChanged(Parameter* p);

    virtual void play() {}

// private:
//     AudioFormatManager formatManager;
//     std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
//     AudioTransportSource transportSource;
//     AudioSourcePlayer sourcePlayer;
//     AudioDeviceManager deviceManager;

};
