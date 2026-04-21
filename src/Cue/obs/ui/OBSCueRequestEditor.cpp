/*
  ==============================================================================

    OBSCueRequestEditor.cpp
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OBSCueRequestEditor.h"
#include "../OBSCueRequest.h"

OBSCueRequestEditor::OBSCueRequestEditor(Array<OBSCueRequest*> requests, bool isRoot) :
    BaseItemEditor(Inspectable::getArrayAs<OBSCueRequest, BaseItem>(requests), isRoot)
{
    OBSCueRequest* request = requests.getFirst();

    targetUI = request->targetTemplate->createTargetUI();
    testBtnUI = request->testBtn->createButtonUI();
    addAndMakeVisible(targetUI);
    addAndMakeVisible(testBtnUI);
}

OBSCueRequestEditor::~OBSCueRequestEditor()
{
    delete targetUI;
    delete testBtnUI;
}

void OBSCueRequestEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    testBtnUI->setBounds(r.removeFromRight(60).reduced(2));
    targetUI->setBounds(r.removeFromRight(250).reduced(2));
}
