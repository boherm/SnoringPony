/*
  ==============================================================================

    ShowTimerManager.h
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "ShowTimer.h"

class ShowTimerManager :
    public BaseManager<ShowTimer>
{
public:
    juce_DeclareSingleton(ShowTimerManager, true);

    ShowTimerManager();
    ~ShowTimerManager();

    ShowTimer* createItem() override;

    // Sum of the displayed value of every "general" timer.
    double getGeneralTotalSeconds() const;

    // True when at least one "general" timer is currently running.
    bool isAnyGeneralRunning() const;
};
