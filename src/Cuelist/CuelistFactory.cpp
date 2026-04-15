/*
  ==============================================================================

    CuelistFactory.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistFactory.h"
#include "dca/DCAMixingCuelist.h"

CuelistFactory::CuelistFactory()
{
    defs.add(Factory<Cuelist>::Definition::createDef("", "Playback Cuelist", &Cuelist::create));
    defs.add(Factory<Cuelist>::Definition::createDef("", "DCA Mixing Cuelist", &DCAMixingCuelist::create));
}

Cuelist* CuelistFactory::create(const juce::String& type)
{
    if (type == "Cuelist") return Factory<Cuelist>::create("Playback Cuelist");
    return Factory<Cuelist>::create(type);
}

// void CuelistFactory::buildPopupMenu(int startOffset)
// {
//     Factory<Cuelist>::buildPopupMenu(startOffset);
// }

// void CuelistFactory::showCreateMenu(std::function<void(Cuelist *)> returnFunc)
// {
//     Factory<Cuelist>::showCreateMenu(returnFunc);
// }

// Cuelist * CuelistFactory::createFromMenuResult(int result)
// {
//     return Factory<Cuelist>::createFromMenuResult(result);
// }

juce_ImplementSingleton(CuelistFactory);
