/*
  ==============================================================================

    CueListUI.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CueListUI.h"
#include "juce_graphics/juce_graphics.h"

CueListUI::CueListUI(CueList *item) : 
  BaseItemUI(item)
{

}

CueListUI::~CueListUI()
{
}

void CueListUI::paint(Graphics &g)
{
  BaseItemUI::paint(g);

  Rectangle<int> textArea = getLocalBounds();

  if (textArea.getWidth() > 200) {
    textArea.setHeight(textArea.getHeight() - 2);
    textArea.setWidth(textArea.getWidth() - 40);

    g.setColour(Colours::white);
    g.setFont(11.0f);
    g.setOpacity(0.5f);
    g.drawText("9999", textArea, Justification::centredRight);
  }
}