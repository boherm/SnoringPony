/*
  ==============================================================================

    CueManager.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "CueManager.h"

CueManager::CueManager() :
    BaseManager("Cues")
{
    itemDataType = "Cue";
    selectItemWhenCreated = false;
}

CueManager::~CueManager()
{
}

void CueManager::addItemInternal(Cue* c, var data) {
    float maxId = 0;
    for (Cue* cue : items) {
        if (c != cue) maxId = jmax(maxId, cue->id->floatValue());
    }
    if (maxId != 0 && c->id->floatValue() == 1) {
        c->id->setValue(floor(maxId+1));
    }
    //reorderItems();
    //correctCueIds();
    // parentCuelist->sendChangeMessage();
}

void CueManager::removeItemInternal(Cue* c)
{
}
