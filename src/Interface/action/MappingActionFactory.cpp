/*
  ==============================================================================

    MappingActionFactory.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MappingActionFactory.h"
#include "GoAction.h"
#include "PanicAction.h"
#include "SelectNextCueAction.h"
#include "SelectPreviousCueAction.h"

juce_ImplementSingleton(MappingActionFactory)

MappingActionFactory::MappingActionFactory()
{
    factory.defs.add(Factory<MappingAction>::Definition::createDef("", "Go", &GoAction::create));
    factory.defs.add(Factory<MappingAction>::Definition::createDef("", "Panic", &PanicAction::create));
    factory.defs.add(Factory<MappingAction>::Definition::createDef("", "Select Previous Cue", &SelectPreviousCueAction::create));
    factory.defs.add(Factory<MappingAction>::Definition::createDef("", "Select Next Cue", &SelectNextCueAction::create));
}

MappingActionFactory::~MappingActionFactory()
{
}
