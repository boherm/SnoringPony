/*
  ==============================================================================

    CuelistEditor.cpp
    Created: 06 Nov 2025 06:02:00pm
    Author:  boherm

  ==============================================================================
*/

#include "CuelistEditor.h"
#include "../../Cuelist/Cuelist.h"

CuelistEditor::CuelistEditor(Array<Cuelist*> cuelists, bool isRoot):
	BaseItemEditor(Inspectable::getArrayAs<Cuelist, BaseItem>(cuelists), isRoot)
{
    Cuelist* cl = cuelists.getFirst();

    goBtnUI = cl->goBtn->createButtonUI();
    goBtnUI->customBGColor = Colours::darkgreen;
    goBtnUI->useCustomBGColor = true;

    stopBtnUI = cl->stopBtn->createButtonUI();
    stopBtnUI->customBGColor = Colours::darkred;
    stopBtnUI->useCustomBGColor = true;

    addAndMakeVisible(goBtnUI);
    addAndMakeVisible(stopBtnUI);
}

CuelistEditor::~CuelistEditor()
{
    delete goBtnUI;
    delete stopBtnUI;
}

void CuelistEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    stopBtnUI->setBounds(r.removeFromRight(60).reduced(2));
    goBtnUI->setBounds(r.removeFromRight(60).reduced(2));
}
