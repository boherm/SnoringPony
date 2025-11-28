/*
  ==============================================================================

    VolumePresets.h
    Created: 28 Nov 2025 12:32:00pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class VolumePresets :
    public ControllableContainer
{
public:
    VolumePresets();

    static void createItem(ControllableContainer* cc);

    void clear();
};
