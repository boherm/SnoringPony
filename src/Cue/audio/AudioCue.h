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
#include "juce_organicui/controllable/parameter/BoolParameter.h"

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

    bool askedToStop = false;

    BoolParameter* loop;

    String getTypeString() const override { return "Audio Cue"; }
    String getCueType() const override { return "Audio"; }
    static AudioCue* create(var params) { return new AudioCue(params); }

    void newMessage(const ContainerAsyncEvent& e) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

    void playInternal() override;
    void stopInternal() override;
    void panicInternal() override;

    void fade(double targetGain, double duration) override;

    String autoDescriptionInternal() override;

    void createFromFiles(const StringArray& files);

private:
    void timerCallback() override;
};
