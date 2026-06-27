/*
  ==============================================================================

    RFSpectrumSource.h
    Created: 26 Jun 2026
    Author:  boherm

    Abstraction over a source of RF spectrum sweeps across a frequency band.
    Phase 1 ships only the simulated source; Phase 2 will add an RTL-SDR source
    behind the same interface (sweep balayé / rtl_power style).

  ==============================================================================
*/

#pragma once

#include <vector>

// A single power-vs-frequency sweep across [minMHz, maxMHz].
// powerDb[i] is the power in dB of the i-th bin; index 0 is the LOW end of the
// band (low frequencies first), so it maps left-to-right on the panel.
struct RFSpectrumSweep
{
    double minMHz = 470.0;
    double maxMHz = 870.0;
    double binHz  = 100000.0;          // 100 kHz resolution
    std::vector<float> powerDb;

    int    numBins() const { return (int)powerDb.size(); }
    double binMHz() const   { return binHz / 1.0e6; }

    int binForFreq(double freqMHz) const
    {
        if (powerDb.empty()) return -1;
        int b = (int)((freqMHz - minMHz) / binMHz());
        return (b < 0 || b >= (int)powerDb.size()) ? -1 : b;
    }

    double freqForBin(int bin) const { return minMHz + ((double)bin + 0.5) * binMHz(); }

    // Power at a frequency, or -200 dB if out of range.
    float powerAt(double freqMHz) const
    {
        int b = binForFreq(freqMHz);
        return b < 0 ? -200.0f : powerDb[(size_t)b];
    }

    // Linearly-interpolated power between adjacent bins, for smooth rendering when
    // the display has more pixels than the sweep has bins (avoids blocky columns).
    float powerAtInterp(double freqMHz) const
    {
        if (powerDb.empty()) return -200.0f;
        double x = (freqMHz - minMHz) / binMHz() - 0.5;   // position in bin-centre units
        if (x <= 0.0) return powerDb.front();
        if (x >= (double)(powerDb.size() - 1)) return powerDb.back();
        int i = (int)x;
        float t = (float)(x - i);
        return powerDb[(size_t)i] * (1.0f - t) + powerDb[(size_t)(i + 1)] * t;
    }
};

class RFSpectrumSource
{
public:
    virtual ~RFSpectrumSource() = default;

    // Scan [minMHz, maxMHz] producing bins of binHz width.
    virtual void start(double minMHz, double maxMHz, double binHz) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    // Reference band over which signals/carriers are laid out in absolute terms,
    // so a full-band scan and a zoomed-in detail scan stay consistent. Optional.
    virtual void setReferenceBand(double minMHz, double maxMHz) { (void)minMHz; (void)maxMHz; }

    // Copies the most recent sweep into out; returns false if none is available yet.
    virtual bool getLatestSweep(RFSpectrumSweep& out) = 0;
};
