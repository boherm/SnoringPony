/*
  ==============================================================================

    PanicAction.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "PanicAction.h"
#include "../../Brain.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cuelist/Cuelist.h"
#include "../../ui/panels/ShowControl.h"

PanicAction::PanicAction() :
    MappingAction("Panic")
{
    useMainCuelist = addBoolParameter("Use Main Cuelist", "When enabled, targets the main cuelist", true);
    targetCuelist = addTargetParameter("Target Cuelist", "Cuelist to panic", CuelistManager::getInstance());
    targetCuelist->targetType = TargetParameter::CONTAINER;
    targetCuelist->maxDefaultSearchLevel = 0;
    targetCuelist->hideInEditor = true;
}

PanicAction::~PanicAction() {}

void PanicAction::execute()
{
    if (useMainCuelist->boolValue())
    {
        Brain::getInstance()->panic();
    }
    else
    {
        Cuelist* cl = targetCuelist->getTargetContainerAs<Cuelist>();
        if (cl != nullptr)
        {
            cl->panic();
            ShowControl::getInstance()->startPanicking();
        }
    }
}

void PanicAction::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == useMainCuelist)
    {
        targetCuelist->hideInEditor = useMainCuelist->boolValue();
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}
