/*
  ==============================================================================

    DCAAssignmentEditor.h
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class DCAAssignment;

class DCAAssignmentEditor :
    public BaseItemEditor
{
public:
    DCAAssignmentEditor(Array<DCAAssignment*> assignments, bool isRoot);
    ~DCAAssignmentEditor();

    DCAAssignment* assignment;

    std::unique_ptr<juce::TextButton> editBtn;

    void buttonClicked(juce::Button* b) override;
    void newMessage(const ContainerAsyncEvent& e) override;

    virtual void resizedInternalHeaderItemInternal(juce::Rectangle<int>& r) override;

private:
    void openCharacterDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DCAAssignmentEditor)
};
