/*
  ==============================================================================

    OSCCurrentCueFeedback.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OSCCurrentCueFeedback.h"
#include "OSCInterface.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cue/Cue.h"

OSCCurrentCueFeedback::OSCCurrentCueFeedback() :
    OSCFeedbackItem("Current Cue")
{
    monitoredParameterName = "Current cue";
    refreshOnDescriptionChange = true;
    oscAddress->setValue("/cuelist/currentCue");
}

void OSCCurrentCueFeedback::sendFeedback()
{
    if (oscInterface == nullptr || resolvedCuelist == nullptr) return;

    Cue* cue = resolvedCuelist->currentCue->getTargetContainerAs<Cue>();
    OSCMessage msg(oscAddress->stringValue());

    if (cue != nullptr)
    {
        msg.addFloat32((float)cue->id->doubleValue());
        msg.addString(cue->getDescription());
    }
    else
    {
        msg.addFloat32(0.0f);
        msg.addString("");
    }

    oscInterface->sendOSC(msg);
}
