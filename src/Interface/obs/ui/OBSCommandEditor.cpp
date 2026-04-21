/*
  ==============================================================================

    OBSCommandEditor.cpp
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OBSCommandEditor.h"
#include "../OBSCommand.h"

OBSCommandEditor::OBSCommandEditor(Array<OBSCommand*> commands, bool isRoot) :
    BaseItemEditor(Inspectable::getArrayAs<OBSCommand, BaseItem>(commands), isRoot)
{
    OBSCommand* c = commands.getFirst();
    testBtnUI = c->testBtn->createButtonUI();
    editableUI = c->editable->createToggle();
    addAndMakeVisible(testBtnUI);
    addAndMakeVisible(editableUI);
}

OBSCommandEditor::~OBSCommandEditor()
{
    delete testBtnUI;
    delete editableUI;
}

void OBSCommandEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    testBtnUI->setBounds(r.removeFromRight(60).reduced(2));
    r.removeFromRight(5);
    editableUI->setBounds(r.removeFromRight(70).reduced(2));
}
