/*
  ==============================================================================

    RFDevice.h
    Created: 26 Jun 2026
    Author:  boherm

    A wireless RF device to coordinate (mic / IEM). It references an RFProfile that
    defines its assignable frequencies (Range or Presets); the device itself only
    keeps its optimizer result (assigned frequency and, in Presets mode, the chosen
    preset/"mode"). Enable/disable uses Organic's built-in `enabled`.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "RFProfile.h"

class RFDevice :
    public BaseItem
{
public:
    RFDevice(var params = var());
    virtual ~RFDevice();

    TargetParameter* profile;            // references an RFProfile (assignable freqs)

    // Result of the optimizer (feedback-only, but saved)
    FloatParameter*  assignedFreqMHz;
    StringParameter* assignedPresetName; // the chosen preset/"mode" (Presets profiles)

    RFProfile*               getProfile() const;
    RFProfile::CandidateMode getCandidateMode() const;             // from the profile
    std::vector<RFProfile::Candidate> getCandidates() const;       // delegates to the profile

    void setAssignment(double freqMHz, const juce::String& presetName);
    void clearAssignment();

    String getTypeString() const override { return "RF Device"; }
    static RFDevice* create(var params) { return new RFDevice(params); }
};
