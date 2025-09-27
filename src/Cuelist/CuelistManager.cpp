/*
  ==============================================================================

    CuelistManager.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistManager.h"
#include "CuelistFactory.h"

CuelistManager::CuelistManager() :
    BaseManager<Cuelist>("Cuelists")
{
    selectItemWhenCreated = true;
    managerFactory = CuelistFactory::getInstance();
}

CuelistManager::~CuelistManager()
{
}

juce_ImplementSingleton(CuelistManager);
