/*
  ==============================================================================

    AssetManager.cpp
    Created: 4 May 2025 5:23:47pm
    Author:  Boris Hermans

  ==============================================================================
*/

#include "../MainIncludes.h"
#include "SPAssetManager.h"

juce_ImplementSingleton(SPAssetManager);

Image SPAssetManager::getAboutImage()
{
	return ImageCache::getFromMemory(BinaryData::about_png, BinaryData::about_pngSize);
}
