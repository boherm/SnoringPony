/*
  ==============================================================================

    FeedbackItem.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "FeedbackItem.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Brain.h"
#include "../../PonyEngine.h"

FeedbackItem::FeedbackItem(const String& name) :
    BaseItem(name)
{
    saveAndLoadRecursiveData = true;
    canBeDisabled = true;

    useMainCuelist = addBoolParameter("Use Main Cuelist", "When enabled, targets the main cuelist", true);
    targetCuelist = addTargetParameter("Target Cuelist", "Cuelist to monitor", CuelistManager::getInstance());
    targetCuelist->targetType = TargetParameter::CONTAINER;
    targetCuelist->maxDefaultSearchLevel = 0;
    targetCuelist->hideInEditor = true;
}

FeedbackItem::~FeedbackItem()
{
    unbindCuelist();
}

Cuelist* FeedbackItem::resolveCuelist()
{
    if (useMainCuelist->boolValue())
    {
        PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
        if (engine != nullptr)
            return engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    }
    else
    {
        return targetCuelist->getTargetContainerAs<Cuelist>();
    }
    return nullptr;
}

void FeedbackItem::bindCuelist()
{
    unbindCuelist();
    resolvedCuelist = resolveCuelist();
    if (resolvedCuelist != nullptr)
    {
        resolvedCuelist->addAsyncContainerListener(this);
        onCuelistBound();
    }
}

void FeedbackItem::unbindCuelist()
{
    if (resolvedCuelist != nullptr)
    {
        resolvedCuelist->removeAsyncContainerListener(this);
        onCuelistUnbound();
        resolvedCuelist = nullptr;
    }
}

void FeedbackItem::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == useMainCuelist)
    {
        targetCuelist->hideInEditor = useMainCuelist->boolValue();
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
        bindCuelist();
    }
    else if (p == targetCuelist)
    {
        bindCuelist();
    }
}

void FeedbackItem::newMessage(const ContainerAsyncEvent& e)
{
    if (resolvedCuelist == nullptr || !enabled->boolValue()) return;

    if (e.type == ContainerAsyncEvent::ControllableFeedbackUpdate)
    {
        if (monitoredParameterName.isNotEmpty() && e.targetControllable != nullptr
            && e.targetControllable->niceName == monitoredParameterName
            && e.targetControllable->parentContainer == resolvedCuelist)
        {
            sendFeedback();
        }

        // Refresh when a cue's Description changes (for current/next cue feedbacks)
        if (refreshOnDescriptionChange && e.targetControllable != nullptr
            && e.targetControllable->niceName == "Description")
        {
            sendFeedback();
        }
    }
}
