/*
  ==============================================================================

    InterfaceManager.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "../ui/SPAssetManager.h"
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

    factory.defs.add(Factory<Interface>::Definition::createDef("", "Audio", &AudioInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("Audio")));
    // factory.defs.add(Factory<Interface>::Definition::createDef("", "MIDI", &MIDIInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("MIDI")));
    // factory.defs.add(Factory<Interface>::Definition::createDef("", "OSC", &OSCInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("OSC")));
    // factory.defs.add(Factory<Interface>::Definition::createDef("", "Mixer", &MixerInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("Mixer")));
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
