/*
  ==============================================================================

    OSCIsPlayingFeedback.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OSCIsPlayingFeedback.h"
#include "OSCInterface.h"
#include "../../Cuelist/Cuelist.h"

OSCIsPlayingFeedback::OSCIsPlayingFeedback() :
    OSCFeedbackItem("Is Playing")
{
    monitoredParameterName = "Is Playing";
    oscAddress->setValue("/cuelist/isPlaying");
}

void OSCIsPlayingFeedback::sendFeedback()
{
    if (oscInterface == nullptr || resolvedCuelist == nullptr) return;
    if (oscAddress->stringValue().isEmpty()) return; // empty OSC address would throw OSCFormatError
    int value = resolvedCuelist->isPlaying->boolValue() ? 1 : 0;
    oscInterface->sendOSC(OSCMessage(oscAddress->stringValue(), value));
}
