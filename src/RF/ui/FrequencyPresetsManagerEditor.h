/*
  ==============================================================================

    FrequencyPresetsManagerEditor.h
    Created: 29 Jun 2026
    Author:  boherm

    Inspector editor for a profile's frequency presets list. Same as the generic
    manager editor, plus an "Import" button (and matching right-click entry) that
    opens a dialog to paste a list of frequencies at once.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../FrequencyPreset.h"

class FrequencyPresetsManagerEditor :
    public GenericManagerEditor<FrequencyPreset>
{
public:
    FrequencyPresetsManagerEditor(BaseManager<FrequencyPreset>* manager, bool isRoot);
    ~FrequencyPresetsManagerEditor() override;

    void resizedInternalHeader(juce::Rectangle<int>& r) override;
    void buttonClicked(juce::Button* b) override;

    void addPopupMenuItems(juce::PopupMenu* p) override;
    void handleMenuSelectedID(int id) override;

private:
    static constexpr int importMenuId = 2001;
    std::unique_ptr<juce::TextButton> importBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyPresetsManagerEditor)
};
