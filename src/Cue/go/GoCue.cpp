/*
  ==============================================================================

    GoCue.cpp
    Created: 15 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "GoCue.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cuelist/CuelistManager.h"
#include "../CueManager.h"

GoCue::GoCue(var params) :
    Cue(params)
{
    itemDataType = "Go Cue";
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    targetCue = addTargetParameter("Target", "Cue to GO, or cuelist to trigger its next cue",
                                   CuelistManager::getInstance());
    targetCue->targetType = TargetParameter::CONTAINER;
    targetCue->customGetTargetContainerFunc = &CuelistManager::showGoTargetMenu;
}

GoCue::~GoCue()
{
}

void GoCue::playInternal()
{
    CuelistManager::triggerGoTarget(targetCue->getTargetContainer());
    endCue();
}

String GoCue::autoDescriptionInternal()
{
    ControllableContainer* container = targetCue->getTargetContainer();
    if (auto* target = dynamic_cast<Cue*>(container))
    {
        if (target == this)
            return "Go Cue (self-reference!)";
        String cuelistName = target->parentCuelist != nullptr ? target->parentCuelist->niceName : String("?");
        return "GO " + cuelistName + " / " + target->id->stringValue() + " - " + target->getDescription();
    }
    if (auto* cl = dynamic_cast<Cuelist*>(container))
    {
        return "GO " + cl->niceName;
    }
    return "Go Cue (no target!)";
}
