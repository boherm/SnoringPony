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
class AudioOutput;
class AudioPlayer;

class AudioFile:
    public BaseItem
{
public:
    AudioFile(var params = var(), AudioCue* audioCue = nullptr);
    virtual ~AudioFile();

    AudioCue* audioCue;
    std::unique_ptr<AudioPlayer> player;
    FileParameter* audioFile;
    FloatParameter* duration;
    TargetParameter* targetAudioInterface;

    String getTypeString() const override { return "AudioFile"; }
    static AudioFile* create(var params) { return new AudioFile(params); }

    void parameterValueChanged(Parameter* p) override;
};

//==============================================================================

class AudioFilesManager:
    public BaseManager<AudioFile>
{
public:
    AudioFilesManager(AudioCue* audioCue);
    virtual ~AudioFilesManager();

    AudioCue* audioCue;

    AudioFile* createItem() override;

    void playAll();
    void stopAll();
};
