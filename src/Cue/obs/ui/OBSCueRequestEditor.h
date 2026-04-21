/*
  ==============================================================================

    OBSCueRequestEditor.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class OBSCueRequest;

class OBSCueRequestEditor :
    public BaseItemEditor
{
public:
    OBSCueRequestEditor(Array<OBSCueRequest*> requests, bool isRoot);
    virtual ~OBSCueRequestEditor();

    TargetParameterUI* targetUI;
    TriggerUI* testBtnUI;

    virtual void resizedInternalHeaderItemInternal(Rectangle<int>& r) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBSCueRequestEditor)
};
