/*
  ==============================================================================

    DCAAssignmentEditor.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCAAssignmentEditor.h"
#include "DCAAssignmentDialog.h"
#include "../DCAAssignment.h"
#include "../DCACue.h"

using namespace juce;

DCAAssignmentEditor::DCAAssignmentEditor(Array<DCAAssignment*> assignments, bool isRoot) :
    BaseItemEditor(Inspectable::getArrayAs<DCAAssignment, BaseItem>(assignments), isRoot),
    assignment(assignments.getFirst())
{
    editBtn = std::make_unique<TextButton>("Edit characters...");
    editBtn->addListener(this);
    addAndMakeVisible(editBtn.get());

    if (assignment != nullptr)
    {
        assignment->addAsyncContainerListener(this);
        if (assignment->characters != nullptr)
            assignment->characters->addAsyncContainerListener(this);
    }
}

DCAAssignmentEditor::~DCAAssignmentEditor()
{
    if (assignment != nullptr)
    {
        assignment->removeAsyncContainerListener(this);
        if (assignment->characters != nullptr)
            assignment->characters->removeAsyncContainerListener(this);
    }
}

void DCAAssignmentEditor::buttonClicked(Button* b)
{
    if (b == editBtn.get())
    {
        openCharacterDialog();
        return;
    }
    BaseItemEditor::buttonClicked(b);
}

void DCAAssignmentEditor::openCharacterDialog()
{
    if (assignment == nullptr) return;
    DCACue* cue = assignment->getParentCue();
    if (cue == nullptr) return;

    DialogWindow::LaunchOptions dw;
    dw.content.setOwned(new DCAAssignmentDialog(cue, assignment));
    dw.dialogTitle = "DCA " + String(assignment->dcaNumber->intValue()) + " — characters";
    dw.dialogBackgroundColour = BG_COLOR;
    dw.escapeKeyTriggersCloseButton = true;
    dw.useNativeTitleBar = false;
    dw.resizable = false;
    dw.launchAsync();
}

void DCAAssignmentEditor::newMessage(const ContainerAsyncEvent& /*e*/)
{
    repaint();
}

void DCAAssignmentEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    if (editBtn != nullptr)
        editBtn->setBounds(r.removeFromRight(140).reduced(2));
}
