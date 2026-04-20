/*
  ==============================================================================

    MIDIFeedbackFactory.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MIDIFeedbackFactory.h"
#include "MIDIIsPlayingFeedback.h"
#include "MIDIIsPanickingFeedback.h"

juce_ImplementSingleton(MIDIFeedbackFactory)

MIDIFeedbackFactory::MIDIFeedbackFactory()
{
    factory.defs.add(Factory<MIDIFeedbackItem>::Definition::createDef("", "Is Playing", &MIDIIsPlayingFeedback::create));
    factory.defs.add(Factory<MIDIFeedbackItem>::Definition::createDef("", "Is Panicking", &MIDIIsPanickingFeedback::create));
}

MIDIFeedbackFactory::~MIDIFeedbackFactory()
{
}
