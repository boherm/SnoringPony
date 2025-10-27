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

    // DBG("CueManager::addItemInternal, cue id: " << c->id->floatValue());
    //reorderItems();
    //correctCueIds();
    // parentCuelist->sendChangeMessage();
    // DBG(getJSONData().toString());
}

//void CueManager::removeItemInternal(Cue* c)
//{
//    DBG("CueManager::removeItemInternal, cue id: " << c->id->floatValue());
//}

// var CueManager::getJSONData(bool includeNonOverriden)
// {
//     Logger::writeToLog("CueManager::getJSONData");
//     var v = BaseManager<Cue>::getJSONData(includeNonOverriden);

//     Array<var> cuesArray;

//     for (Cue* c : items) {
//         cuesArray.add(c->getJSONData(includeNonOverriden));
//     }

//     v.getDynamicObject()->setProperty("cues", cuesArray);

//     return v;
// }
