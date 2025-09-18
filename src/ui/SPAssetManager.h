/*
  ==============================================================================

    SPAssetManager.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class SPAssetManager
{
public:
	juce_DeclareSingleton(SPAssetManager, true)
	//SPAssetManager();
	//virtual ~SPAssetManager();

	Image getAboutImage();
};
