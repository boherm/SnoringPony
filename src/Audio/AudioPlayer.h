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
    AudioTransportSource* transport;
    AudioFormatReaderSource* readerSource;

    bool setFile(const File& audioFile);
    bool setOutput(AudioOutput* output);

    void play();
    void stop();
    void stopAndClean();

    // void setSource(AudioSource* newSource);
    // AudioSource* getCurrentSource() const noexcept { return source; }

    // void setGain(float newGain) noexcept;
    // float getGain() const noexcept { return gain; }

    // AudioSource overrides
    // void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    // void releaseResources() override;
    // void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPlayer)
};
