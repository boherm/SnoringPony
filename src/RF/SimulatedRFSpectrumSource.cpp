/*
  ==============================================================================

    SimulatedRFSpectrumSource.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "SimulatedRFSpectrumSource.h"

SimulatedRFSpectrumSource::SimulatedRFSpectrumSource() :
    juce::Thread("RF Sim Sweep")
{
}

SimulatedRFSpectrumSource::~SimulatedRFSpectrumSource()
{
    stop();
}

void SimulatedRFSpectrumSource::setReferenceBand(double minMHz, double maxMHz)
{
    refMinMHz = juce::jmin(minMHz, maxMHz);
    refMaxMHz = juce::jmax(minMHz, maxMHz);
    buildCarriers();
}

void SimulatedRFSpectrumSource::start(double minMHz, double maxMHz, double binHz)
{
    stop();

    bandMinMHz = juce::jmin(minMHz, maxMHz);
    bandMaxMHz = juce::jmax(minMHz, maxMHz);
    if (bandMaxMHz - bandMinMHz < 0.1) bandMaxMHz = bandMinMHz + 0.1;
    targetBinHz = juce::jmax(1000.0, binHz);

    {
        const juce::ScopedLock sl(lock);
        hasData = false;
    }

    // Lay carriers over the reference band (defaults to the scan window if unset),
    // so carriers stay at fixed absolute frequencies across full/detail scans.
    if (refMaxMHz <= refMinMHz) { refMinMHz = bandMinMHz; refMaxMHz = bandMaxMHz; }
    buildCarriers();

    frame = 0;
    running.store(true);
    startThread();
}

void SimulatedRFSpectrumSource::stop()
{
    running.store(false);
    stopThread(500);
}

void SimulatedRFSpectrumSource::buildCarriers()
{
    carriers.clear();

    // A handful of "occupied" carriers spread across the band: TV-like wide blocks
    // (steady) and narrow wireless-mic-like spikes (wandering, fluctuating, some
    // intermittent). Levels are dBFS-ish relative to a ~ -98 dB noise floor.
    //                  freq        lvl    width  drift  dSpeed phase  lvlMod lSpeed blink
    const double span = refMaxMHz - refMinMHz;
    auto at = [&](double frac) { return refMinMHz + frac * span; };

    carriers.push_back({ at(0.06), -28.0, 0.20, 0.02, 0.15, 0.0,  2.0, 0.4, 0.0 });  // wide block, steady
    carriers.push_back({ at(0.18), -42.0, 0.04, 0.30, 0.90, 1.1,  6.0, 1.7, 0.0 });  // wandering spike
    carriers.push_back({ at(0.31), -35.0, 0.05, 0.18, 0.55, 2.3,  4.0, 1.1, 0.35 }); // intermittent
    carriers.push_back({ at(0.44), -25.0, 0.25, 0.03, 0.20, 0.7,  2.0, 0.3, 0.0 });  // wide block, steady
    carriers.push_back({ at(0.57), -40.0, 0.04, 0.45, 1.20, 3.0,  8.0, 2.3, 0.0 });  // fast wandering spike
    carriers.push_back({ at(0.69), -33.0, 0.06, 0.25, 0.70, 1.7,  5.0, 1.4, 0.22 }); // intermittent
    carriers.push_back({ at(0.80), -30.0, 0.22, 0.04, 0.18, 2.9,  3.0, 0.5, 0.0 });  // wide block
    carriers.push_back({ at(0.91), -44.0, 0.04, 0.35, 1.00, 0.4,  7.0, 1.9, 0.45 }); // intermittent spike

    // A lone interferer that slowly sweeps across the whole band.
    carriers.push_back({ at(0.5), -38.0, 0.05, span * 0.45, 0.12, 1.4, 4.0, 0.8, 0.0 });
}

void SimulatedRFSpectrumSource::renderSweep(RFSpectrumSweep& sweep)
{
    sweep.minMHz = bandMinMHz;
    sweep.maxMHz = bandMaxMHz;
    sweep.binHz  = targetBinHz;

    const int n = juce::jmax(1, (int)std::round((bandMaxMHz - bandMinMHz) * 1.0e6 / sweep.binHz));
    sweep.powerDb.assign((size_t)n, 0.0f);

    const double noiseFloor = -98.0;

    // Slowly drift each carrier so the waterfall shows movement over time.
    const double t = (double)frame * 0.05; // ~time in seconds (50 ms / frame)

    // Pre-compute each carrier's current centre/level/gate once per sweep so the
    // whole spectrum moves coherently frame to frame.
    struct Live { double fc; double level; bool on; };
    std::vector<Live> live;
    live.reserve(carriers.size());
    for (const auto& c : carriers)
    {
        const double fc = c.freqMHz + c.driftMHz * std::sin(t * c.driftSpeed + c.phase);
        double level = c.levelDb + c.levelModDb * std::sin(t * c.levelSpeed + c.phase * 1.7);

        // Intermittent carriers fade out when their gate is low.
        bool on = true;
        if (c.blinkSpeed > 0.0)
        {
            const double g = std::sin(t * c.blinkSpeed + c.phase);
            if (g < -0.1) on = false;
            else level -= (1.0 - g) * 4.0; // soft fade near the edges
        }
        live.push_back({ fc, level, on });
    }

    for (int i = 0; i < n; ++i)
    {
        const double f = sweep.freqForBin(i);

        // Noise floor with a little random texture.
        double db = noiseFloor + (rng.nextFloat() - 0.5f) * 3.0;

        for (size_t ci = 0; ci < live.size(); ++ci)
        {
            if (!live[ci].on) continue;
            const double d  = (f - live[ci].fc) / juce::jmax(1.0e-4, carriers[ci].widthMHz);
            const double peak = live[ci].level - 12.0 * d * d; // gaussian-ish skirt
            db = juce::jmax(db, peak);
        }

        sweep.powerDb[(size_t)i] = (float)db;
    }
}

void SimulatedRFSpectrumSource::run()
{
    while (!threadShouldExit() && running.load())
    {
        RFSpectrumSweep sweep;
        renderSweep(sweep);

        {
            const juce::ScopedLock sl(lock);
            latest = sweep;
            hasData = true;
        }

        ++frame;
        wait(50); // ~20 sweeps / second
    }
}

bool SimulatedRFSpectrumSource::getLatestSweep(RFSpectrumSweep& out)
{
    const juce::ScopedLock sl(lock);
    if (!hasData) return false;
    out = latest;
    return true;
}
