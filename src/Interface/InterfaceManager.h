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

    void addItemInternal(Interface* i, juce::var data) override;

    // Redundancy standby: fan the current standby state out to every interface (and to any newly
    // added one). Driven by RedundancyManager on role change.
    bool redundancyStandby = false;
    void setRedundancyStandby(bool standby);

    static void showMenuForTargetAudioOutput(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc);
    static void showMenuForTargetMIDIInterface(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc);
	// void feedback(String address, var value, String origin);
};
