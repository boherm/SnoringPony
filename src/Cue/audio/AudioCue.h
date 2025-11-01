/*
  ==============================================================================

    AudioCue.h
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class AudioCue :
    public Cue
{
public:
    AudioCue(var params = var());
    virtual ~AudioCue();

    FileParameter* audioFile;
    TargetParameter* targetAudioInterface;

    String getTypeString() const override { return "Audio"; }
    String getCueType() const override { return "Audio"; }
    static AudioCue* create(var params) { return new AudioCue(params); }

    void play();
};
