/*
  ==============================================================================

    ShowTimerManagerUI.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../ShowTimerManager.h"
#include "ShowTimerManagerItemUI.h"

class ShowTimerManagerUI :
    public BaseManagerShapeShifterUI<ShowTimerManager, ShowTimer, ShowTimerManagerItemUI>,
    public juce::Timer
{
public:
    ShowTimerManagerUI(const String& contentName);
    ~ShowTimerManagerUI();

    Rectangle<int> footerBounds;
    juce::TextButton resetAllBtn { "Reset" };

    void resizedInternalHeader(Rectangle<int>& r) override;
    void resizedInternalFooter(Rectangle<int>& r) override;
    void paintOverChildren(Graphics& g) override;
    void buttonClicked(Button* b) override;
    void timerCallback() override;

    static ShowTimerManagerUI* create(const String& name) { return new ShowTimerManagerUI(name); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowTimerManagerUI)
};
