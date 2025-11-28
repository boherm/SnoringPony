/*
  ==============================================================================

    VolumePresets.cpp
    Created: 28 Nov 2025 12:32:00pm
    Author:  boherm

  ==============================================================================
*/

#include "VolumePresets.h"

VolumePresets::VolumePresets() :
    ControllableContainer("Volume Presets")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;
    userCanAddControllables = true;
    userAddControllablesFilters.add(FloatParameter::getTypeStringStatic());
    customUserCreateControllableFunc = &VolumePresets::createItem;
}

void VolumePresets::createItem(ControllableContainer* cc)
{
    FloatParameter* cp = new FloatParameter("Volume", "", 1.0f);
    cp->userCanChangeName = true;
    cp->isRemovableByUser = true;
    cp->setRange(0.0f, 1.5f);
	cp->saveValueOnly = false;
    cc->addControllable(cp);
}

void VolumePresets::clear()
{
    controllables.clear();
}
