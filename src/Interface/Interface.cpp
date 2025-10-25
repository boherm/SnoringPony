/*
  ==============================================================================

    Interface.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "Interface.h"
#include "ui/InterfaceUI.h"

Interface::Interface(String name) :
    BaseItem(name)
{
    saveAndLoadRecursiveData = true;
    userCanRemove = true;
    canBeDisabled = true;
    showWarningInUI = true;

    logIncomingData = addBoolParameter("Log Incoming", "Log incoming data", false);
    logOutgoingData = addBoolParameter("Log Outgoing", "Log outgoing data", false);
}

Interface::~Interface()
{
}

InterfaceUI* Interface::createUI()
{
    return new InterfaceUI(this);
}

bool Interface::isMidiInterface(Interface* i)
{
    return false;
}
