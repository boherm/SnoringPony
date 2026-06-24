/*
  ==============================================================================

    SetMainCuelistCue.cpp
    Created: 24 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "SetMainCuelistCue.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cuelist/CuelistManager.h"
#include "../CueManager.h"
#include "../../PonyEngine.h"

SetMainCuelistCue::SetMainCuelistCue(var params) :
    Cue(params)
{
    itemDataType = "Set Main Cuelist Cue";
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    // This cue sets the next cue itself; don't let play() auto-advance it (which would
    // clobber our choice when the target cuelist is this cue's own cuelist).
    notSetNextCueAuto = true;

    targetCuelist = addTargetParameter("Target Cuelist",
                                       "Cuelist to set as the current main cuelist (does not change the startup main cuelist)",
                                       CuelistManager::getInstance());
    targetCuelist->targetType = TargetParameter::CONTAINER;
    targetCuelist->maxDefaultSearchLevel = 0;

    nextCue = addTargetParameter("Next Cue", "Optional: a cue of the target cuelist to set as its next cue",
                                 CuelistManager::getInstance());
    nextCue->targetType = TargetParameter::CONTAINER;
    // Only let the user pick among the cues of the currently targeted cuelist.
    nextCue->customGetTargetContainerFunc =
        [this](ControllableContainer*, std::function<void(ControllableContainer*)> returnFunc)
        {
            if (Cuelist* cl = targetCuelist->getTargetContainerAs<Cuelist>())
                CuelistManager::showMenuForTargetCue(cl->cues, [returnFunc](Cue* c) { returnFunc(c); });
        };
}

SetMainCuelistCue::~SetMainCuelistCue()
{
}

void SetMainCuelistCue::playInternal()
{
    if (auto* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine))
    {
        if (Cuelist* cl = targetCuelist->getTargetContainerAs<Cuelist>())
        {
            // Change only the current main cuelist, never the startup one.
            engine->showProperties.mainCuelist->setValueFromTarget(cl);
            engine->showProperties.mainCuelist->notifyValueChanged();
        }
    }

    // If a cue is selected, declare it as the next cue of its cuelist.
    Cue* c = nextCue->getTargetContainerAs<Cue>();
    if (c != nullptr && c->parentCuelist != nullptr)
    {
        c->parentCuelist->nextCue->setValueFromTarget(c);
        c->parentCuelist->nextCue->notifyValueChanged();
    }

    // notSetNextCueAuto is true, so play() never auto-advances this action's cuelist.
    // Do it ourselves *unless* the selected cue is in this very cuelist (in which case
    // it IS the next cue we just armed and auto-advancing would clobber it).
    bool armedOwnCuelist = (c != nullptr && c->parentCuelist == parentCuelist);
    if (!armedOwnCuelist)
        setNextCue();

    endCue();
}

void SetMainCuelistCue::onContainerParameterChangedInternal(Parameter* p)
{
    if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile) return;

    // The Next Cue picker is scoped to the target cuelist; if the target changes, the
    // previously chosen cue no longer belongs to it, so clear the selection.
    if (p == targetCuelist)
        nextCue->resetValue();
}

String SetMainCuelistCue::autoDescriptionInternal()
{
    String desc;
    if (Cuelist* cl = targetCuelist->getTargetContainerAs<Cuelist>())
        desc = "Set main cuelist -> " + cl->niceName;
    else
        desc = "Set Main Cuelist (no target!)";

    if (Cue* c = nextCue->getTargetContainerAs<Cue>())
        desc += " (next: " + c->id->stringValue() + ")";

    return desc;
}
