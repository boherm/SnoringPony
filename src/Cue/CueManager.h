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

    // Copy cues to the clipboard, expanding any group to include its sub-cues (so pasting a
    // group re-creates its members too). Kept in cuelist order; same clipboard format as the
    // app's global Copy so the regular paste path handles it.
    void copyCues(juce::Array<Cue*> cues);

    // Remove several cues (expanding any group to its sub-cues) as ONE undoable action.
    void removeCues(juce::Array<Cue*> cues);
    // The selection expanded to include the members of any group in it (deduped).
    juce::Array<Cue*> expandWithGroupMembers(const juce::Array<Cue*>& cues);
    // If the cuelist's next cue is in toRemove, advance it past the removed block.
    void fixNextCueBeforeRemoval(const juce::Array<Cue*>& toRemove);

    // Item creation from data (undo of a removal, load, ...): re-resolves group memberships.
    Cue* addItemFromData(juce::var data, bool addToUndo = true) override;
    juce::Array<Cue*> addItemsFromData(juce::var data, bool addToUndo = true) override;

    void loadJSONDataManagerInternal(var data) override;

    void createOneAudioCueFromFiles(const StringArray& files);
    void createMultipleAudioCueFromFiles(const StringArray& files);
    void createPlaylistCueFromFiles(const StringArray& files);
};
