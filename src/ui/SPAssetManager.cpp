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

Image SPAssetManager::getInterfaceIcon(String type)
{
    if (type == "MIDI") {
        return ImageCache::getFromMemory(BinaryData::midi_interface_png, BinaryData::midi_interface_pngSize);
    }
    else if (type == "Audio") {
        return ImageCache::getFromMemory(BinaryData::audio_interface_png, BinaryData::audio_interface_pngSize);
    }
    else if (type == "OSC") {
        return ImageCache::getFromMemory(BinaryData::network_interface_png, BinaryData::network_interface_pngSize);
    }
    else if (type == "Mixer") {
        return ImageCache::getFromMemory(BinaryData::mixer_interface_png, BinaryData::mixer_interface_pngSize);
    }
    else if (type == "OBS") {
        return ImageCache::getFromMemory(BinaryData::obs_interface_png, BinaryData::obs_interface_pngSize);
    }
    else {
        return ImageCache::getFromMemory(BinaryData::noicon_png, BinaryData::noicon_pngSize);
    }
}

Image SPAssetManager::getCueIcon(String type)
{
    if (type == "Audio") {
        return ImageCache::getFromMemory(BinaryData::audio_cue_png, BinaryData::audio_cue_pngSize);
    } else if (type == "Group") {
        return ImageCache::getFromMemory(BinaryData::group_cue_png, BinaryData::group_cue_pngSize);
    } else if (type == "Note") {
        return ImageCache::getFromMemory(BinaryData::note_cue_png, BinaryData::note_cue_pngSize);
    } else if (type == "Fade") {
        return ImageCache::getFromMemory(BinaryData::fade_cue_png, BinaryData::fade_cue_pngSize);
    } else if (type == "Playlist") {
        return ImageCache::getFromMemory(BinaryData::playlist_cue_png, BinaryData::playlist_cue_pngSize);
    } else if (type == "OSC") {
        return ImageCache::getFromMemory(BinaryData::network_interface_png, BinaryData::network_interface_pngSize);
    } else if (type == "OBS") {
        return ImageCache::getFromMemory(BinaryData::obs_interface_png, BinaryData::obs_interface_pngSize);
    } else {
        return ImageCache::getFromMemory(BinaryData::noicon_png, BinaryData::noicon_pngSize);
    }
}
