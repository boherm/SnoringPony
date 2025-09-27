/*
  ==============================================================================

    CuelistFactory.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "Cuelist.h"

class CuelistFactory :
    public Factory<Cuelist>
{
public:
    juce_DeclareSingleton(CuelistFactory, true)

    CuelistFactory();
    ~CuelistFactory() {}

    void buildPopupMenu(int startOffset = 0) override;
    void showCreateMenu(std::function<void(Cuelist *)> returnFunc) override;
    Cuelist * createFromMenuResult(int result) override;
};
