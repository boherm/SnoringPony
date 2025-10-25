/*
  ==============================================================================

    MusicCue.cpp
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "MusicCue.h"

MusicCue::MusicCue(var params)
{
    objectType = "MusicCue";

    audioFile = addFileParameter("Audio File", "Audio file to play for this cue", params.getProperty("audioFile", ""));
}

MusicCue::~MusicCue()
{
}

void MusicCue::play()
{
    Logger::writeToLog("MusicCue::play");
}
