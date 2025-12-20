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

    bool hasCuePlaying();
    bool hasCuePanickingPlaying();

    void addItemInternal(Cue* c, var data);

    void askForRemoveBaseItem(BaseItem* item) override;

    void loadJSONDataManagerInternal(var data) override;

    void createOneAudioCueFromFiles(const StringArray& files);
    void createMultipleAudioCueFromFiles(const StringArray& files);
    void createPlaylistCueFromFiles(const StringArray& files);
};
