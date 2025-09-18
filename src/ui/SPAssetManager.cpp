/*
  ==============================================================================

    SPAssetManager.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "SPAssetManager.h"

juce_ImplementSingleton(SPAssetManager);

Image SPAssetManager::getAboutImage()
{
	return ImageCache::getFromMemory(BinaryData::about_png, BinaryData::about_pngSize);
}
