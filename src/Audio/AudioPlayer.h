/*
  ==============================================================================

    AudioPlayer.h
    Created: 11 Nov 2025 06:22:20pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class AudioOutput;

class AudioPlayerMixer:
    public MixerAudioSource
{
public:
    bool isPanicking = false;
    bool isFading = false;
    bool fadeStopPending = false;
    double sampleRate = 44100.0;
    AudioTransportSource* transportSource = nullptr;
    LinearSmoothedValue<float> panicFadingGain { 1.0f };
    LinearSmoothedValue<float> fadingGain { 1.0f };

    void setSource(AudioTransportSource* transport)
    {
        transportSource = transport;
    }

    void setSampleRate(double rate)
    {
        sampleRate = rate;
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override;

    void panicFade();
    void resetPanicFade();
    void fade(double targetGain, double duration);
    void resetFade();
};

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    AudioOutput* output = nullptr;
    File currentFile;
    AudioFormatManager formatManager;
    double duration;

    AudioSourcePlayer* player;
    AudioPlayerMixer* mixer;
    AudioTransportSource* transport;
    std::unique_ptr<AudioFormatReaderSource> readerSource;

    bool setFile(const File& audioFile);
    bool setOutput(AudioOutput* output);

    void play(bool resetFade = true);
    void preview(AudioOutput* previewOutput);
    void stop();
    void panic();
    void stopAndClean();

    void fade(double targetGain, double duration);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPlayer)
};
