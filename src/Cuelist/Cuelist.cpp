/*
  ==============================================================================

    Cuelist.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Cuelist.h"

Cuelist::Cuelist() :
    BaseItem("Cuelist"),
    cues()
{
    // cues = new CueManager();
    // cues.parentCuelist = this;

    askConfirmationBeforeRemove = true;
    setCanBeDisabled(false);
    setHasCustomColor(true);
	itemColor->setDefaultValue(BG_COLOR.brighter(.2f));

    goBtn = addTrigger("GO", "Trigger next cue");
    stopBtn = addTrigger("STOP", "Stop current cue");
}

Cuelist::~Cuelist()
{
    // if (cues) {
    //     delete cues;
    //     cues = nullptr;
    // }
}
