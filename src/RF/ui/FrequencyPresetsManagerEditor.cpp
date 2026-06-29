/*
  ==============================================================================

    FrequencyPresetsManagerEditor.cpp
    Created: 29 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "FrequencyPresetsManagerEditor.h"
#include "FrequencyPresetsImportDialog.h"

using namespace juce;

FrequencyPresetsManagerEditor::FrequencyPresetsManagerEditor(BaseManager<FrequencyPreset>* manager, bool isRoot) :
    GenericManagerEditor<FrequencyPreset>(manager, isRoot)
{
    addItemText = "Add a frequency preset";
    noItemText  = "No presets yet. Use Import to paste a list of frequencies.";

    importBtn = std::make_unique<TextButton>("Import");
    importBtn->setTooltip("Paste a list of frequencies (one per line) to add them as presets");
    importBtn->addListener(this);
    addAndMakeVisible(importBtn.get());

    resized();
}

FrequencyPresetsManagerEditor::~FrequencyPresetsManagerEditor()
{
}

void FrequencyPresetsManagerEditor::resizedInternalHeader(Rectangle<int>& r)
{
    if (addItemBT != nullptr)
    {
        addItemBT->setBounds(r.removeFromRight(r.getHeight()));
        r.removeFromRight(2);
    }

    if (importBtn != nullptr)
    {
        importBtn->setBounds(r.removeFromRight(60).reduced(0, 1));
        r.removeFromRight(4);
    }

    EnablingControllableContainerEditor::resizedInternalHeader(r);
}

void FrequencyPresetsManagerEditor::buttonClicked(Button* b)
{
    if (b == importBtn.get())
    {
        FrequencyPresetsImportDialog::show(manager);
        return;
    }

    GenericManagerEditor<FrequencyPreset>::buttonClicked(b);
}

void FrequencyPresetsManagerEditor::addPopupMenuItems(PopupMenu* p)
{
    GenericManagerEditor<FrequencyPreset>::addPopupMenuItems(p);
    p->addSeparator();
    p->addItem(importMenuId, "Import from text...");
}

void FrequencyPresetsManagerEditor::handleMenuSelectedID(int id)
{
    if (id == importMenuId)
    {
        FrequencyPresetsImportDialog::show(manager);
        return;
    }

    GenericManagerEditor<FrequencyPreset>::handleMenuSelectedID(id);
}
