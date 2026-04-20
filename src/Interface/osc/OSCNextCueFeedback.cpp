/*
  ==============================================================================

    OSCNextCueFeedback.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OSCNextCueFeedback.h"
#include "OSCInterface.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cue/Cue.h"

OSCNextCueFeedback::OSCNextCueFeedback() :
    OSCFeedbackItem("Next Cue")
{
    monitoredParameterName = "Next cue";
    refreshOnDescriptionChange = true;
    oscAddress->setValue("/cuelist/nextCue");
}

void OSCNextCueFeedback::sendFeedback()
{
    if (oscInterface == nullptr || resolvedCuelist == nullptr) return;

    Cue* cue = resolvedCuelist->nextCue->getTargetContainerAs<Cue>();
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
