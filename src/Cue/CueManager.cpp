/*
  ==============================================================================

    CueManager.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "CueManager.h"
#include "audio/AudioCue.h"
#include "playlist/PlaylistCue.h"
#include "../Cue/Cue.h"
#include "../Cuelist/Cuelist.h"

CueManager::CueManager() :
    BaseManager<Cue>("Cues")
{
    managerFactory = &factory;
}

void CueManager::bindFactoryFromCuelist()
{
    factory.defs.clear();
    factory.menu.clear();
    if (parentCuelist != nullptr)
        parentCuelist->registerCueTypes(factory);
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

Array<Cue*> CueManager::addItemsFromClipboard(bool showWarning)
{
    // Which cue of this manager (if any) the paste anchors to. Mirrors the base logic: with
    // a selected cue the paste inserts right after it, otherwise it appends at the end.
    Cue* anchor = nullptr;
    if (auto* sm = InspectableSelectionManager::activeSelectionManager)
        anchor = sm->getInspectableAs<Cue>();
    if (anchor != nullptr && !items.contains(anchor)) anchor = nullptr;

    Array<Cue*> pasted = BaseManager<Cue>::addItemsFromClipboard(showWarning);
    if (pasted.isEmpty()) return pasted;

    // Work in integer hundredths to avoid float drift when stepping by 0.01.
    auto idHundredths = [](Cue* c) { return (int) std::lround(c->id->floatValue() * 100.0); };

    // Is `candidate` (in hundredths) already used by a cue that isn't part of this paste?
    auto isTaken = [&](int candidate)
    {
        for (auto* other : items)
            if (other != nullptr && !pasted.contains(other) && idHundredths(other) == candidate) return true;
        return false;
    };

    // Anchor present -> step by 0.01 after it. No selection (paste appended at end) -> step
    // by whole IDs (+1) from the highest existing ID.
    const int step = (anchor != nullptr) ? 1 : 100;

    int candidate;
    if (anchor != nullptr)
    {
        candidate = idHundredths(anchor);
    }
    else
    {
        int maxId = 0;
        for (auto* other : items)
            if (other != nullptr && !pasted.contains(other))
                maxId = jmax(maxId, (int) std::floor(other->id->floatValue()));
        candidate = maxId * 100;
    }

    for (auto* c : pasted)
    {
        candidate += step;
        while (isTaken(candidate)) candidate += step;

        c->id->setValue(candidate / 100.0f);
        c->id->notifyValueChanged();
    }

    refreshDuplicateIdWarnings();
    return pasted;
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
    refreshDuplicateIdWarnings();
}

void CueManager::refreshDuplicateIdWarnings()
{
    const String dupMsg = "A cue with this ID already exists!";

    for (int i = 0; i < items.size(); ++i)
    {
        Cue* c = items[i];

        bool duplicate = false;
        for (int j = 0; j < items.size(); ++j)
        {
            if (j != i && items[j]->id->floatValue() == c->id->floatValue())
            {
                duplicate = true;
                break;
            }
        }

        if (duplicate)
            c->setWarningMessage(dupMsg);
        else if (c->getWarningMessage() == dupMsg)
            c->clearWarning(); // only clear the duplicate-id warning, leave other warnings intact
    }
}

bool CueManager::hasCuePlaying()
{
    for (Cue* c : items) {
        if (c->isPlaying->boolValue() || c->preWaitActive->boolValue() || c->postWaitActive->boolValue()) {
            return true;
        }
    }
    return false;
}

bool CueManager::hasCuePanickingPlaying()
{
    for (Cue* c : items) {
        if (c->isPanicking && c->isPlaying->boolValue()) {
            return true;
        }
    }
    return false;
}


void CueManager::createOneAudioCueFromFiles(const StringArray& files)
{
    AudioCue* newCue = dynamic_cast<AudioCue*>(factory.create("Audio Cue"));
    if (newCue == nullptr) return;
    newCue->createFromFiles(files);
    addItem(newCue, var());
}

void CueManager::createMultipleAudioCueFromFiles(const StringArray& files)
{
    for (auto& file : files) {
        AudioCue* newCue = dynamic_cast<AudioCue*>(factory.create("Audio Cue"));
        if (newCue == nullptr) continue;
        newCue->createFromFiles(file);
        addItem(newCue, var());
    }
}

void CueManager::createPlaylistCueFromFiles(const StringArray& files)
{
    PlaylistCue* newCue = dynamic_cast<PlaylistCue*>(factory.create("Playlist Cue"));
    if (newCue == nullptr) return;
    newCue->createFromFiles(files);
    addItem(newCue, var());
}
