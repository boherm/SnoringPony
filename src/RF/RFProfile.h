/*
  ==============================================================================

    RFProfile.h
    Created: 27 Jun 2026
    Author:  boherm

    A reusable frequency profile: defines the assignable frequencies either as a
    continuous Range (with a tuning grid step) or as a discrete list of named
    Presets ("mode a / mode b ..."). Devices reference a profile; the optimizer
    uses the profile's candidates to place each device.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "FrequencyPreset.h"
#include "FrequencyPresetsManager.h"

class RFProfile :
    public BaseItem
{
public:
    RFProfile(var params = var());
    virtual ~RFProfile();

    enum CandidateMode { RANGE = 0, PRESETS = 1 };

    EnumParameter*   candidateMode;      // Range / Presets
    FloatParameter*  rangeMinMHz;
    FloatParameter*  rangeMaxMHz;
    FloatParameter*  gridStepKHz;        // tuning grid step

    // Presets mode: the list of selectable frequencies ("mode a / mode b ...")
    std::unique_ptr<FrequencyPresetsManager> presetsManager;

    struct Candidate { double freqMHz; juce::String presetName; };

    CandidateMode getCandidateMode() const { return (CandidateMode)(int)candidateMode->getValueData(); }

    // All candidate frequencies (grid over the range, or the presets).
    std::vector<Candidate> getCandidates() const;

    String getTypeString() const override { return "RF Profile"; }
    static RFProfile* create(var params) { return new RFProfile(params); }

    void onContainerParameterChangedInternal(Parameter* p) override;

private:
    void updateModeVisibility();
};
