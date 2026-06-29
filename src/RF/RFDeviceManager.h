/*
  ==============================================================================

    RFDeviceManager.h
    Created: 26 Jun 2026
    Author:  boherm

    The list of wireless RF devices (mics and IEMs) to coordinate. Lives under
    RFCoordinationSettings in the project settings, so it is saved with the show.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "RFDevice.h"
#include "juce_organicui/controllable/parameter/BoolParameter.h"

class RFDeviceManager :
    public BaseManager<RFDevice>
{
public:
    juce_DeclareSingleton(RFDeviceManager, true);

    BoolParameter* displayPresets;

    RFDeviceManager();
    ~RFDeviceManager();

    RFDevice* createItem() override;

};
