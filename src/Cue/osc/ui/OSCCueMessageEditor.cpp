/*
  ==============================================================================

    OSCCueMessageEditor.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCCueMessageEditor.h"
#include "../OSCCueMessage.h"

OSCCueMessageEditor::OSCCueMessageEditor(Array<OSCCueMessage *> messages, bool isRoot) :
	BaseItemEditor(Inspectable::getArrayAs<OSCCueMessage, BaseItem>(messages), isRoot)
{
    OSCCueMessage* message = messages.getFirst();

    targetUI = message->targetTemplate->createTargetUI();
    testBtnUI = message->testBtn->createButtonUI();
    addAndMakeVisible(targetUI);
    addAndMakeVisible(testBtnUI);
}

OSCCueMessageEditor::~OSCCueMessageEditor()
{
    delete targetUI;
    delete testBtnUI;
}

void OSCCueMessageEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    testBtnUI->setBounds(r.removeFromRight(60).reduced(2));
    targetUI->setBounds(r.removeFromRight(250).reduced(2));
}
