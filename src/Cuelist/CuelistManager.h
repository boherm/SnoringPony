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

class Cue;

class CuelistManager :
    public BaseManager<Cuelist>
{
public:
    juce_DeclareSingleton(CuelistManager, true);

    CuelistManager();
    ~CuelistManager();

    PopupMenu getItemsMenuWithTickedItem(int startID, Cuelist* currentCuelist);

    static void showMenuForTargetCue(ControllableContainer* startFromCC, std::function<void(Cue*)> returnFunc);
};
