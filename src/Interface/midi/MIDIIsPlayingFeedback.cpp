/*
  ==============================================================================

    MIDIIsPlayingFeedback.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MIDIIsPlayingFeedback.h"
#include "MIDIInterface.h"
#include "../../Cuelist/Cuelist.h"

MIDIIsPlayingFeedback::MIDIIsPlayingFeedback() :
    MIDIFeedbackItem("Is Playing")
{
    monitoredParameterName = "Is Playing";
}

void MIDIIsPlayingFeedback::sendFeedback()
{
    if (midiInterface == nullptr || resolvedCuelist == nullptr) return;
    int value = resolvedCuelist->isPlaying->boolValue() ? 127 : 0;
    midiInterface->sendMessage(juce::MidiMessage::controllerEvent(midiChannel->intValue(), midiCC->intValue(), value));
}
