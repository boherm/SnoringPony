/*
  ==============================================================================

    Cue.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "Cue.h"

Cue::Cue(var params) :
    BaseItem(params.getProperty("name", "Cue 1")),
    objectType(params.getProperty("type", "Cue").toString()),
	objectData(params)

{
	canBeDisabled = false;
	editorIsCollapsed = false;
	itemDataType = "Cue";

    id = addFloatParameter("ID", "ID of this cue", params.getProperty("id", 1.0));
    id->lockManualControlMode = true;

	notes = addStringParameter("Notes", "Notes of the cue", params.getProperty("notes", ""));
    notes->lockManualControlMode = true;
    notes->multiline = true;

    formatManager.registerBasicFormats();
    deviceManager.initialise(2, 2, nullptr, true);
    deviceManager.addAudioCallback(&sourcePlayer);

    sourcePlayer.setSource(&transportSource);
}

Cue::~Cue()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    deviceManager.removeAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(nullptr);
}

CueEditor* Cue::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new CueEditor(this, isRoot);
}

void Cue::play()
{
}
