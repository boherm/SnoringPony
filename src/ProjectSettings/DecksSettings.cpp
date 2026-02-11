/*
  ==============================================================================

    DecksSettings.cpp
    Created: 25 Sep 2025 01:30:00pm
    Author:  boherm

  ==============================================================================
*/

#include "DecksSettings.h"
#include "../Cuelist/CuelistManager.h"

DecksSettings::DecksSettings() :
    ControllableContainer("Decks Settings"),
    deckA(nullptr),
    deckB(nullptr),
    deckC(nullptr),
    deckD(nullptr)
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;

    deckA = addTargetParameter("Deck A", "Cue list for deck A", CuelistManager::getInstance());
    deckA->targetType = TargetParameter::CONTAINER;
    deckA->maxDefaultSearchLevel = 0;

    deckB = addTargetParameter("Deck B", "Cue list for deck B", CuelistManager::getInstance());
    deckB->targetType = TargetParameter::CONTAINER;
    deckB->maxDefaultSearchLevel = 0;

    deckC = addTargetParameter("Deck C", "Cue list for deck C", CuelistManager::getInstance());
    deckC->targetType = TargetParameter::CONTAINER;
    deckC->maxDefaultSearchLevel = 0;

    deckD = addTargetParameter("Deck D", "Cue list for deck D", CuelistManager::getInstance());
    deckD->targetType = TargetParameter::CONTAINER;
    deckD->maxDefaultSearchLevel = 0;
}

void DecksSettings::clear()
{
    deckA->resetValue();
    deckB->resetValue();
    deckC->resetValue();
    deckD->resetValue();
}
