/*
  ==============================================================================

    MusicCue.h
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class MusicCue :
    public Cue
{
public:
    MusicCue(var params = var());
    virtual ~MusicCue();

    FileParameter* audioFile;

    String getTypeString() const override { return "MusicCue"; }
    static MusicCue* create(var params) { return new MusicCue(params); }

    void play();
};
