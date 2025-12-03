/*
  ==============================================================================

    PlaylistCue.cpp
    Created: 3 Dec 2025 06:30:22pm
    Author:  boherm

  ==============================================================================
*/

#include "PlaylistCue.h"

PlaylistCue::PlaylistCue(var params) :
    Cue(params)
{
    objectType = "Playlist";

    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;
}

PlaylistCue::~PlaylistCue()
{
}

void PlaylistCue::play()
{
    Logger::writeToLog("PlaylistCue::play: " + niceName);
}

void PlaylistCue::stop()
{
    Logger::writeToLog("PlaylistCue::stop: " + niceName);
}
