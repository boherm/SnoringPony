/*
  ==============================================================================

    MIDIFeedbackItem.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MIDIFeedbackItem.h"
#include "MIDIInterface.h"

MIDIFeedbackItem::MIDIFeedbackItem(const String& name) :
    FeedbackItem(name)
{
    midiChannel = addIntParameter("MIDI Channel", "MIDI channel for feedback", 1, 1, 16);
    midiCC = addIntParameter("CC Number", "Control Change number", 0, 0, 127);
}

MIDIFeedbackItem::~MIDIFeedbackItem()
{
}

void MIDIFeedbackItem::setMIDIInterface(MIDIInterface* iface)
{
    midiInterface = iface;
    bindCuelist();
}
