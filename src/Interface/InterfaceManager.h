/*
  ==============================================================================

    InterfaceManager.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "../Interface/Interface.h"

class InterfaceManager :
    public BaseManager<Interface>
{
public:
    juce_DeclareSingleton(InterfaceManager, true);

    InterfaceManager();
    ~InterfaceManager();

    Factory<Interface> factory;

    static void showMenuForTargetAudioOutput(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc);
    static void showMenuForTargetMIDIInterface(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc);
	// void feedback(String address, var value, String origin);
};
