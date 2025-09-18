/*
  ==============================================================================

    CueListFactory.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CueListFactory.h"

CueListFactory::CueListFactory()
{
    defs.add(Factory<CueList>::Definition::createDef("", "CueList", &CueList::create));
}

void CueListFactory::buildPopupMenu(int startOffset)
{
    Factory<CueList>::buildPopupMenu(startOffset);
}

void CueListFactory::showCreateMenu(std::function<void(CueList *)> returnFunc)
{
    Factory<CueList>::showCreateMenu(returnFunc);
}

CueList * CueListFactory::createFromMenuResult(int result)
{
    return Factory<CueList>::createFromMenuResult(result);
}

juce_ImplementSingleton(CueListFactory);