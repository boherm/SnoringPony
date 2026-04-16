/*
  ==============================================================================

    SelectNextCueAction.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "SelectNextCueAction.h"
#include "../../Brain.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cuelist/Cuelist.h"

SelectNextCueAction::SelectNextCueAction() :
    MappingAction("Select Next Cue")
{
    useMainCuelist = addBoolParameter("Use Main Cuelist", "When enabled, targets the main cuelist", true);
    targetCuelist = addTargetParameter("Target Cuelist", "Cuelist to select next cue on", CuelistManager::getInstance());
    targetCuelist->targetType = TargetParameter::CONTAINER;
    targetCuelist->maxDefaultSearchLevel = 0;
    targetCuelist->hideInEditor = true;
}

SelectNextCueAction::~SelectNextCueAction() {}

void SelectNextCueAction::execute()
{
    if (useMainCuelist->boolValue())
    {
        Brain::getInstance()->selectNextCue();
    }
    else
    {
        Cuelist* cl = targetCuelist->getTargetContainerAs<Cuelist>();
        if (cl != nullptr) Brain::selectNextCueOn(cl);
    }
}

void SelectNextCueAction::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == useMainCuelist)
    {
        targetCuelist->hideInEditor = useMainCuelist->boolValue();
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}
