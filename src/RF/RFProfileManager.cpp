/*
  ==============================================================================

    RFProfileManager.cpp
    Created: 27 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFProfileManager.h"

juce_ImplementSingleton(RFProfileManager)

RFProfileManager::RFProfileManager() :
    BaseManager<RFProfile>("RF Profiles")
{
    itemDataType = "RF Profile";
}

RFProfileManager::~RFProfileManager()
{
}

RFProfile* RFProfileManager::createItem()
{
    RFProfile* p = new RFProfile();
    p->setNiceName("Profile " + String(items.size() + 1));
    return p;
}
