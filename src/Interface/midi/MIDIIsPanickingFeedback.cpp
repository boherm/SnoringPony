/*
  ==============================================================================

    MIDIIsPanickingFeedback.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MIDIIsPanickingFeedback.h"
#include "MIDIInterface.h"
#include "../../Cuelist/Cuelist.h"

MIDIIsPanickingFeedback::MIDIIsPanickingFeedback() :
    MIDIFeedbackItem("Is Panicking")
{
    monitoredParameterName = "Is Panicking";
}

void MIDIIsPanickingFeedback::sendFeedback()
{
    if (midiInterface == nullptr || resolvedCuelist == nullptr) return;
    int value = resolvedCuelist->isPanicking->boolValue() ? 127 : 0;
    midiInterface->sendMessage(juce::MidiMessage::controllerEvent(midiChannel->intValue(), midiCC->intValue(), value));
}
