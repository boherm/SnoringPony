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
class PluginChainManager;

class AudioPlayerMixer:
    public MixerAudioSource
{
public:
    bool isPanicking = false;
    bool isFading = false;
    bool fadeStopAfterComplete = false;
    double sampleRate = 44100.0;
    int blockSize = 512;
    int numChannels = 2;
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
    void fadeIn(double duration, float targetGain = 1.0f);
    void fadeOut(double duration, bool stopAfterFade = true);
    void resetFade();

    PluginChainManager* pluginChain = nullptr;
    void setPluginChain(PluginChainManager* chain);
};

// Wraps the file reader and jumps over "skip" regions directly in the audio callback,
// sample-accurate, with a short equal-power crossfade at each splice to avoid clicks.
// Positions stay real file positions (getNextReadPosition is never remapped).
class SkipRegionsAudioSource:
    public PositionableAudioSource
{
public:
    static constexpr int maxRegions = 64;
    static constexpr double crossfadeSeconds = 0.01;

    void setInput(PositionableAudioSource* source, double sourceSampleRate);

    // Called from the message thread. Regions are in seconds, they get sorted and merged.
    void setSkipRegions(const Array<Range<double>>& regions);

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const AudioSourceChannelInfo& info) override;

    void setNextReadPosition(int64 newPosition) override;
    int64 getNextReadPosition() const override;
    int64 getTotalLength() const override;
    bool isLooping() const override;
    void setLooping(bool shouldLoop) override;

private:
    PositionableAudioSource* input = nullptr;
    double inputSampleRate = 44100.0;

    SpinLock regionsLock;
    Array<Range<double>> pendingRegions; // seconds, guarded by regionsLock
    bool regionsChanged = false;         // guarded by regionsLock

    // audio thread only
    std::array<Range<int64>, maxRegions> regions;
    int numRegions = 0;
    int crossfadeLength = 0;    // nominal length, in source samples
    int crossfadeTotal = 1;     // length of the crossfade in progress
    int crossfadeRemaining = 0;
    int64 crossfadeTailPos = 0;
    AudioBuffer<float> tailBuffer;

    void pullRegions();
    void readFromInput(AudioBuffer<float>& buffer, int startSample, int numSamples);
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
    SkipRegionsAudioSource skipSource;

    // Regions (in seconds, file time) jumped over during playback. Kept across play() calls.
    void setSkipRegions(const Array<Range<double>>& regions);

    bool setFile(const File& audioFile);
    bool setOutput(AudioOutput* output);

    void play(bool resetFade = true);
    void preview(AudioOutput* previewOutput);
    void stop();
    void panic();
    void stopAndClean();

    void fade(double targetGain, double duration);
    void fadeIn(double duration, float targetGain = 1.0f);
    void fadeOut(double duration, bool stopAfterFade = true);

    // Current fade/panic multiplier (0..1), i.e. the transient gain applied on top of the
    // transport volume. Used to display the live output level (e.g. while ducking).
    double getFadeMultiplier() const;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPlayer)
};
