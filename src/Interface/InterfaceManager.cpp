/*
  ==============================================================================

    InterfaceManager.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "InterfaceManager.h"
#include "Interface.h"

#include "audio/AudioOutput.h"
#include "midi/MIDIInterface.h"
#include "osc/OSCInterface.h"
#include "audio/AudioInterface.h"
#include "mixer/MixerInterface.h"

InterfaceManager::InterfaceManager() :
    BaseManager("Interfaces")
{
    managerFactory = &factory;

    factory.defs.add(Factory<Interface>::Definition::createDef("", "Audio", &AudioInterface::create));
    factory.defs.add(Factory<Interface>::Definition::createDef("", "MIDI", &MIDIInterface::create));
    // factory.defs.add(Factory<Interface>::Definition::createDef("", "OSC", &OSCInterface::create));
    // factory.defs.add(Factory<Interface>::Definition::createDef("", "Mixer", &MixerInterface::create));
}

InterfaceManager::~InterfaceManager()
{
}

// void InterfaceManager::feedback(String address, var value, String origin = "")
// {
    // Array<MIDIInterface*> midiInterfaces = getInterfacesOfType<MIDIInterface>();
    // for (int i = 0; i < midiInterfaces.size(); i++) {
    //     midiInterfaces[i]->feedback(address, value, origin);
    // }
    // Array<OSCInterface*> oscInterfaces = getInterfacesOfType<OSCInterface>();
    // for (int i = 0; i < oscInterfaces.size(); i++) {
    //     oscInterfaces[i]->feedback(address, value, origin);
    // }
// }

juce_ImplementSingleton(InterfaceManager);
