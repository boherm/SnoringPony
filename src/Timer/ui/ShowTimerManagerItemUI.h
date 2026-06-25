/*
  ==============================================================================

    ShowTimerManagerItemUI.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../ShowTimer.h"

class ShowTimerManagerItemUI :
    public BaseItemUI<ShowTimer>
{
public:
    ShowTimerManagerItemUI(ShowTimer* item);
    ~ShowTimerManagerItemUI();

    void paint(Graphics& g) override;
};
