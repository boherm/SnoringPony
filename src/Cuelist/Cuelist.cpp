/*
  ==============================================================================

    Cuelist.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Cuelist.h"
#include "../ProjectSettings/ColorPresets.h"
#include "CuelistManager.h"
#include "ui/CuelistEditor.h"

Cuelist::Cuelist(var params) :
    BaseItem("Cuelist"),
    objectType(params.getProperty("type", "Cuelist").toString()),
	objectData(params),
    cues()
{
    cues.parentCuelist = this;

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

    nextCue = addTargetParameter("Next cue", "Target the next cue to play", &cues);
    nextCue->targetType = TargetParameter::CONTAINER;
    nextCue->maxDefaultSearchLevel = 0;

    addChildControllableContainer(&cues);
}

Cuelist::~Cuelist()
{
}

void Cuelist::go()
{
    Cue* nCue = nextCue->getTargetContainerAs<Cue>();

    if (nCue) {
        nCue->play();

        int idx = cues.items.indexOf(nCue);

        if (cues.items.size() > idx + 1) {
            cues.items[idx + 1]->setGoNext();
        } else {
            nextCue->resetValue();
            nextCue->notifyValueChanged();
        }
    }
}

void Cuelist::stop()
{
    // todo: implement this!
}

CuelistEditor* Cuelist::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
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
    Cue* firstCue = cues.items.getFirst();

    if (firstCue)
        firstCue->setGoNext();
}
