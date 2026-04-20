/*
  ==============================================================================

    InterfaceManager.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "../ui/SPAssetManager.h"
#include "Interface.h"
#include "InterfaceManager.h"

#include "audio/AudioInterface.h"
#include "audio/AudioOutput.h"
#include "osc/OSCInterface.h"
#include "midi/MIDIInterface.h"
#include "mixer/MixerInterface.h"

InterfaceManager::InterfaceManager() :
    BaseManager("Interfaces")
{
    managerFactory = &factory;

    factory.defs.add(Factory<Interface>::Definition::createDef("", "Audio", &AudioInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("Audio")));
    factory.defs.add(Factory<Interface>::Definition::createDef("", "MIDI", &MIDIInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("MIDI")));
    factory.defs.add(Factory<Interface>::Definition::createDef("", "OSC", &OSCInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("OSC")));
    factory.defs.add(Factory<Interface>::Definition::createDef("", "Mixer", &MixerInterface::create)->addIcon(SPAssetManager::getInstance()->getInterfaceIcon("Mixer")));
}

InterfaceManager::~InterfaceManager()
{
}

void InterfaceManager::showMenuForTargetAudioOutput(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc)
{
    PopupMenu menu;

    InterfaceManager* m = InterfaceManager::getInstance();

    if (startFromCC != nullptr && startFromCC->niceName == "Outputs") {
        AudioOutputManager* outputManager = dynamic_cast<AudioOutputManager*>(startFromCC);
        menu.addSectionHeader(outputManager->audioInterface->niceName);
        for (int i = 1 ; i <= outputManager->items.size(); ++i)
        {
            menu.addItem(i, outputManager->items[i-1]->niceName);
        }

        menu.showMenuAsync(PopupMenu::Options(), [outputManager, returnFunc](int result)
            {
                if (result <= 0) return;
                returnFunc(outputManager->items[result-1]);
            }
        );
    } else {
        for (int i = 1; i <= m->items.size(); ++i)
        {
            if (m->items[i-1]->getTypeString() != "Audio") continue;

            AudioInterface* audioInterface = dynamic_cast<AudioInterface*>(m->items[i-1]);
            PopupMenu sub;

            for (int y = 1 ; y <= audioInterface->outputs->items.size(); ++y)
            {
                sub.addItem((i * 1000) + y, audioInterface->outputs->items[y-1]->niceName);
            }

            menu.addSubMenu(audioInterface->niceName, sub);
        }

        menu.showMenuAsync(PopupMenu::Options(), [m, returnFunc](int result)
            {
                if (result <= 0) return;

                int outputIdx = (result % 1000) - 1;
                int interfaceIdx = ((result - outputIdx) / 1000) - 1;

                AudioInterface* audioInterface = dynamic_cast<AudioInterface*>(m->items[interfaceIdx]);
                returnFunc(audioInterface->outputs->items[outputIdx]);
            }
        );

    }
}

void InterfaceManager::showMenuForTargetMIDIInterface(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc)
{
    PopupMenu menu;
    InterfaceManager* m = InterfaceManager::getInstance();

    int idx = 1;
    for (auto* item : m->items)
    {
        if (item->getTypeString() != "MIDI") continue;
        menu.addItem(idx, item->niceName);
        idx++;
    }

    menu.showMenuAsync(PopupMenu::Options(), [m, returnFunc](int result)
    {
        if (result <= 0) return;

        int current = 1;
        for (auto* item : m->items)
        {
            if (item->getTypeString() != "MIDI") continue;
            if (current == result)
            {
                returnFunc(item);
                return;
            }
            current++;
        }
    });
}

juce_ImplementSingleton(InterfaceManager);
