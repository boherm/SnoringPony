/*
  ==============================================================================

    RFDevice.h
    Created: 26 Jun 2026
    Author:  boherm

    A wireless RF device to coordinate: a microphone (transmitter) or an IEM
    (in-ear monitor). Its allowed frequencies are defined either by a continuous
    Range (with a tuning grid step) or by a discrete list of named Presets. The
    optimizer fills in the assigned frequency (and preset name, in Presets mode).

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "FrequencyPreset.h"

class RFDevice :
    public BaseItem
{
public:
    RFDevice(var params = var());
    virtual ~RFDevice();

    enum CandidateMode { RANGE = 0, PRESETS = 1 };

    EnumParameter*   candidateMode;      // Range / Presets

    // Range mode
    FloatParameter*  rangeMinMHz;
    FloatParameter*  rangeMaxMHz;
    FloatParameter*  gridStepKHz;        // tuning grid step

    // Result of the optimizer (feedback-only, but saved)
    FloatParameter*  assignedFreqMHz;
    StringParameter* assignedPresetName; // only meaningful in Presets mode

    // Presets mode: the list of selectable frequencies ("mode a / mode b ...")
    std::unique_ptr<BaseManager<FrequencyPreset>> presetsManager;

    struct Candidate { double freqMHz; juce::String presetName; };

    CandidateMode getCandidateMode() const { return (CandidateMode)(int)candidateMode->getValueData(); }

    // All candidate frequencies for this device (grid over the range, or the presets).
    std::vector<Candidate> getCandidates() const;

    void setAssignment(double freqMHz, const juce::String& presetName);
    void clearAssignment();

    String getTypeString() const override { return "RF Device"; }
    static RFDevice* create(var params) { return new RFDevice(params); }

    void onContainerParameterChangedInternal(Parameter* p) override;

private:
    void updateModeVisibility();
};
