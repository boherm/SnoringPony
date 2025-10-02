/*
  ==============================================================================

    CueManager.h
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "Cue.h"
#include "../MainIncludes.h"

class Cuelist;

class CueManager :
    public BaseManager<Cue>
{
public:
    CueManager();
    ~CueManager();

    Cuelist* parentCuelist;

    // void addItemInternal(Cue* c, var data);
    // void removeItemInternal(Cue*) override;
    // var getJSONData(bool includeNonOverriden = false) override;
};
