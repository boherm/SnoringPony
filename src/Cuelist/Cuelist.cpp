/*
  ==============================================================================

    Cuelist.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Cuelist.h"

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
	itemColor->setDefaultValue(BG_COLOR.brighter(.2f));

    goBtn = addTrigger("GO", "Trigger next cue");
    stopBtn = addTrigger("STOP", "Stop current cue");

    addChildControllableContainer(&cues);
}

Cuelist::~Cuelist()
{
    // if (cues) {
    //     delete cues;
    //     cues = nullptr;
    // }
}

// juce::var Cuelist::getJSONData(bool includeNonOverriden)
// {
//     Logger::writeToLog("Cuelist::getJSONData");
//     juce::var v = BaseItem::getJSONData(includeNonOverriden);
//     v.getDynamicObject()->setProperty("cues", "toto2");
//     return v;
// }
