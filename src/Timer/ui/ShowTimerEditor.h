/*
  ==============================================================================

    ShowTimerEditor.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "MarksManagerUI.h"

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
    TriggerUI* markBtnUI;
    Label timeLabel;

    // Marks container shown in the body, below the parameters, with a small title.
    Label marksTitle;
    std::unique_ptr<MarksManagerUI> marksUI;
    int lastMarksContentHeight = -1;

    void resizedInternalHeaderItemInternal(Rectangle<int>& r) override;
    void resizedInternalContent(Rectangle<int>& r) override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowTimerEditor)
};
