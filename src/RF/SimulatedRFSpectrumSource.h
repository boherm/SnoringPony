/*
  ==============================================================================

    SimulatedRFSpectrumSource.h
    Created: 26 Jun 2026
    Author:  boherm

    Phase-1 stand-in for the RTL-SDR: a background thread that synthesises a
    plausible UHF spectrum (noise floor + a handful of slowly drifting carriers)
    so the panel and optimizer can be exercised without hardware.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "RFSpectrumSource.h"

class SimulatedRFSpectrumSource :
    public RFSpectrumSource,
    public juce::Thread
{
public:
    SimulatedRFSpectrumSource();
    ~SimulatedRFSpectrumSource() override;

    void start(double minMHz, double maxMHz, double binHz) override;
    void stop() override;
    bool isRunning() const override { return running.load(); }
    bool getLatestSweep(RFSpectrumSweep& out) override;
    void setReferenceBand(double minMHz, double maxMHz) override;

    void run() override;

private:
    struct Carrier
    {
        double freqMHz;      // base centre frequency
        double levelDb;      // base level
        double widthMHz;     // skirt width
        double driftMHz;     // drift amplitude
        double driftSpeed;   // drift angular speed (rad/s)
        double phase;        // per-carrier phase offset
        double levelModDb;   // level fluctuation amplitude (dB)
        double levelSpeed;   // level fluctuation speed (rad/s)
        double blinkSpeed;   // on/off gating speed (rad/s); 0 = always on
    };

    std::atomic<bool> running { false };
    double bandMinMHz { 470.0 }, bandMaxMHz { 870.0 };   // current scan window
    double refMinMHz { 0.0 }, refMaxMHz { 0.0 };          // absolute carrier layout band
    double targetBinHz { 100000.0 };

    juce::CriticalSection lock;
    RFSpectrumSweep latest;
    bool hasData { false };

    std::vector<Carrier> carriers;
    juce::Random rng;
    int frame { 0 };

    void buildCarriers();
    void renderSweep(RFSpectrumSweep& sweep);
};
