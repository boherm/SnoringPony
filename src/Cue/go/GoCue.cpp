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

GoCue::GoCue(var params) :
    Cue(params)
{
    itemDataType = "Go Cue";
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    targetCue = addTargetParameter("Target Cue", "Cue to GO (plays the cue and advances its cuelist)",
                                   CuelistManager::getInstance());
    targetCue->targetType = TargetParameter::CONTAINER;
    targetCue->customGetTargetContainerFunc =
        [](ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc)
        {
            CuelistManager::showMenuForTargetCue(startFromCC, [returnFunc](Cue* c) { returnFunc(c); });
        };
}

GoCue::~GoCue()
{
}

void GoCue::playInternal()
{
    Cue* target = targetCue->getTargetContainerAs<Cue>();
    if (target != nullptr && target->parentCuelist != nullptr)
    {
        target->parentCuelist->currentCue->setValueFromTarget(target);
        target->play();
    }
    endCue();
}

String GoCue::autoDescriptionInternal()
{
    Cue* target = targetCue->getTargetContainerAs<Cue>();
    if (target == nullptr) return "Go Cue (no target)";
    String cuelistName = target->parentCuelist != nullptr ? target->parentCuelist->niceName : String("?");
    return "GO " + cuelistName + " / " + target->id->stringValue();
}
