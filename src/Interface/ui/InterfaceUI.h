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

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterfaceUI)
};
