/*
  ==============================================================================

    CueManager.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "CueManager.h"
#include "audio/AudioCue.h"
#include "osc/OSCCue.h"
#include "playlist/PlaylistCue.h"
#include "fade/FadeCue.h"
#include "../ui/SPAssetManager.h"
#include "../Cue/Cue.h"
#include "../Cuelist/Cuelist.h"

CueManager::CueManager() :
    BaseManager<Cue>("Cues")
{
    // selectItemWhenCreated = true;
    // hideInEditor = true;

    managerFactory = &factory;
    factory.defs.add(Factory<Cue>::Definition::createDef("Audio", "Audio Cue", &AudioCue::create)->addIcon(SPAssetManager::getInstance()->getCueIcon("Audio")));
    factory.defs.add(Factory<Cue>::Definition::createDef("Audio", "Playlist Cue", &PlaylistCue::create)->addIcon(SPAssetManager::getInstance()->getCueIcon("Playlist")));
    factory.defs.add(Factory<Cue>::Definition::createDef("Playback", "Fade Cue", &FadeCue::create)->addIcon(SPAssetManager::getInstance()->getCueIcon("Fade")));
    factory.defs.add(Factory<Cue>::Definition::createDef("Network", "OSC Cue", &OSCCue::create)->addIcon(SPAssetManager::getInstance()->getCueIcon("OSC")));
}

CueManager::~CueManager()
{
}

void CueManager::addItemInternal(Cue* c, var data)
{
    c->parentCuelist = parentCuelist;

    if (data.hasProperty("id")) {
        c->id->setValue(data.getProperty("id", 1.0));
    } else {
        float maxId = 0;
        for (Cue* cue : items) {
            if (c != cue) maxId = jmax(maxId, cue->id->floatValue());
        }
        if (maxId != 0 && c->id->floatValue() == 1) {
            c->id->setValue(floor(maxId+1));
        }
    }

    // if not loading file, we set the first cue automatically to next cue to be fire
    if (!Engine::mainEngine->isLoadingFile) {
        if (items.size() == 1) {
            c->setGoNext();
        }
    }
}

void CueManager::askForRemoveBaseItem(BaseItem* item)
{
    Cue* itemCue = static_cast<Cue*>(item);
    Cue* nextCue = parentCuelist->nextCue->getTargetContainerAs<Cue>();

    if (nextCue == itemCue) {
        int idx = parentCuelist->cues->items.indexOf(itemCue);
        if (idx + 1 < parentCuelist->cues->items.size()) {
            Cue* c = parentCuelist->cues->items[idx + 1];
            c->setGoNext();
        } else {
            parentCuelist->nextCue->resetValue();
            parentCuelist->nextCue->notifyValueChanged();
        }
    }

    BaseManager<Cue>::askForRemoveBaseItem(item);
}

void CueManager::loadJSONDataManagerInternal(var data)
{
    BaseManager<Cue>::loadJSONDataManagerInternal(data);

    // If we have multiple cues with same ID, we set a warning on them
    Array<float> existingIds;
    for (Cue* c : items) {
        float idValue = c->id->floatValue();
        if (existingIds.contains(idValue)) {
            c->setWarningMessage("A cue with this ID already exists!");
        }
        existingIds.add(idValue);
    }
}

bool CueManager::hasCuePlaying()
{
    for (Cue* c : items) {
        if (c->isPlaying->boolValue()) {
            return true;
        }
    }
    return false;
}
