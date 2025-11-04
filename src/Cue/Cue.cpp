/*
  ==============================================================================

    Cue.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "Cue.h"
#include "../Cuelist/Cuelist.h"
#include "../PonyEngine.h"

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

    goBtn = addTrigger("Go", "Trigger this cue now");
    goBtn->hideInEditor = true;
    goNextBtn = addTrigger("Go Next", "Trigger this cue after next");
    goNextBtn->hideInEditor = true;

    id = addFloatParameter("ID", "ID of this cue", params.getProperty("id", 1.0));
    id->lockManualControlMode = true;

    description = addStringParameter("Description", "Description of the cue", params.getProperty("description", ""));
    description->lockManualControlMode = true;

	notes = addStringParameter("Notes", "Notes of the cue", params.getProperty("notes", ""));
    notes->lockManualControlMode = true;
    notes->multiline = true;

    // formatManager.registerBasicFormats();
    // deviceManager.initialise(2, 2, nullptr, true);
    // deviceManager.addAudioCallback(&sourcePlayer);

    // sourcePlayer.setSource(&transportSource);
}

Cue::~Cue()
{
    // transportSource.stop();
    // transportSource.setSource(nullptr);
    // deviceManager.removeAudioCallback(&sourcePlayer);
    // sourcePlayer.setSource(nullptr);
}

CueEditor* Cue::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new CueEditor(this, isRoot);
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

void Cue::setGoNext()
{
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.nextCueToGo->setTarget(this);
    engine->showProperties.nextCueToGo->notifyValueChanged();
}

void Cue::triggerTriggered(Trigger* t)
{
    if (t == goBtn) {
        play();
    }

    if (t == goNextBtn) {
        setGoNext();
    }
}
