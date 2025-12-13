/*
  ==============================================================================

    OSCCommandArgumentsEditor.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class OSCCommandArgumentsEditor :
    public GenericManagerEditor<GenericControllableItem>
{
public:
    OSCCommandArgumentsEditor(GenericControllableManager* manager, bool isRoot);
    ~OSCCommandArgumentsEditor();

    BoolToggleUI* editableUI;

    void resizedInternalHeader(Rectangle<int>& r) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCCommandArgumentsEditor)
};
