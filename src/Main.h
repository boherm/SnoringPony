/*
  ==============================================================================

    Main.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once
// #pragma warning(disable:4244 4100 4305 26451 26495)

#include "MainIncludes.h"

//==============================================================================
class SnoringPonyApplication : public OrganicApplication
{
public:
	//==============================================================================
	SnoringPonyApplication();

	bool crashSent;

	void initialiseInternal(const String& /*commandLine*/) override;
	void afterInit() override;

	void shutdown() override;
    void systemRequestedQuit() override;

	void handleCrashed() override;
};

START_JUCE_APPLICATION(SnoringPonyApplication)
