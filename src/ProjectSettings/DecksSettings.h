/*
  ==============================================================================

    DecksSettings.h
    Created: 25 Sep 2025 01:30:00pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "juce_organicui/controllable/parameter/TargetParameter.h"

class DecksSettings :
	public ControllableContainer
{
public:
	DecksSettings();

    TargetParameter* deckA;
    TargetParameter* deckB;
    TargetParameter* deckC;
    TargetParameter* deckD;

	void clear();
};
