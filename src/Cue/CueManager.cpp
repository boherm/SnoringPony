/*
  ==============================================================================

    CueManager.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "CueManager.h"
#include "music/MusicCue.h"
#include "test/TestCue.h"
#include "../ui/SPAssetManager.h"

CueManager::CueManager() :
    BaseManager("Cues")
{
    // selectItemWhenCreated = true;
    // hideInEditor = true;

    managerFactory = &factory;
    factory.defs.add(Factory<Cue>::Definition::createDef("", "Music", &MusicCue::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("Audio")));
    factory.defs.add(Factory<Cue>::Definition::createDef("", "Test", &TestCue::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("test")));
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

    c->parentCuelist = parentCuelist;
}

void CueManager::loadJSONDataManagerInternal(var data) {
    // Logger::writeToLog("CueManager:loadJSONDataManagerInternal");
    BaseManager<Cue>::loadJSONDataManagerInternal(data);

    // If we have multiple cues with same ID
    Array<float> existingIds;
    for (Cue* c : items) {
        float idValue = c->id->floatValue();
        if (existingIds.contains(idValue)) {
            c->setWarningMessage("A cue with this ID already exists!");
        }
        existingIds.add(idValue);
    }
}
