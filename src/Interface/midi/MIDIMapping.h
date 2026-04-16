/*
  ==============================================================================

    MIDIMapping.h
    Created: 16 Apr 2026
    Author:  boherm

    Binds a MIDI event (CC, Note On/Off) to a list of MappingActions.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../action/MappingAction.h"

class MIDIMapping :
    public BaseItem
{
public:
    MIDIMapping(var params = var());
    virtual ~MIDIMapping();

    EnumParameter* eventType;
    IntParameter* channel;
    IntParameter* number;

    std::unique_ptr<BaseManager<MappingAction>> actions;

    bool matches(const juce::MidiMessage& msg) const;
    void executeActions();

    String getTypeString() const override { return "MIDI Mapping"; }
    static MIDIMapping* create(var params) { return new MIDIMapping(params); }
};
