/*
  ==============================================================================

    RFDevice.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFDevice.h"

RFDevice::RFDevice(var params) :
    BaseItem(params.getProperty("name", "Device"))
{
    itemDataType = "RF Device";
    saveAndLoadRecursiveData = true;   // so the nested Presets manager is saved with the show

    // Devices use Organic's built-in enable/disable (the inherited `enabled`); a
    // disabled device is excluded from the coordination.

    candidateMode = addEnumParameter("Frequency Mode", "How the device's candidate frequencies are defined");
    candidateMode->addOption("Range", RANGE)->addOption("Presets", PRESETS);

    rangeMinMHz = addFloatParameter("Range Min", "Lowest tunable frequency, in MHz", 470.0f, 1.0f, 6000.0f);
    rangeMaxMHz = addFloatParameter("Range Max", "Highest tunable frequency, in MHz", 870.0f, 1.0f, 6000.0f);
    gridStepKHz = addFloatParameter("Tuning Step", "Tuning grid step, in kHz", 25.0f, 5.0f, 1000.0f);

    // Read-only in the UI (feedback-only) but still saved with the show: feedback-only
    // params are skipped on save unless forceSaveValue is set.
    assignedFreqMHz = addFloatParameter("Assigned Frequency", "Frequency chosen by the optimizer, in MHz (0 = unassigned)", 0.0f, 0.0f, 6000.0f);
    assignedFreqMHz->setControllableFeedbackOnly(true);
    assignedFreqMHz->forceSaveValue = true;

    assignedPresetName = addStringParameter("Assigned Preset", "Name of the chosen preset (Presets mode only)", "");
    assignedPresetName->setControllableFeedbackOnly(true);
    assignedPresetName->forceSaveValue = true;

    presetsManager.reset(new BaseManager<FrequencyPreset>("Presets"));
    presetsManager->selectItemWhenCreated = false;
    addChildControllableContainer(presetsManager.get());

    updateModeVisibility();
}

RFDevice::~RFDevice()
{
}

void RFDevice::updateModeVisibility()
{
    const bool presets = getCandidateMode() == PRESETS;

    rangeMinMHz->hideInEditor = presets;
    rangeMaxMHz->hideInEditor = presets;
    gridStepKHz->hideInEditor = presets;
    presetsManager->hideInEditor = !presets;
    assignedPresetName->hideInEditor = !presets;
}

void RFDevice::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == candidateMode)
    {
        updateModeVisibility();
        // Rebuild the open editor so the mode-dependent fields appear/disappear live.
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}

std::vector<RFDevice::Candidate> RFDevice::getCandidates() const
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

    // Cap the number of candidates so a huge range with a fine grid can't blow up
    // the optimizer; widen the step if necessary.
    const int maxCandidates = 4000;
    int count = (int)((hi - lo) / step) + 1;
    if (count > maxCandidates) step = (hi - lo) / (double)(maxCandidates - 1);

    for (double f = lo; f <= hi + 1e-9; f += step)
        out.push_back({ f, juce::String() });

    return out;
}

void RFDevice::setAssignment(double freqMHz, const juce::String& presetName)
{
    assignedFreqMHz->setValue((float)freqMHz);
    assignedPresetName->setValue(getCandidateMode() == PRESETS ? presetName : juce::String());
}

void RFDevice::clearAssignment()
{
    assignedFreqMHz->setValue(0.0f);
    assignedPresetName->setValue(juce::String());
}
