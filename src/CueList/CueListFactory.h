/*
  ==============================================================================

    CueListFactory.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "CueList.h"

class CueListFactory :
    public Factory<CueList>
{
public:
    juce_DeclareSingleton(CueListFactory, true)

    CueListFactory();
    ~CueListFactory() {}

    void buildPopupMenu(int startOffset = 0) override;
    void showCreateMenu(std::function<void(CueList *)> returnFunc) override;
    CueList * createFromMenuResult(int result) override;
};
