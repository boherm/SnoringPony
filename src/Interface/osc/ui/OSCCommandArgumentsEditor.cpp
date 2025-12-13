/*
  ==============================================================================

    OSCCommandArgumentsEditor.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCCommandArgumentsEditor.h"
#include "../OSCCommand.h"


OSCCommandArgumentsEditor::OSCCommandArgumentsEditor(GenericControllableManager* manager, bool isRoot) :
    GenericManagerEditor<GenericControllableItem>(manager, isRoot)
{
    OSCCommandArguments* argsManager = dynamic_cast<OSCCommandArguments*>(manager);
    editableUI = argsManager->editable->createToggle();
    addAndMakeVisible(editableUI);
}

OSCCommandArgumentsEditor::~OSCCommandArgumentsEditor()
{
    delete editableUI;
}

void OSCCommandArgumentsEditor::resizedInternalHeader(Rectangle<int>& r)
{
    GenericManagerEditor<GenericControllableItem>::resizedInternalHeader(r);
    r.removeFromRight(5);
    editableUI->setBounds(r.removeFromRight(70));
}
