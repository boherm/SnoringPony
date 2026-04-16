/*
  ==============================================================================

    SelectPreviousCueAction.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "SelectPreviousCueAction.h"
#include "../../Brain.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cuelist/Cuelist.h"

SelectPreviousCueAction::SelectPreviousCueAction() :
    MappingAction("Select Previous Cue")
{
    useMainCuelist = addBoolParameter("Use Main Cuelist", "When enabled, targets the main cuelist", true);
    targetCuelist = addTargetParameter("Target Cuelist", "Cuelist to select previous cue on", CuelistManager::getInstance());
    targetCuelist->targetType = TargetParameter::CONTAINER;
    targetCuelist->maxDefaultSearchLevel = 0;
    targetCuelist->hideInEditor = true;
}

SelectPreviousCueAction::~SelectPreviousCueAction() {}

void SelectPreviousCueAction::execute()
{
    if (useMainCuelist->boolValue())
    {
        Brain::getInstance()->selectPreviousCue();
    }
    else
    {
        Cuelist* cl = targetCuelist->getTargetContainerAs<Cuelist>();
        if (cl != nullptr) Brain::selectPreviousCueOn(cl);
    }
}

void SelectPreviousCueAction::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == useMainCuelist)
    {
        targetCuelist->hideInEditor = useMainCuelist->boolValue();
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}
