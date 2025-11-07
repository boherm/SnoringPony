/*
  ==============================================================================

    CuelistManagerItemUI.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistManagerItemUI.h"
#include "../../Cue/CueManager.h"

CuelistManagerItemUI::CuelistManagerItemUI(Cuelist *item) :
  BaseItemUI(item)
{
    item->cues->addBaseManagerListener(this);
}

CuelistManagerItemUI::~CuelistManagerItemUI()
{
    item->cues->removeBaseManagerListener(this);
}

void CuelistManagerItemUI::paint(Graphics &g)
{
  BaseItemUI::paint(g);

  Rectangle<int> textArea = getLocalBounds();

  if (textArea.getWidth() > 200) {
    textArea.setHeight(textArea.getHeight() - 2);
    textArea.setWidth(textArea.getWidth() - 40);

    g.setColour(Colours::white);
    g.setFont(11.0f);
    g.setOpacity(0.5f);
    g.drawText((String)(item->cues->items.size()), textArea, Justification::centredRight);
  }
}
