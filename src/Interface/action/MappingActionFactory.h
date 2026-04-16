/*
  ==============================================================================

    MappingActionFactory.h
    Created: 16 Apr 2026
    Author:  boherm

    Singleton factory that registers all available MappingAction types.
    Used by MIDIMapping (and future interfaces) to offer action creation menus.

  ==============================================================================
*/

#pragma once

#include "MappingAction.h"

class MappingActionFactory
{
public:
    juce_DeclareSingleton(MappingActionFactory, true)

    MappingActionFactory();
    ~MappingActionFactory();

    Factory<MappingAction> factory;
};
