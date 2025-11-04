/*
  ==============================================================================

    CueEditor.cpp
    Created: 06 Oct 2025 08:47:22pm
    Author:  boherm

  ==============================================================================
*/

#include "CueEditor.h"
#include "../Cue.h"
#include "juce_graphics/juce_graphics.h"

CueEditor::CueEditor(Array<Cue *> cues, bool isRoot) :
	BaseItemEditor(Inspectable::getArrayAs<Cue, BaseItem>(cues), isRoot)
{
    Cue* cue = cues.getFirst();

    cueTypeUI = std::make_unique<CueTypeUI>(cue);
    addAndMakeVisible(cueTypeUI.get());

    goBtnUI = cue->goBtn->createButtonUI();
    goBtnUI->customBGColor = Colours::darkgreen;
    goBtnUI->useCustomBGColor = true;

    goNextBtnUI = cue->goNextBtn->createButtonUI();
    goNextBtnUI->customBGColor = Colours::darkorange.darker(0.3f);
    goNextBtnUI->useCustomBGColor = true;

    addAndMakeVisible(goBtnUI);
    addAndMakeVisible(goNextBtnUI);
}

CueEditor::~CueEditor()
{
    delete goBtnUI;
    delete goNextBtnUI;
}

void CueEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    cueTypeUI->setBounds(r.removeFromRight(90).reduced(2));

    goNextBtnUI->setBounds(r.removeFromRight(60).reduced(2));
    goBtnUI->setBounds(r.removeFromRight(60).reduced(2));


}

CueTypeUI::CueTypeUI(Cue* cue)
{
    currentCue = cue;
}

CueTypeUI::~CueTypeUI()
{
}

void CueTypeUI::paint(Graphics& g)
{
    g.setColour(BG_COLOR.darker(0.05f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
    g.setColour(Colours::white);
    g.setFont(Font(12, Font::bold));
    g.drawText(currentCue->getCueType(), getLocalBounds().reduced(4), Justification::centred, true);
}
