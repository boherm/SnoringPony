/*
  ==============================================================================

    CueListManager.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CueListManager.h"
#include "CueListFactory.h"

CueListManager::CueListManager() :
    BaseManager<CueList>("Cue Lists")
{
    selectItemWhenCreated = true;
    managerFactory = CueListFactory::getInstance();
}

CueListManager::~CueListManager()
{
}

juce_ImplementSingleton(CueListManager);
