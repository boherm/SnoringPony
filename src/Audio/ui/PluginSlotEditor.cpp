/*
  ==============================================================================

    PluginSlotEditor.cpp
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "PluginSlotEditor.h"
#include "../PluginSlot.h"

using namespace juce;

PluginSlotEditor::PluginSlotEditor(Array<PluginSlot*> slots, bool isRoot) :
    BaseItemEditor(Inspectable::getArrayAs<PluginSlot, BaseItem>(slots), isRoot),
    slot(slots.getFirst())
{
    selectBtn = std::make_unique<TextButton>("Select");
    selectBtn->addListener(this);
    addAndMakeVisible(selectBtn.get());

    editBtn = std::make_unique<TextButton>("Edit");
    editBtn->addListener(this);
    addAndMakeVisible(editBtn.get());

}

PluginSlotEditor::~PluginSlotEditor()
{
}

void PluginSlotEditor::buttonClicked(Button* b)
{
    if (b == selectBtn.get())
    {
        if (slot != nullptr)
            slot->selectPluginBtn->trigger();
        return;
    }

    if (b == editBtn.get())
    {
        if (slot != nullptr)
            slot->editBtn->trigger();
        return;
    }

    BaseItemEditor::buttonClicked(b);
}

void PluginSlotEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    editBtn->setBounds(r.removeFromRight(60).reduced(2));
    selectBtn->setBounds(r.removeFromRight(60).reduced(2));
}
