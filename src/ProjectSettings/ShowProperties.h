/*
  ==============================================================================

    ShowProperties.h
    Created: 24 Sep 2025 03:33:00pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class ShowProperties :
	public ControllableContainer
{
public:
	ShowProperties();

	StringParameter* projectName;
    StringParameter* companyName;
    StringParameter* showFileVersion;
    TargetParameter* nextCueToGo;

	void clear();
};
