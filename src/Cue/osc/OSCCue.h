/*
  ==============================================================================

    OSCCue.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class OSCCueMessage;

class OSCCue :
    public Cue,
    public BaseManager<OSCCueMessage>::ManagerListener
{
public:
    OSCCue(var params = var());
    virtual ~OSCCue();

    std::unique_ptr<BaseManager<OSCCueMessage>> messagesManager;

    String getTypeString() const override { return "OSC Cue"; }
    String getCueType() const override { return "OSC"; }
    static OSCCue* create(var params) { return new OSCCue(params); }

    void play() override;
    void stop() override;

    void itemAdded(OSCCueMessage*) override;
    void itemsAdded(juce::Array<OSCCueMessage *>) override;
};
