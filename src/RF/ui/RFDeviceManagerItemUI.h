/*
  ==============================================================================

    RFDeviceManagerItemUI.h
    Created: 26 Jun 2026
    Author:  boherm

    List row for an RF device: shows the assigned frequency (Range mode) or the
    chosen preset name with the frequency as a tooltip (Presets mode), in a small
    box on the right - same style as the timers list.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../RFDevice.h"

class RFDeviceManagerItemUI :
    public BaseItemUI<RFDevice>,
    public juce::SettableTooltipClient
{
public:
    RFDeviceManagerItemUI(RFDevice* item);
    ~RFDeviceManagerItemUI();

    void paint(Graphics& g) override;
    void controllableFeedbackUpdateInternal(Controllable* c) override;
    void mouseDown(const MouseEvent& e) override;

    // Listen to the manager's "Display presets" toggle to refresh the row.
    void newMessage(const ContainerAsyncEvent& e) override;

private:
    // Bounds of the value box drawn in paint(), used to hit-test clicks.
    Rectangle<int> valueBox;
};
