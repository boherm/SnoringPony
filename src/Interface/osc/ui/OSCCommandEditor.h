/*
  ==============================================================================

    OSCCommandEditor.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class OSCCommand;

class OSCCommandEditor :
    public BaseItemEditor
{
public:
    OSCCommandEditor(Array<OSCCommand*> commands, bool isRoot);
    ~OSCCommandEditor();

    TriggerUI* testBtnUI;

    virtual void resizedInternalHeaderItemInternal(Rectangle<int> &r) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCCommandEditor)
};
