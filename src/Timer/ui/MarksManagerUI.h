/*
  ==============================================================================

    MarksManagerUI.h
    Created: 30 Jun 2026
    Author:  boherm

    Compact, headerless, self-sizing container that renders a timer's marks. Meant to
    be embedded inside a ShowTimerManagerItemUI (no viewport, grows with its content).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../MarksManager.h"
#include "MarkUI.h"

class MarksManagerUI :
    public BaseManagerUI<MarksManager, Mark, MarkUI>
{
public:
    MarksManagerUI(MarksManager* manager);
    ~MarksManagerUI();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarksManagerUI)
};
