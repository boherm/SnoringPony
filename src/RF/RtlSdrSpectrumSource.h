/*
  ==============================================================================

    RtlSdrSpectrumSource.h
    Created: 26 Jun 2026
    Author:  boherm

    Real RF spectrum source backed by an RTL2832U (RTL-SDR) USB dongle. It sweeps
    the requested band by retuning the tuner in steps, taking an FFT of the IQ
    samples at each step and stitching the segments together (rtl_power principle).

    Only functional when built with SP_HAS_RTLSDR=1 (librtlsdr vendored, see
    external/rtlsdr/README.md); otherwise every method is a no-op stub.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "RFSpectrumSource.h"
#include <juce_dsp/juce_dsp.h>

class RtlSdrSpectrumSource :
    public RFSpectrumSource,
    public juce::Thread
{
public:
    RtlSdrSpectrumSource();
    ~RtlSdrSpectrumSource() override;

    void start(double minMHz, double maxMHz, double binHz) override;
    void stop() override;
    bool isRunning() const override { return running.load(); }
    bool getLatestSweep(RFSpectrumSweep& out) override;

    void run() override;

    // Tuner configuration (applied on the next start()).
    void configure(int deviceIndex, bool autoGain, float gainDb, int ppm);

    juce::String getStatusMessage() const;

    static bool isSupported();                  // true if built with hardware support
    static juce::StringArray listDevices();     // names of connected dongles

private:
    std::atomic<bool> running { false };
    double bandMinMHz { 470.0 }, bandMaxMHz { 870.0 }, targetBinHz { 25000.0 };
    int    deviceIndex { 0 };
    bool   autoGain { true };
    float  gainDb { 30.0f };
    int    ppm { 0 };

    void* dev { nullptr };   // rtlsdr_dev_t*

    juce::CriticalSection lock;
    RFSpectrumSweep latest;
    bool hasData { false };

    juce::CriticalSection statusLock;
    juce::String status;
    void setStatus(const juce::String& s);
};
