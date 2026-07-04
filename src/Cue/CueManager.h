/*
  ==============================================================================

    CueManager.h
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "Cue.h"
#include "../MainIncludes.h"

class Cuelist;
class GroupCue;

class CueManager :
    public BaseManager<Cue>
{
public:
    CueManager();
    ~CueManager();

    Factory<Cue> factory;

    Cuelist* parentCuelist;

    // While true (e.g. during an id reorder), per-cue duplicate-id checks are skipped;
    // a single full-cuelist check is run once the operation finishes.
    bool isReordering = false;

    // Scan the whole cuelist and set/clear the "duplicate id" warning on each cue.
    void refreshDuplicateIdWarnings();

    void bindFactoryFromCuelist();

    bool hasCuePlaying();
    bool hasCuePanickingPlaying();

    void addItemInternal(Cue* c, var data);

    // --- Group membership ---------------------------------------------------
    // Attach `cue` to `group`: mark it, and (when reposition) move it contiguously right
    // after the group's current members. No-op if cue is the group or already a GroupCue.
    // Drag-drop passes reposition=false since it already placed the cue at the drop spot.
    void addToGroup(Cue* cue, GroupCue* group, bool reposition = true);
    // Detach `cue` from its group (leaves it in place, now ungrouped).
    void removeFromGroup(Cue* cue);
    // The group owning `uid`, or nullptr.
    GroupCue* findGroupByUID(const juce::String& uid);
    // After load: turn each cue's saved parentGroupUID back into a parentGroup pointer.
    void resolveGroupMemberships();

    // Pasted / duplicated cues keep everything but their ID: each gets the previous cue's
    // ID plus 0.01 steps (skipping IDs already taken), instead of copying the source ID.
    juce::Array<Cue*> addItemsFromClipboard(bool showWarning = true) override;

    void askForRemoveBaseItem(BaseItem* item) override;

    void loadJSONDataManagerInternal(var data) override;

    void createOneAudioCueFromFiles(const StringArray& files);
    void createMultipleAudioCueFromFiles(const StringArray& files);
    void createPlaylistCueFromFiles(const StringArray& files);
};
