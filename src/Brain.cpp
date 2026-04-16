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
#include "ui/panels/ShowControl.h"

juce_ImplementSingleton(Brain);

void Brain::go()
{
    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr)
        cl->go();
}

void Brain::panic()
{
    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr) {
        cl->panic();
        ShowControl::getInstance()->startPanicking();
    }
}

void Brain::selectNextCue()
{
    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr) selectNextCueOn(cl);
}

void Brain::selectPreviousCue()
{
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
        if (!c->isAutoStartCue())
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
        if (!c->isAutoStartCue())
        {
            c->setGoNext();
            return;
        }
    }
}
