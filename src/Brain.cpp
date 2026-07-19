/*
  ==============================================================================

    Brain.cpp
    Created: 6 Nov 2025 08:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Brain.h"
#include "PonyEngine.h"
#include "Cue/Cue.h"
#include "Cue/CueManager.h"
#include "Cuelist/Cuelist.h"
#include "Cuelist/CuelistManager.h"
#include "Redundancy/RedundancyManager.h"
#include "ui/panels/ShowControl.h"

juce_ImplementSingleton(Brain);

// While this instance is a Backup in standby, it is not the playback authority: every local trigger
// (GO / panic / cue selection, from OSC, MIDI or keyboard) must be ignored so it doesn't drive the
// show in parallel with the Main. The Backup only mirrors state (applied out-of-band, not via Brain).
static bool redundancySuppressesTriggers()
{
    if (auto* rm = RedundancyManager::getInstanceWithoutCreating())
        return rm->isTriggeringSuppressed();
    return false;
}

void Brain::go()
{
    if (redundancySuppressesTriggers()) return;

    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr)
        cl->go();
}

void Brain::panic()
{
    if (redundancySuppressesTriggers()) return;

    const MessageManagerLock mmLock;

    // Emergency stop applies to EVERY cuelist, not just the main one.
    for (auto* cl : CuelistManager::getInstance()->items)
        if (cl != nullptr) cl->panic();

    ShowControl::getInstance()->startPanicking();
}

void Brain::selectNextCue()
{
    if (redundancySuppressesTriggers()) return;

    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr) selectNextCueOn(cl);
}

void Brain::selectPreviousCue()
{
    if (redundancySuppressesTriggers()) return;

    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr) selectPreviousCueOn(cl);
}

void Brain::selectNextCueOn(Cuelist* cl)
{
    if (cl == nullptr || cl->cues == nullptr) return;

    Cue* current = cl->nextCue->getTargetContainerAs<Cue>();
    int idx = (current != nullptr) ? cl->cues->items.indexOf(current) : -1;

    for (int i = idx + 1; i < cl->cues->items.size(); ++i)
    {
        Cue* c = cl->cues->items[i];
        if (!c->isAutoStartCue() && c->canBeNextCueAuto())
        {
            c->setGoNext();
            return;
        }
    }
}

void Brain::selectPreviousCueOn(Cuelist* cl)
{
    if (cl == nullptr || cl->cues == nullptr) return;

    Cue* current = cl->nextCue->getTargetContainerAs<Cue>();
    int idx = (current != nullptr) ? cl->cues->items.indexOf(current) : cl->cues->items.size();

    for (int i = idx - 1; i >= 0; --i)
    {
        Cue* c = cl->cues->items[i];
        if (!c->isAutoStartCue() && c->canBeNextCueAuto())
        {
            c->setGoNext();
            return;
        }
    }
}
