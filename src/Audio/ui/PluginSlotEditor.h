/*
  ==============================================================================

    PluginSlotEditor.h
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class PluginSlot;

class PluginSlotEditor :
    public BaseItemEditor
{
public:
    PluginSlotEditor(Array<PluginSlot*> slots, bool isRoot);
    ~PluginSlotEditor();

    PluginSlot* slot;

    std::unique_ptr<juce::TextButton> selectBtn;
    std::unique_ptr<juce::TextButton> editBtn;

    void buttonClicked(juce::Button* b) override;

    void resizedInternalHeaderItemInternal(juce::Rectangle<int>& r) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSlotEditor)
};
