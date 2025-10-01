/*
  ==============================================================================

    Cue.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "Cue.h"

Cue::Cue(var params) : BaseItem(params.getProperty("name", "Cue 1"))
{
	canBeDisabled = false;
	saveAndLoadRecursiveData = true;
	editorIsCollapsed = false;
	itemDataType = "Cue";

    id = addFloatParameter("ID", "Id of this cue", 1, 0);
	description = addStringParameter("Description", "Description of the cue", "");
}

Cue::~Cue()
{
}
