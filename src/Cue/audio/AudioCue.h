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

class AudioFilesManager;
class AudioOutput;

class AudioCue :
    public Cue,
    public ContainerAsyncListener
{
public:
    AudioCue(var params = var());
    virtual ~AudioCue();

    AudioFilesManager* filesManager;
    AudioFormatManager formatManager;

    String getTypeString() const override { return "Audio"; }
    String getCueType() const override { return "Audio"; }
    static AudioCue* create(var params) { return new AudioCue(params); }

    void newMessage(const ContainerAsyncEvent& e) override;

    void play() override;
    void stop() override;
};
