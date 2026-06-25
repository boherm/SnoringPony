/*
  ==============================================================================

    MeteringPanel.h
    Created: 25 Jun 2026
    Author:  boherm

    Display-only metering panel: dBFS meter + big calibrated dB SPL readouts
    (instantaneous and 15-min LAeq, A/B/C weighted, red over a threshold) and a
    1-minute scrolling spectrogram (hover to read the frequency). All setup lives
    in the project settings (MeteringSettings, which owns the input device); the
    panel only adds a Start/Stop button.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include <juce_dsp/juce_dsp.h>

class MeteringSettings;

class MeteringUI : public ShapeShifterContent {
public:
    MeteringUI(const String &contentName);
    ~MeteringUI();

    static MeteringUI *create(const String &name) { return new MeteringUI(name); }
};

class MeteringPanel :
    public juce::Component,
    public juce::Timer,
    public juce::AudioIODeviceCallback,
    public ParameterListener,
    public EngineListener
{
public:
    juce_DeclareSingleton(MeteringPanel, true);
    MeteringPanel();
    ~MeteringPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void parameterValueChanged(Parameter* p) override;

    void fileLoaded() override;
    void engineCleared() override;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                          float* const* outputChannelData, int numOutputChannels,
                                          int numSamples, const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numBins  = fftSize / 2;
    static constexpr int laeqSeconds = 15 * 60;

    MeteringSettings* settings = nullptr;
    juce::TextButton startStopBtn { "Start" };

    float calibrationDb { 100.0f };
    float thresholdDb { 90.0f };
    int   weightingType { 0 };
    bool  verticalTime { false };

    juce::AbstractFifo fifo { 1 << 16 };
    std::vector<float> fifoData;
    std::atomic<double> sampleRate { 48000.0 };
    std::atomic<float>  inputPeak { 0.0f };
    std::atomic<bool>   deviceRunning { false };

    juce::dsp::FFT fft { fftOrder };
    std::vector<float> window;
    double windowPower { 1.0 };
    std::vector<float> fftBuffer;
    std::vector<float> sampleAccum;
    std::vector<float> pendingColumn;
    double pendingColumnTime { 0.0 };
    juce::Image spectro;

    std::vector<float> weightingGain;
    int    weightingComputedType { -1 };
    double weightingComputedSr { 0.0 };
    double splFastMs { 0.0 };
    std::vector<float> laeqRing;
    int    laeqPos { 0 }, laeqCount { 0 };
    double laeqSecEnergy { 0.0 }, laeqSecTime { 0.0 };
    float  splNow { -120.0f }, splLaeq { -120.0f };

    float meterDb { -100.0f };
    juce::Point<int> mousePos { -1, -1 };
    bool  mouseInSpectro { false };

    juce::Rectangle<int> readoutArea, meterArea, freqAxisArea, spectroImageArea;

    void applySettings();
    void clearSpectrogram();
    void pullAndProcess();
    void ensureWeighting(double sr);
    void commitColumn(int w, int h, double sr);

    void drawMeter(juce::Graphics& g, juce::Rectangle<int> area);
    void drawReadout(juce::Graphics& g);
    void drawFreqAxis(juce::Graphics& g, double sr);

    double fracFromFreq(double f, double sr) const;
    double freqFromFrac(double frac, double sr) const;
    static juce::Colour magnitudeToColour(float mag);
    static float weightingLinear(double f, int type);
    String weightingLabel() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeteringPanel)
};
