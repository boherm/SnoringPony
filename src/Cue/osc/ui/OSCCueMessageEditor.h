/*
  ==============================================================================

    OSCCueMessageEditor.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class OSCCueMessage;

class OSCCueMessageEditor :
    public BaseItemEditor
{
public:
    OSCCueMessageEditor(Array<OSCCueMessage*> cues, bool isRoot);
    virtual ~OSCCueMessageEditor();

    TargetParameterUI* targetUI;
    TriggerUI* testBtnUI;

    virtual void resizedInternalHeaderItemInternal(Rectangle<int> &r) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCCueMessageEditor)
};
