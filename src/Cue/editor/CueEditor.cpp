/*
  ==============================================================================

    CueEditor.cpp
    Created: 06 Oct 2025 08:47:22pm
    Author:  boherm

  ==============================================================================
*/

#include "CueEditor.h"
#include "../Cue.h"

CueEditor::CueEditor(Array<Cue *> cues, bool isRoot) :
	BaseItemEditor(Inspectable::getArrayAs<Cue, BaseItem>(cues), isRoot)
{
    Cue* cue = cues.getFirst();

    cueTypeUI = std::make_unique<CueTypeUI>(cue);
    addAndMakeVisible(cueTypeUI.get());
    // idStepperUI.reset(cue->id->createLabelParameter());
    // idStepperUI->showLabel = false;
    // addAndMakeVisible(idStepperUI.get());
    // Logger::writeToLog("CueEditor::CueEditor: " + String(cue->niceName));
}

CueEditor::~CueEditor()
{
}

void CueEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    // idStepperUI->setBounds(r.removeFromRight(90).reduced(2));
    cueTypeUI->setBounds(r.removeFromRight(70).reduced(2));
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
    g.drawText(getCueTypeName(), getLocalBounds().reduced(4), Justification::centred, true);
}

String CueTypeUI::getCueTypeName()
{
    if (currentCue->getTypeString() == "MusicCue") {
        return "Music";
    } else {
        return "Cue";
    }
}
