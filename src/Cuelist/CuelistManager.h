/*
  ==============================================================================

    CuelistManager.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "Cuelist.h"

class CuelistManager :
    public BaseManager<Cuelist>
{
public:
    juce_DeclareSingleton(CuelistManager, true);

    CuelistManager();
    ~CuelistManager();
};
