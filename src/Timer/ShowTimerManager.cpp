/*
  ==============================================================================

    ShowTimerManager.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "ShowTimerManager.h"

juce_ImplementSingleton(ShowTimerManager)

ShowTimerManager::ShowTimerManager() :
    BaseManager<ShowTimer>("Timers")
{
    itemDataType = "Timer";
    selectItemWhenCreated = true;
}

ShowTimerManager::~ShowTimerManager()
{
}

ShowTimer* ShowTimerManager::createItem()
{
    ShowTimer* t = new ShowTimer();
    // Auto-numbering is disabled in BaseManager, so name new timers ourselves.
    t->setNiceName("Timer " + String(items.size() + 1));
    return t;
}

double ShowTimerManager::getGeneralTotalSeconds() const
{
    double total = 0.0;
    for (auto* t : items)
        if (t->isGeneralTimer())
            total += t->getDisplaySeconds();
    return total;
}

bool ShowTimerManager::isAnyGeneralRunning() const
{
    for (auto* t : items)
        if (t->isGeneralTimer() && t->isRunning())
            return true;
    return false;
}
