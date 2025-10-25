/*
  ==============================================================================

    Interface.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class InterfaceUI;

class Interface :
    public BaseItem
{
public:
    Interface(String name = "Interface");
    virtual ~Interface();

    BoolParameter* logIncomingData;
    BoolParameter* logOutgoingData;

    // virtual void sendValuesForObject(Object* o) {}

    virtual ControllableContainer* getInterfaceParams() { return new ControllableContainer("Interface parameters"); }

    virtual InterfaceUI* createUI();

    static bool isMidiInterface(Interface* i);
};
