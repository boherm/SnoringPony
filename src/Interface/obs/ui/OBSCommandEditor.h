/*
  ==============================================================================

    OBSCommandEditor.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class OBSCommand;

class OBSCommandEditor :
    public BaseItemEditor
{
public:
    OBSCommandEditor(Array<OBSCommand*> commands, bool isRoot);
    ~OBSCommandEditor();

    TriggerUI* testBtnUI;
    BoolToggleUI* editableUI;

    virtual void resizedInternalHeaderItemInternal(Rectangle<int>& r) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBSCommandEditor)
};
