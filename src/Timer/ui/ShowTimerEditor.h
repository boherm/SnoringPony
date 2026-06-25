/*
  ==============================================================================

    ShowTimerEditor.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class ShowTimer;

class ShowTimerEditor :
    public BaseItemEditor,
    public juce::Timer
{
public:
    ShowTimerEditor(Array<ShowTimer*> timers, bool isRoot);
    virtual ~ShowTimerEditor();

    ShowTimer* timer;

    TriggerUI* playBtnUI;
    TriggerUI* pauseBtnUI;
    TriggerUI* resetBtnUI;
    Label timeLabel;

    void resizedInternalHeaderItemInternal(Rectangle<int>& r) override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowTimerEditor)
};
