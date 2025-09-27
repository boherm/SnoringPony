/*
  ==============================================================================

    CuelistFactory.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistFactory.h"

CuelistFactory::CuelistFactory()
{
    defs.add(Factory<Cuelist>::Definition::createDef("", "Cuelist", &Cuelist::create));
}

void CuelistFactory::buildPopupMenu(int startOffset)
{
    Factory<Cuelist>::buildPopupMenu(startOffset);
}

void CuelistFactory::showCreateMenu(std::function<void(Cuelist *)> returnFunc)
{
    Factory<Cuelist>::showCreateMenu(returnFunc);
}

Cuelist * CuelistFactory::createFromMenuResult(int result)
{
    return Factory<Cuelist>::createFromMenuResult(result);
}

juce_ImplementSingleton(CuelistFactory);