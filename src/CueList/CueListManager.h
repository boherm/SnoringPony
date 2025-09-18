/*
  ==============================================================================

    CueListManager.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "CueList.h"

class CueListManager :
    public BaseManager<CueList>
{
public:
    juce_DeclareSingleton(CueListManager, true);

    CueListManager();
    ~CueListManager();
};
