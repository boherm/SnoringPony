/*
  ==============================================================================

    AudioFile.h
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class AudioCue;

class AudioFile:
    public BaseItem
{
public:
    AudioFile(var params = var(), AudioCue* audioCue = nullptr);
    virtual ~AudioFile() { }

    AudioCue* audioCue;
    FileParameter* audioFile;
    TargetParameter* targetAudioInterface;

    String getTypeString() const override { return "AudioFile"; }
    static AudioFile* create(var params) { return new AudioFile(params); }
};

//==============================================================================

class AudioManager:
    public BaseManager<AudioFile>
{
public:
    AudioManager(AudioCue* audioCue);
    virtual ~AudioManager() { }

    AudioCue* audioCue;

    AudioFile* createItem() override;
};
