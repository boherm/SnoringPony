/*
  ==============================================================================

    GoAction.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "GoAction.h"
#include "../../Brain.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cuelist/Cuelist.h"

GoAction::GoAction() :
    MappingAction("Go")
{
    useMainCuelist = addBoolParameter("Use Main Cuelist", "When enabled, targets the main cuelist", true);
    targetCuelist = addTargetParameter("Target Cuelist", "Cuelist to trigger Go on", CuelistManager::getInstance());
    targetCuelist->targetType = TargetParameter::CONTAINER;
    targetCuelist->maxDefaultSearchLevel = 0;
    targetCuelist->hideInEditor = true;
}

GoAction::~GoAction() {}

void GoAction::execute()
{
    if (useMainCuelist->boolValue())
    {
        Brain::getInstance()->go();
    }
    else
    {
        Cuelist* cl = targetCuelist->getTargetContainerAs<Cuelist>();
        if (cl != nullptr) cl->go();
    }
}

void GoAction::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == useMainCuelist)
    {
        targetCuelist->hideInEditor = useMainCuelist->boolValue();
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}
