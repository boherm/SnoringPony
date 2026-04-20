/*
  ==============================================================================

    MIDIFeedbackItem.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../feedback/FeedbackItem.h"

class MIDIInterface;

class MIDIFeedbackItem : public FeedbackItem
{
public:
    MIDIFeedbackItem(const String& name = "MIDI Feedback");
    virtual ~MIDIFeedbackItem();

    IntParameter* midiChannel;
    IntParameter* midiCC;

    MIDIInterface* midiInterface = nullptr;
    void setMIDIInterface(MIDIInterface* iface);
};
