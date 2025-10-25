/*
  ==============================================================================

    InterfaceManagerUI.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "InterfaceManagerUI.h"
#include "../audio/AudioInterface.h"

InterfaceManagerUI::InterfaceManagerUI(const String& name) :
    BaseManagerShapeShifterUI(name, InterfaceManager::getInstance())
{
    addItemText = "Add a new Interface";
    noItemText = "You can manage your i/o here,\nadd a new interface to get started.";

    addExistingItems();
}

InterfaceManagerUI::~InterfaceManagerUI()
{
}

// InterfaceUI* InterfaceManagerUI::createUIForItem(Interface* item)
// {
//     return item->createUI();
// }
