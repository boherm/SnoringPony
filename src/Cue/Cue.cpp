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

    id = addFloatParameter("ID", "Id of this cue", 1, 0);
	description = addStringParameter("Description", "Description of the cue", "");
}

Cue::~Cue()
{
}

// var Cue::getJSONData(bool includeNonOverriden)
// {
//     Logger::writeToLog("Cue::getJSONData");
//     var v = var(new DynamicObject());
//     v.getDynamicObject()->setProperty("id", id->floatValue());
//     v.getDynamicObject()->setProperty("description", description->stringValue());
//     return v;
//     // var v = BaseItem::getJSONData(includeNonOverriden);
//     // v.getDynamicObject()->setProperty("id", id->floatValue());
//     // v.getDynamicObject()->setProperty("description", description->stringValue());
//     // return v;
// }
