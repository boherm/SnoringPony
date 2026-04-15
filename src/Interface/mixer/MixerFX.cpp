/*
  ==============================================================================

    MixerFX.cpp
    Created: 15 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MixerFX.h"

MixerFX::MixerFX(var params) :
    BaseItem(params.getProperty("name", "FX"))
{
    itemDataType = "FX";
    saveAndLoadRecursiveData = true;
    canBeDisabled = false;
    nameCanBeChangedByUser = true;

    busNumber = addIntParameter("Bus", "Bus number on the mixing console",
                                params.getProperty("bus", 1), 1, 64);
}

MixerFX::~MixerFX()
{
}
