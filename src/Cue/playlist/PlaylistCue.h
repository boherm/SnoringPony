/*
  ==============================================================================

    PlaylistCue.h
    Created: 3 Dec 2025 06:30:22pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class PlaylistCue :
    public Cue
{
public:
    PlaylistCue(var params = var());
    virtual ~PlaylistCue();

    String getTypeString() const override { return "Playlist Cue"; }
    String getCueType() const override { return "Playlist"; }
    static PlaylistCue* create(var params) { return new PlaylistCue(params); }

    void play() override;
    void stop() override;
};
