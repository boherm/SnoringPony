/*
  ==============================================================================

    InterfaceManager.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "Interface.h"
#include "audio/AudioInterface.h"
#include "audio/AudioOutput.h"

class InterfaceManager :
    public BaseManager<Interface>
{
public:
    juce_DeclareSingleton(InterfaceManager, true);

    InterfaceManager();
    ~InterfaceManager();

    Factory<Interface> factory;

	template <class T>
	static void showAndGetInterfaceOfType(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc)
	{
		PopupMenu menu;
		Array<Interface*> validModules;
		for (auto& m : InterfaceManager::getInstance()->items)
		{
			T* mt = dynamic_cast<T*>(m);
			if (mt == nullptr) continue;
			validModules.add(m);
			menu.addItem(validModules.indexOf(m) + 1, m->niceName);
		}

		menu.showMenuAsync(PopupMenu::Options(), [validModules, returnFunc](int result)
			{

				if (result == 0) return;
		returnFunc(validModules[result - 1]);
			}
		);
	}

    static void showMenuForTargetAudioOutput(ControllableContainer* startFromCC, std::function<void(AudioOutput*)> returnFunc)
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

                for (int y = 1 ; y <= audioInterface->outputs.items.size(); ++y)
                {
                    sub.addItem((i * 1000) + y, audioInterface->outputs.items[y-1]->niceName);
                }

                menu.addSubMenu(audioInterface->niceName, sub);
            }

            menu.showMenuAsync(PopupMenu::Options(), [m, returnFunc](int result)
                {
                    if (result <= 0) return;

                    int outputIdx = (result % 1000) - 1;
                    int interfaceIdx = ((result - outputIdx) / 1000) - 1;

                    AudioInterface* audioInterface = dynamic_cast<AudioInterface*>(m->items[interfaceIdx]);
                    returnFunc(audioInterface->outputs.items[outputIdx]);
                }
            );

        }
    }

	template <class T>
	static Array<T*> getInterfacesOfType()
	{
		Array<T*> ret;
		for (auto& m : InterfaceManager::getInstance()->items)
		{
			T* mt = dynamic_cast<T*>(m);
			if (mt == nullptr) continue;
			ret.add(mt);
		}
		return ret;

	}

	// void feedback(String address, var value, String origin);
};
