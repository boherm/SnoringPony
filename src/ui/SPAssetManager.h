/*
  ==============================================================================

    AssetManager.h
    Created: 4 May 2025 5:23:47pm
    Author:  Boris Hermans

  ==============================================================================
*/

#pragma once

class SPAssetManager
{
public:
	juce_DeclareSingleton(SPAssetManager, true)
	//SPAssetManager();
	//virtual ~SPAssetManager();

	Image getAboutImage();
};
