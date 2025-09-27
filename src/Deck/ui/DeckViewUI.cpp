/*
  ==============================================================================

    DeckViewUI.cpp
    Created: 27 Sep 2025 05:23:00am
    Author:  boherm

  ==============================================================================
*/

#include "DeckViewUI.h"

DeckViewUI::DeckViewUI(const String &deckName) 
{
    this->deckName = deckName;
}

DeckViewUI::~DeckViewUI()
{
}

void DeckViewUI::paint (Graphics& g)
{
    g.setColour(Colours::white);
    g.drawText("DeckViewUI " + deckName, getLocalBounds(), Justification::centred, true);
}
