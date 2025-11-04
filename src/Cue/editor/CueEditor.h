/*
  ==============================================================================

    CueEditor.h
    Created: 06 Oct 2025 08:47:22pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "juce_organicui/controllable/ui/TriggerUI.h"

class Cue;

class CueTypeUI:
    public Component
{
public:
    CueTypeUI(Cue* cue);
    ~CueTypeUI() override;

    Cue* currentCue;

    void paint(Graphics& g) override;
};

class CueEditor :
    public BaseItemEditor
{
public:
    CueEditor(Array<Cue*> cues, bool isRoot);
    virtual ~CueEditor();

    // std::unique_ptr<FloatParameterLabelUI> idStepperUI;
    std::unique_ptr<CueTypeUI> cueTypeUI;

    TriggerUI* goBtnUI;
    TriggerUI* goNextBtnUI;

    virtual void resizedInternalHeaderItemInternal(Rectangle<int> &r) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CueEditor)
};
