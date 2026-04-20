/*
  ==============================================================================

    OSCFeedbackFactory.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OSCFeedbackFactory.h"
#include "OSCIsPlayingFeedback.h"
#include "OSCIsPanickingFeedback.h"
#include "OSCCurrentCueFeedback.h"
#include "OSCNextCueFeedback.h"

juce_ImplementSingleton(OSCFeedbackFactory)

OSCFeedbackFactory::OSCFeedbackFactory()
{
    factory.defs.add(Factory<OSCFeedbackItem>::Definition::createDef("", "Is Playing", &OSCIsPlayingFeedback::create));
    factory.defs.add(Factory<OSCFeedbackItem>::Definition::createDef("", "Is Panicking", &OSCIsPanickingFeedback::create));
    factory.defs.add(Factory<OSCFeedbackItem>::Definition::createDef("", "Current Cue", &OSCCurrentCueFeedback::create));
    factory.defs.add(Factory<OSCFeedbackItem>::Definition::createDef("", "Next Cue", &OSCNextCueFeedback::create));
}

OSCFeedbackFactory::~OSCFeedbackFactory()
{
}
