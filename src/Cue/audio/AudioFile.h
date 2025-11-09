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

class AudioFile:
    public BaseItem
{
public:
    AudioFile(var params = var(), AudioCue* audioCue = nullptr);
    virtual ~AudioFile() { }

    AudioCue* audioCue;
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
    // private ChangeListener
{
public:
    AudioFilesManager(AudioCue* audioCue);
    virtual ~AudioFilesManager() { }

    AudioCue* audioCue;

    AudioFile* createItem() override;

    void playAll();
    void stopAll();

private:
    struct OutputPlaybackContext
    {
        AudioOutput* output = nullptr;
        std::unique_ptr<MixerAudioSource> mixer;
        std::unique_ptr<AudioSourcePlayer> player;
        int activeTransports = 0;
    };

    struct PlaybackInstance
    {
        OutputPlaybackContext* context = nullptr;
        std::unique_ptr<AudioFormatReaderSource> readerSource;
        std::unique_ptr<AudioTransportSource> transport;
    };

    std::unordered_map<AudioOutput*, std::unique_ptr<OutputPlaybackContext>> outputContexts;
    std::vector<std::unique_ptr<PlaybackInstance>> activePlaybacks;

    OutputPlaybackContext* getOrCreateOutputContext(AudioOutput*);
    void releaseOutputContext(AudioOutput*);
    // void changeListenerCallback(ChangeBroadcaster* source) override;
};
