/*
  ==============================================================================

    ReorderCuesWindow.cpp
    Created: 16 Dec 2025 10:31:21am
    Author:  boherm

  ==============================================================================
*/

#include "ReorderCuesWindow.h"
#include "CuesTableModel.h"
#include "../../Cue/Cue.h"
#include "../../Cue/CueManager.h"
#include "../../Cuelist/Cuelist.h"

ReorderCuesWindow::ReorderCuesWindow(CuesTableModel* caller)
{
    this->caller = caller;
    setSize(280, 120);

    closeBtn.addListener(this);
    reorderBtn.addListener(this);

    if (caller == nullptr || caller->tlb == nullptr || caller->cl == nullptr )
        return;

    if (caller->tlb->getNumSelectedRows() == 0)
        return;

    if (caller->tlb->getSelectedRow(0) >= caller->cl->cues->items.size())
        return;

    Cue* firstCueSelected = caller->cl->cues->items[caller->tlb->getSelectedRow(0)];

    startIdEdit.setColour(startIdEdit.backgroundColourId, BG_COLOR.darker(0.1f));
    startIdEdit.setColour(startIdEdit.outlineColourId, TEXT_COLOR.darker(0.1f));
	startIdEdit.setColour(CaretComponent::caretColourId, Colours::orange);
	startIdEdit.setColour(startIdEdit.textColourId, TEXT_COLOR);
    startIdEdit.setText(firstCueSelected->id->stringValue(), dontSendNotification);
    startIdEdit.setSelectAllWhenFocused(true);

    incrementEdit.setColour(startIdEdit.backgroundColourId, BG_COLOR.darker(0.1f));
    incrementEdit.setColour(startIdEdit.outlineColourId, TEXT_COLOR.darker(0.1f));
	incrementEdit.setColour(CaretComponent::caretColourId, Colours::orange);
	incrementEdit.setColour(startIdEdit.textColourId, TEXT_COLOR);
    incrementEdit.setText("1.000", dontSendNotification);
    incrementEdit.setSelectAllWhenFocused(true);

    bottomArea.addAndMakeVisible(closeBtn);
    bottomArea.addAndMakeVisible(reorderBtn);
    addAndMakeVisible(bottomArea);

    addAndMakeVisible(startIdLabel);
    addAndMakeVisible(startIdEdit);
    addAndMakeVisible(incrementLabel);
    addAndMakeVisible(incrementEdit);
}

void ReorderCuesWindow::resized()
{
    auto area = getLocalBounds().reduced(10);

    bottomArea.setBounds(area.removeFromBottom(40));
    closeBtn.setBounds(10, 5, 100, 30);
    reorderBtn.setBounds(bottomArea.getWidth() - 110, 5, 100, 30);

    area.removeFromBottom(10);

    auto rowH = 26;
    auto labelW = 70;

    auto row1 = area.removeFromTop(rowH);
    startIdLabel.setBounds(row1.removeFromLeft(labelW));
    startIdEdit.setBounds(row1);

    area.removeFromTop(10);

    auto row2 = area.removeFromTop(rowH);
    incrementLabel.setBounds(row2.removeFromLeft(labelW));
    incrementEdit.setBounds(row2);
}

void ReorderCuesWindow::buttonClicked(Button* btn)
{
    if (&closeBtn == btn) {
        if (auto* dw = getTopLevelComponent())
            dw->exitModalState(0);
    }

    if (&reorderBtn == btn) {
        if (caller != nullptr) {
            float startId = getFloatFromEditor(startIdEdit, -1.0f);
            float increment = getFloatFromEditor(incrementEdit, -1.0f);

            if (startId < 0.0f || increment < 0.0f)
                return;

            caller->reorderCues(startId, increment);

            if (auto* dw = getTopLevelComponent())
                dw->exitModalState(0);
        }
    }
}

float ReorderCuesWindow::getFloatFromEditor(const juce::TextEditor& ed, float fallback) const
{
    auto s = ed.getText().trim().replaceCharacter(',', '.');
    auto v = s.getFloatValue();

    if (s.isEmpty() || (! juce::CharacterFunctions::isDigit(s[0]) && s[0] != '-' && s[0] != '.'))
        return fallback;

    return v;
}
