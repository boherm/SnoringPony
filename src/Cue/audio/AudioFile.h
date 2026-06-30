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
    FloatParameter* volume;

    String getTypeString() const override { return "AudioFile"; }
    static AudioFile* create(var params) { return new AudioFile(params); }

    // Apply the playback gain: the file's volume when enabled, silence when disabled.
    // Disabled files keep running (transport never stops) so they stay sample-locked
    // with the others and can be unmuted in perfect sync. This sits on the transport
    // gain stage, independent of the cue fades (which use the mixer's fading gain).
    void updateGain();

    void parameterValueChanged(Parameter* p) override;
    void parameterControlModeChanged(Parameter* p) override;
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

    void playAll(bool resetFade = true);
    void previewAll(bool resetFade = true);
    void stopAll();
    void panicAll();

    void resetCurrentTime();
    void setCurrentTime(double time);

    bool haveOnePlaying();

    double getCurrentTime();
};
