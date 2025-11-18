/*
  ==============================================================================

    Cue.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "Cue.h"
#include "ui/CueEditor.h"
#include "../Cuelist/Cuelist.h"

Cue::Cue(var params) :
    BaseItem(params.getProperty("name", "1.000")),
    objectType(params.getProperty("type", "Cue").toString()),
	objectData(params)

{
    saveAndLoadRecursiveData = true;
    nameCanBeChangedByUser = false;
	canBeDisabled = false;
	editorIsCollapsed = false;
	itemDataType = "Cue";
    setHasCustomColor(true);
    itemColor->hideInEditor = false;
	itemColor->setDefaultValue(BG_COLOR);
    itemColor->setControlMode(Parameter::ControlMode::REFERENCE);

    isPlaying = addBoolParameter("Is Playing", "Indicates if this cue is currently playing", false, false);
    isPlaying->isSavable = false;

    setNextBtn = addTrigger("Set next", "Trigger this cue after next");
    setNextBtn->hideInEditor = true;

    id = addFloatParameter("ID", "ID of this cue", params.getProperty("id", 1.0));
    id->lockManualControlMode = true;

    currentTime = addFloatParameter("Current time", "Current time of the cue", 0.0);
    currentTime->lockManualControlMode = true;
    currentTime->hideInRemoteControl = true;
    currentTime->defaultUI = FloatParameter::TIME;
    currentTime->setEnabled(false);

    duration = addFloatParameter("Duration", "Duration of the cue", params.getProperty("duration", 0.0));
    duration->lockManualControlMode = true;
    duration->defaultUI = FloatParameter::TIME;

    description = addStringParameter("Description", "Description of the cue", params.getProperty("description", ""));
    description->lockManualControlMode = true;

	notes = addStringParameter("Notes", "Notes of the cue", params.getProperty("notes", ""));
    notes->lockManualControlMode = true;
    notes->multiline = true;
}

Cue::~Cue()
{
}

InspectableEditor* Cue::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new CueEditor(this, isRoot);
}

void Cue::setGoNext()
{
    parentCuelist->nextCue->setTarget(this);
    parentCuelist->nextCue->notifyValueChanged();
}

void Cue::parameterValueChanged(Parameter* p)
{
    ControllableContainer::parameterValueChanged(p);

    if (p == id) {
        clearWarning();
        setNiceName(id->stringValue());
        setCustomShortName(id->stringValue());

        if (id->stringValue() != niceName) {
            setWarningMessage("A cue with this ID already exists!");
        }
    }
}

void Cue::parameterControlModeChanged(Parameter* p)
{
    if (p == itemColor && itemColor->controlMode == Parameter::ControlMode::REFERENCE)
    {
        itemColor->referenceTarget->setRootContainer(ProjectSettings::getInstance()->getControllableContainerByName("colorPresets"));
    }
}

void Cue::triggerTriggered(Trigger* t)
{
    if (t == setNextBtn) {
        setGoNext();
    }
}
