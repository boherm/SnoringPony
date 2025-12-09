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
    public ContainerAsyncListener,
    public Timer,
    public ChangeListener
{
public:
    AudioCue(var params = var());
    virtual ~AudioCue();

    AudioFilesManager* filesManager;
    AudioFormatManager formatManager;

    float savedRelativeGain = 1.0f;

    String getTypeString() const override { return "Audio Cue"; }
    String getCueType() const override { return "Audio"; }
    static AudioCue* create(var params) { return new AudioCue(params); }

    void newMessage(const ContainerAsyncEvent& e) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

    void play() override;
    void stop() override;
    void panic() override;

    void setRelativeGlobalGainForFade(float gain, float percent);
    void resetRelativeGlobalGainForFade();

private:
    void timerCallback() override;
};
