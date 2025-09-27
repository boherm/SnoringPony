/*
  ==============================================================================

    DecksSettings.cpp
    Created: 25 Sep 2025 01:30:00pm
    Author:  boherm

  ==============================================================================
*/

#include "DecksSettings.h"
#include "../Cuelist/CuelistManager.h"
#include "juce_organicui/controllable/parameter/TargetParameter.h"

juce_ImplementSingleton(DecksSettings)

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
    deckB = addTargetParameter("Deck B", "Cue list for deck B", CuelistManager::getInstance());
    deckB->targetType = TargetParameter::CONTAINER;
    deckC = addTargetParameter("Deck C", "Cue list for deck C", CuelistManager::getInstance());
    deckC->targetType = TargetParameter::CONTAINER;
    deckD = addTargetParameter("Deck D", "Cue list for deck D", CuelistManager::getInstance());
    deckD->targetType = TargetParameter::CONTAINER;
}

DecksSettings::~DecksSettings()
{
    delete deckA;
    delete deckB;
    delete deckC;
    delete deckD;
}

void DecksSettings::clear()
{
    deckA->resetValue();
    deckB->resetValue();
    deckC->resetValue();
    deckD->resetValue();
}
