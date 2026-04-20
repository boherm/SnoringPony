/*
  ==============================================================================

    OSCIsPanickingFeedback.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OSCIsPanickingFeedback.h"
#include "OSCInterface.h"
#include "../../Cuelist/Cuelist.h"

OSCIsPanickingFeedback::OSCIsPanickingFeedback() :
    OSCFeedbackItem("Is Panicking")
{
    monitoredParameterName = "Is Panicking";
    oscAddress->setValue("/cuelist/isPanicking");
}

void OSCIsPanickingFeedback::sendFeedback()
{
    if (oscInterface == nullptr || resolvedCuelist == nullptr) return;
    int value = resolvedCuelist->isPanicking->boolValue() ? 1 : 0;
    oscInterface->sendOSC(OSCMessage(oscAddress->stringValue(), value));
}
