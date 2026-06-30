/*
  ==============================================================================

    MarkUI.h
    Created: 30 Jun 2026
    Author:  boherm

    Compact one-row UI for a Mark: editable label on the left, then the segment
    duration and the cumulative time side by side on the right.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Mark.h"

class MarkUI :
    public BaseItemUI<Mark>
{
public:
    MarkUI(Mark* mark);
    ~MarkUI();

    void paintOverChildren(Graphics& g) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkUI)
};
