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

    // Pasted / duplicated cues keep everything but their ID: each gets the previous cue's
    // ID plus 0.01 steps (skipping IDs already taken), instead of copying the source ID.
    juce::Array<Cue*> addItemsFromClipboard(bool showWarning = true) override;

    void askForRemoveBaseItem(BaseItem* item) override;

    void loadJSONDataManagerInternal(var data) override;

    void createOneAudioCueFromFiles(const StringArray& files);
    void createMultipleAudioCueFromFiles(const StringArray& files);
    void createPlaylistCueFromFiles(const StringArray& files);
};
