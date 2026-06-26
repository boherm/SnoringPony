/*
  ==============================================================================

    RFDeviceManager.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFDeviceManager.h"

juce_ImplementSingleton(RFDeviceManager)

RFDeviceManager::RFDeviceManager() :
    BaseManager<RFDevice>("RF Devices")
{
    itemDataType = "RF Device";
    selectItemWhenCreated = false;
}

RFDeviceManager::~RFDeviceManager()
{
}

RFDevice* RFDeviceManager::createItem()
{
    RFDevice* d = new RFDevice();
    // Auto-numbering is disabled in BaseManager, so name new devices ourselves.
    d->setNiceName("Device " + String(items.size() + 1));
    return d;
}
