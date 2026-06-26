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
    void mouseDown(const juce::MouseEvent& e) override;

    void parameterValueChanged(Parameter* p) override;

    // Current input level in dBFS (uncalibrated, weighted like the SPL reading).
    // Used by MeteringSettings' Auto-Calibrate to derive the calibration.
    float getMeasuredDbFs() const;
    bool  isInputRunning() const { return deviceRunning.load(); }

    // Reset the held Max / Peak readouts (called on start and by the settings trigger).
    void  resetMaxPeak() { splMax = -120.0f; splPeak = -120.0f; repaint(); }

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
    juce::TextButton modeBtn { "RTA" };       // toggles spectrogram <-> RTA

    float calibrationDb { 100.0f };
    float thresholdDb { 90.0f };
    int   weightingType { 0 };
    int   displayMode { 0 };          // 0 = spectrogram, 1 = RTA
    bool  verticalTime { false };

    // Frequencies are shown on the horizontal axis for the RTA (always) and for the
    // vertical-time spectrogram; on the vertical axis for the horizontal spectrogram.
    bool  freqOnX() const { return displayMode == 1 || verticalTime; }

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

    // RTA: log-spaced 1/24-octave bands (20 Hz .. 20 kHz), smoothed magnitude + falling peak hold.
    static constexpr double rtaFMin = 20.0;
    static constexpr double rtaFMax = 20000.0;
    static constexpr int    rtaBandsPerOctave = 24;
    int numRtaBands { 0 };
    std::vector<double> rtaCenters;            // band centre frequencies (Hz)
    std::vector<int>   rtaBandLo, rtaBandHi;   // FFT bin range per band (lo > hi = out of range)
    std::vector<float> rtaSmooth, rtaPeak;     // linear magnitude per band
    double rtaBandsComputedSr { 0.0 };

    std::vector<float> weightingGain;
    int    weightingComputedType { -1 };
    double weightingComputedSr { 0.0 };
    double splFastMs { 0.0 };
    std::vector<float> laeqRing;
    int    laeqPos { 0 }, laeqCount { 0 };
    double laeqSecEnergy { 0.0 }, laeqSecTime { 0.0 };
    float  splNow { -120.0f }, splLaeq { -120.0f };
    float  splMax { -120.0f };   // held max of the (weighted) Now reading
    float  splPeak { -120.0f };  // held true peak (unweighted) level

    float meterDb { -100.0f };
    int   alertHoldTicks { 0 };   // frames left to keep the over-threshold red border
    juce::Point<int> mousePos { -1, -1 };
    bool  mouseInSpectro { false };

    juce::Rectangle<int> readoutArea, meterArea, freqAxisArea, spectroImageArea;
    juce::Rectangle<int> maxClickArea, peakClickArea;   // click to reset Max / Peak

    void applySettings();
    void clearSpectrogram();
    void pullAndProcess();
    void ensureWeighting(double sr);
    void ensureRtaBands(double sr);
    void commitColumn(int w, int h, double sr);

    void drawMeter(juce::Graphics& g, juce::Rectangle<int> area);
    void drawReadout(juce::Graphics& g);
    void drawFreqAxis(juce::Graphics& g, double sr);
    void drawRTA(juce::Graphics& g, double sr);

    double fracFromFreq(double f, double sr) const;
    double freqFromFrac(double frac, double sr) const;
    static juce::Colour magnitudeToColour(float mag);
    static float weightingLinear(double f, int type);
    String weightingLabel() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeteringPanel)
};
