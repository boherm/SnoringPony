/*
  ==============================================================================

    OSCCommandEditor.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCCommandEditor.h"
#include "../OSCCommand.h"

OSCCommandEditor::OSCCommandEditor(Array<OSCCommand*> commands, bool isRoot):
	BaseItemEditor(Inspectable::getArrayAs<OSCCommand, BaseItem>(commands), isRoot)
{
    OSCCommand* c = commands.getFirst();
    testBtnUI = c->testBtn->createButtonUI();
    addAndMakeVisible(testBtnUI);
}

OSCCommandEditor::~OSCCommandEditor()
{
    delete testBtnUI;
}

void OSCCommandEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    testBtnUI->setBounds(r.removeFromRight(60).reduced(2));
}
