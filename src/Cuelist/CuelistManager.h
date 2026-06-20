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

    // Shared GO-target picker: lets the user choose a specific Cue (in any cuelist)
    // or a whole Cuelist (to GO its next cue). Used by GoCue and DCACue.
    static void showGoTargetMenu(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc);
    // Execute the GO action on a target container: play a Cue (and set it as its
    // cuelist's current cue), or trigger a Cuelist's GO (play its next cue).
    static void triggerGoTarget(ControllableContainer* container);

    void addItemInternal(Cuelist* cl, var data);
    void askForRemoveBaseItem(BaseItem* item);

    bool haveOnePlaying();
};
