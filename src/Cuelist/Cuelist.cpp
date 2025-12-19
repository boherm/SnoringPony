/*
  ==============================================================================

    Cuelist.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Cuelist.h"
#include "CuelistManager.h"
#include "../Cue/CueManager.h"
#include "juce_organicui/inspectable/ui/InspectableEditor.h"
#include "ui/CuelistEditor.h"

Cuelist::Cuelist(var params) :
    BaseItem("Cuelist"),
    objectType(params.getProperty("type", "Cuelist").toString()),
	objectData(params)
{
    cues = new CueManager();
    cues->parentCuelist = this;
    cues->hideInEditor = true;
    cues->addAsyncContainerListener(this);

    saveAndLoadRecursiveData = true;

    askConfirmationBeforeRemove = true;
    setCanBeDisabled(false);
    setHasCustomColor(true);

    goBtn = addTrigger("GO", "Trigger next cue");
    goBtn->hideInEditor = true;
    stopBtn = addTrigger("STOP", "Stop all active cues in this cuelist");
    stopBtn->hideInEditor = true;

    itemColor->hideInEditor = false;
	itemColor->setDefaultValue(BG_COLOR.brighter(.2f));
    itemColor->setControlMode(Parameter::ControlMode::REFERENCE);

    isPlaying = addBoolParameter("Is Playing", "Indicates if a cue in this cuelist is currently playing", false, false);
    isPlaying->isSavable = false;

    isPanicking = addBoolParameter("Is Panicking", "Indicates if a cue in this cuelist is currently panicking", false, false);
    isPanicking->isSavable = false;
    isPanicking->alwaysNotify = true;
    isPanicking->hideInEditor = true;

    currentCue = addTargetParameter("Current cue", "Target the current cue", cues);
    currentCue->targetType = TargetParameter::CONTAINER;
    currentCue->maxDefaultSearchLevel = 0;
    currentCue->isSavable = false;
    currentCue->alwaysNotify = true;
    currentCue->setEnabled(false);

    nextCue = addTargetParameter("Next cue", "Target the next cue to play", cues);
    nextCue->targetType = TargetParameter::CONTAINER;
    nextCue->maxDefaultSearchLevel = 0;

    addChildControllableContainer(cues);
}

Cuelist::~Cuelist()
{
    cues->removeAsyncContainerListener(this);
    delete cues;
}

void Cuelist::go()
{
    Cue* nCue = nextCue->getTargetContainerAs<Cue>();

    if (nCue) {
        currentCue->setValueFromTarget(nCue);
        nCue->play();
    }
}

void Cuelist::stop()
{
    for (int i = 0; i < cues->items.size(); i++) {
        Cue* c = cues->items[i];
        if (c->isPlaying->boolValue()) {
            c->stop();
        }
    }
}

void Cuelist::panic()
{
    isPanicking->setValue(true);
    for (int i = 0; i < cues->items.size(); i++) {
        Cue* c = cues->items[i];
        if (c->isPlaying->boolValue() || c->preWaitActive->boolValue() || c->postWaitActive->boolValue()) {
            c->panic();
        }
    }
}

InspectableEditor* Cuelist::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new CuelistEditor(this, isRoot);
}

void Cuelist::parameterControlModeChanged(Parameter* p)
{
    if (p == itemColor && itemColor->controlMode == Parameter::ControlMode::REFERENCE)
    {
        itemColor->referenceTarget->setRootContainer(ProjectSettings::getInstance()->getControllableContainerByName("colorPresets"));
    }
}

void Cuelist::triggerTriggered(Trigger* t)
{
    if (t == goBtn)
        go();

    if (t == stopBtn)
        stop();
}

void Cuelist::loadJSONDataInternal(var data)
{
    // Set first cue to next cue at load
    Cue* firstCue = cues->items.getFirst();

    if (firstCue)
        firstCue->setGoNext();
}

void Cuelist::newMessage(const ContainerAsyncEvent& e)
{
    if (
            e.source == cues && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate &&
                (e.targetControllable->niceName == "Is Playing" || e.targetControllable->niceName == "Active")
        )
    {
        isPlaying->setValue(cues->hasCuePlaying());
        checkIfPanickingNeeded();
    }
}

void Cuelist::checkIfPanickingNeeded()
{
    if (!isPanicking->boolValue())
        return;

    bool inPanic = cues->hasCuePanickingPlaying();

    if (inPanic != isPanicking->boolValue()) {
        isPanicking->setValue(inPanic);
        isPanicking->notifyValueChanged();
    }
}
