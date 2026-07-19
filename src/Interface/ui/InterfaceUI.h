/*
  ==============================================================================

    InterfaceUI.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Interface.h"

class InterfaceUI :
    public BaseItemUI<Interface>
{
public:
    InterfaceUI(Interface* item);
    virtual ~InterfaceUI();

    ImageComponent iconUI;

    void resizedHeader(Rectangle<int>& r);

    // Draw a distinct "STANDBY" tint/badge when the interface is silenced by the Backup role
    // (active but held quiet). Repaints when the redundancy role changes.
    void paintOverChildren(Graphics& g) override;
    void newMessage(const Parameter::ParameterEvent& e) override;

private:
    Parameter* rmRoleParam = nullptr;
    Parameter* rmEnabledParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterfaceUI)
};
