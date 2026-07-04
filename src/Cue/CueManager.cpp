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
#include "group/GroupCue.h"
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
    // (unless it's a note, which is never auto-selected)
    if (!Engine::mainEngine->isLoadingFile) {
        if (items.size() == 1 && c->canBeNextCueAuto()) {
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

void CueManager::addToGroup(Cue* cue, GroupCue* group, bool reposition)
{
    if (cue == nullptr || group == nullptr || cue == group)
        return;
    // v1: no nested groups — a group cannot be a member of another group.
    if (dynamic_cast<GroupCue*>(cue) != nullptr)
        return;

    cue->parentGroup = group;
    cue->parentGroupUID->setValue(group->groupUID->stringValue());
    cue->notSetNextCueAuto = true;
    cue->applyGroupMembershipConstraints();

    // Keep members contiguous: move the cue right after the group's last member. Skipped
    // for drag-drop, which already dropped the cue at the intended (contiguous) position.
    if (reposition)
    {
        int groupIdx = items.indexOf(group);
        if (groupIdx >= 0)
        {
            int lastMemberIdx = groupIdx;
            for (int i = groupIdx + 1; i < items.size(); i++)
            {
                Cue* c = items[i];
                if (c == cue) continue;
                if (c->parentGroup == group) lastMemberIdx = i;
            }

            int target = lastMemberIdx + 1;
            int currentIdx = items.indexOf(cue);
            if (currentIdx >= 0 && currentIdx < target) target--; // account for the removal shift
            target = jlimit(0, items.size() - 1, target);
            if (currentIdx != target)
                setItemIndex(cue, target, true);
        }
    }

    // A freshly grouped cue can no longer be a next cue: hand the pointer to the group.
    if (parentCuelist != nullptr && parentCuelist->nextCue->getTargetContainerAs<Cue>() == cue)
        group->setGoNext();

    group->refreshDuration();
}

void CueManager::removeFromGroup(Cue* cue)
{
    if (cue == nullptr) return;
    GroupCue* g = cue->parentGroup;
    cue->parentGroup = nullptr;
    cue->parentGroupUID->setValue("");
    cue->notSetNextCueAuto = false;
    cue->applyGroupMembershipConstraints(); // ungrouped: restore post-wait availability
    if (g != nullptr) g->refreshDuration();
}

GroupCue* CueManager::findGroupByUID(const String& uid)
{
    if (uid.isEmpty()) return nullptr;
    for (auto* c : items)
        if (auto* g = dynamic_cast<GroupCue*>(c))
            if (g->groupUID->stringValue() == uid) return g;
    return nullptr;
}

void CueManager::resolveGroupMemberships()
{
    for (auto* c : items)
    {
        if (c == nullptr) continue;
        GroupCue* g = findGroupByUID(c->parentGroupUID->stringValue());
        c->parentGroup = g;
        c->notSetNextCueAuto = (g != nullptr);
        c->applyGroupMembershipConstraints();
    }

    // Now that memberships are known, seed each group's derived duration.
    for (auto* c : items)
        if (auto* g = dynamic_cast<GroupCue*>(c))
            g->refreshDuration();
}

void CueManager::askForRemoveBaseItem(BaseItem* item)
{
    Cue* itemCue = static_cast<Cue*>(item);

    // Deleting a group ungroups its members (they stay in the cuelist), rather than
    // deleting them along with it.
    if (auto* g = dynamic_cast<GroupCue*>(itemCue))
        for (auto* m : g->getMembers())
            removeFromGroup(m);

    Cue* nextCue = parentCuelist->nextCue->getTargetContainerAs<Cue>();

    if (nextCue == itemCue) {
        int idx = parentCuelist->cues->items.indexOf(itemCue);
        Cue* replacement = nullptr;
        for (int i = idx + 1; i < parentCuelist->cues->items.size(); i++) {
            Cue* c = parentCuelist->cues->items[i];
            if (c->canBeNextCueAuto()) { replacement = c; break; }
        }
        if (replacement != nullptr) {
            replacement->setGoNext();
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

    // Re-attach grouped sub-cues to their group now that every cue exists.
    resolveGroupMemberships();

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
