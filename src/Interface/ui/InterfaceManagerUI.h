/*
  ==============================================================================

    InterfaceManagerUI.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../InterfaceManager.h"
#include "InterfaceUI.h"

class InterfaceManagerUI :
    public BaseManagerShapeShifterUI<InterfaceManager, Interface, InterfaceUI>
{
public:
    InterfaceManagerUI(const String &name);
    ~InterfaceManagerUI();

    // InterfaceUI* createUIForItem(Interface* item) override;

    static InterfaceManagerUI* create(const String& name) { return new InterfaceManagerUI(name); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterfaceManagerUI)
};
