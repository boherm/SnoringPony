/*
  ==============================================================================

    RFProfileManager.h
    Created: 27 Jun 2026
    Author:  boherm

    The list of reusable frequency profiles. Lives under RFCoordinationSettings so
    it is saved with the show; devices reference a profile by name.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "RFProfile.h"

class RFProfileManager :
    public BaseManager<RFProfile>
{
public:
    juce_DeclareSingleton(RFProfileManager, true);

    RFProfileManager();
    ~RFProfileManager();

    RFProfile* createItem() override;
};
