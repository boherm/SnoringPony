/*
  ==============================================================================

    RFDevice.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFDevice.h"
#include "RFProfileManager.h"

RFDevice::RFDevice(var params) :
    BaseItem(params.getProperty("name", "Device"))
{
    itemDataType = "RF Device";

    // The device's assignable frequencies come from a referenced profile.
    profile = addTargetParameter("Profile", "Frequency profile (assignable range / presets) this device uses",
                                 RFProfileManager::getInstance());
    profile->targetType = TargetParameter::CONTAINER;
    // Only list the profiles themselves (direct children of the manager) and show just
    // their name: no "Project Settings > ... >" breadcrumb, no descending into a profile.
    profile->showParentNameInEditor = false;
    profile->maxDefaultSearchLevel = 0;

    // Read-only in the UI (feedback-only) but still saved with the show: feedback-only
    // params are skipped on save unless forceSaveValue is set.
    assignedFreqMHz = addFloatParameter("Assigned Frequency", "Frequency chosen by the optimizer, in MHz (0 = unassigned)", 0.0f, 0.0f, 6000.0f);
    assignedFreqMHz->setControllableFeedbackOnly(true);
    assignedFreqMHz->forceSaveValue = true;

    assignedPresetName = addStringParameter("Assigned Preset", "Name of the chosen preset (Presets profiles only)", "");
    assignedPresetName->setControllableFeedbackOnly(true);
    assignedPresetName->forceSaveValue = true;
}

RFDevice::~RFDevice()
{
}

RFProfile* RFDevice::getProfile() const
{
    return profile->getTargetContainerAs<RFProfile>();
}

RFProfile::CandidateMode RFDevice::getCandidateMode() const
{
    if (auto* p = getProfile()) return p->getCandidateMode();
    return RFProfile::RANGE;
}

std::vector<RFProfile::Candidate> RFDevice::getCandidates() const
{
    if (auto* p = getProfile()) return p->getCandidates();
    return {};
}

void RFDevice::setAssignment(double freqMHz, const juce::String& presetName)
{
    assignedFreqMHz->setValue((float)freqMHz);
    assignedPresetName->setValue(getCandidateMode() == RFProfile::PRESETS ? presetName : juce::String());
}

void RFDevice::clearAssignment()
{
    assignedFreqMHz->setValue(0.0f);
    assignedPresetName->setValue(juce::String());
}
