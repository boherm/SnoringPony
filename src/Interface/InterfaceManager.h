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

    static void showMenuForTargetAudioOutput(ControllableContainer* startFromCC, std::function<void(AudioOutput*)> returnFunc);

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
