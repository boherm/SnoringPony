/*
  ==============================================================================

    RFProfile.cpp
    Created: 27 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFProfile.h"

RFProfile::RFProfile(var params) :
    BaseItem(params.getProperty("name", "Profile"))
{
    itemDataType = "RF Profile";
    saveAndLoadRecursiveData = true;   // so the nested Presets manager is saved
    canBeDisabled = false;

    candidateMode = addEnumParameter("Frequency Mode", "How the assignable frequencies are defined");
    candidateMode->addOption("Range", RANGE)->addOption("Presets", PRESETS);

    rangeMinMHz = addFloatParameter("Range Min", "Lowest tunable frequency, in MHz", 470.0f, 1.0f, 6000.0f);
    rangeMaxMHz = addFloatParameter("Range Max", "Highest tunable frequency, in MHz", 870.0f, 1.0f, 6000.0f);
    gridStepKHz = addFloatParameter("Tuning Step", "Tuning grid step, in kHz", 25.0f, 5.0f, 1000.0f);

    presetsManager.reset(new BaseManager<FrequencyPreset>("Presets"));
    presetsManager->selectItemWhenCreated = false;
    addChildControllableContainer(presetsManager.get());

    updateModeVisibility();
}

RFProfile::~RFProfile()
{
}

void RFProfile::updateModeVisibility()
{
    const bool presets = getCandidateMode() == PRESETS;
    rangeMinMHz->hideInEditor = presets;
    rangeMaxMHz->hideInEditor = presets;
    gridStepKHz->hideInEditor = presets;
    presetsManager->hideInEditor = !presets;
}

void RFProfile::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == candidateMode)
    {
        updateModeVisibility();
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}

std::vector<RFProfile::Candidate> RFProfile::getCandidates() const
{
    std::vector<Candidate> out;

    if (getCandidateMode() == PRESETS)
    {
        for (auto* p : presetsManager->items)
            out.push_back({ (double)p->freqMHz->floatValue(), p->niceName });
        return out;
    }

    double lo = rangeMinMHz->floatValue();
    double hi = rangeMaxMHz->floatValue();
    if (hi < lo) std::swap(lo, hi);

    double step = juce::jmax(1.0, (double)gridStepKHz->floatValue()) / 1000.0; // -> MHz

    // Cap the candidate count so a huge range with a fine grid can't blow up the
    // optimizer; widen the step if necessary.
    const int maxCandidates = 4000;
    int count = (int)((hi - lo) / step) + 1;
    if (count > maxCandidates) step = (hi - lo) / (double)(maxCandidates - 1);

    for (double f = lo; f <= hi + 1e-9; f += step)
        out.push_back({ f, juce::String() });

    return out;
}
