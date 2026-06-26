/*
  ==============================================================================

    FrequencyOptimizer.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "FrequencyOptimizer.h"
#include <algorithm>

namespace
{
    // Median of a copy of the sweep, used as the noise-floor estimate.
    float estimateNoiseFloor(const RFSpectrumSweep& sweep)
    {
        if (sweep.powerDb.empty()) return -100.0f;
        std::vector<float> v = sweep.powerDb;
        size_t mid = v.size() / 2;
        std::nth_element(v.begin(), v.begin() + (long)mid, v.end());
        return v[mid];
    }
}

FrequencyOptimizer::Result FrequencyOptimizer::optimize(const RFSpectrumSweep& sweep,
                                                        const std::vector<DeviceInput>& devices,
                                                        const Constraints& c)
{
    Result result;

    const double guardMHz   = c.guardKHz / 1000.0;
    const double spacingMHz  = c.minSpacingKHz / 1000.0;
    const float  occThreshold = estimateNoiseFloor(sweep) + (float)c.occupancyMarginDb;

    // A frequency is occupied if any bin within the guard exceeds the threshold.
    auto isOccupied = [&](double freqMHz) -> bool
    {
        if (sweep.powerDb.empty()) return false;
        const double bm = sweep.binMHz();
        int lo = (int)std::floor((freqMHz - guardMHz - sweep.minMHz) / bm);
        int hi = (int)std::ceil ((freqMHz + guardMHz - sweep.minMHz) / bm);
        lo = juce::jlimit(0, sweep.numBins() - 1, lo);
        hi = juce::jlimit(0, sweep.numBins() - 1, hi);
        for (int b = lo; b <= hi; ++b)
            if (sweep.powerDb[(size_t)b] > occThreshold) return true;
        return false;
    };

    // A frequency is legal if it falls inside one of the allowed ranges (or if no
    // range is declared, in which case assignment is unrestricted).
    auto isAllowed = [&](double freqMHz) -> bool
    {
        if (c.allowedRanges.empty()) return true;
        for (const auto& r : c.allowedRanges)
            if (freqMHz >= r.first && freqMHz <= r.second) return true;
        return false;
    };

    // --- Filter each device's candidates to the un-occupied ones, then order
    //     devices most-constrained-first (fewest candidates).
    struct Work
    {
        const DeviceInput* dev;
        std::vector<Candidate> cands;
    };
    std::vector<Work> work;
    work.reserve(devices.size());

    for (const auto& d : devices)
    {
        Work w; w.dev = &d;
        for (const auto& cand : d.candidates)
            if (isAllowed(cand.freqMHz) && !isOccupied(cand.freqMHz))
                w.cands.push_back(cand);

        if (w.cands.empty() && !d.candidates.empty())
            result.warnings.add(d.deviceId + ": no candidate frequency is both legal (allowed range) and free.");

        work.push_back(std::move(w));
    }

    std::stable_sort(work.begin(), work.end(),
                     [](const Work& a, const Work& b) { return a.cands.size() < b.cands.size(); });

    // --- IMD helpers --------------------------------------------------------
    const bool doImd = c.imdMode != IMD_OFF;
    const bool do3Tx = c.imdMode == IMD_2TX_3TX;

    std::vector<double> assigned;       // chosen carriers so far
    std::vector<double> assignedProd;   // intermod products among the assigned set

    auto recomputeProducts = [&]()
    {
        assignedProd.clear();
        const int m = (int)assigned.size();
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < m; ++j)
                if (i != j) assignedProd.push_back(2.0 * assigned[i] - assigned[j]);

        if (do3Tx)
            for (int i = 0; i < m; ++i)
                for (int j = i + 1; j < m; ++j)
                    for (int k = 0; k < m; ++k)
                        if (k != i && k != j)
                            assignedProd.push_back(assigned[i] + assigned[j] - assigned[k]);
    };

    auto near = [&](double a, double b) { return std::abs(a - b) <= guardMHz; };

    // Is candidate f compatible with the already-assigned carriers?
    auto candidateOk = [&](double f) -> bool
    {
        for (double a : assigned)
            if (std::abs(f - a) < spacingMHz) return false;   // spacing

        if (!doImd) return true;

        // case 1: f must not land on an existing product among the assigned set.
        for (double p : assignedProd)
            if (near(f, p)) return false;

        // carriers including the candidate
        std::vector<double> S = assigned; S.push_back(f);

        // case 2 (2-Tx): new products involving f must not land on any carrier.
        for (double a : assigned)
        {
            double p1 = 2.0 * f - a;
            double p2 = 2.0 * a - f;
            for (double s : S)
                if (near(p1, s) || near(p2, s)) return false;
        }

        // case 2 (3-Tx): triples involving f.
        if (do3Tx)
        {
            const int m = (int)assigned.size();
            for (int i = 0; i < m; ++i)
                for (int j = i + 1; j < m; ++j)
                {
                    double pa = f + assigned[i] - assigned[j];
                    double pb = f + assigned[j] - assigned[i];
                    double pc = assigned[i] + assigned[j] - f;
                    for (double s : S)
                        if (near(pa, s) || near(pb, s) || near(pc, s)) return false;
                }
        }

        return true;
    };

    // --- Greedy assignment --------------------------------------------------
    for (auto& w : work)
    {
        Assignment as;
        as.deviceId = w.dev->deviceId;

        double bestF = 0.0; juce::String bestName; double bestScore = -1.0e18; bool found = false;

        for (const auto& cand : w.cands)
        {
            if (!candidateOk(cand.freqMHz)) continue;

            // Prefer candidates far from the carriers already placed (spreads the
            // set out, which also reduces future IMD pressure) and in quieter spectrum.
            double minDist = 1.0e9;
            for (double a : assigned) minDist = juce::jmin(minDist, std::abs(cand.freqMHz - a));
            if (assigned.empty()) minDist = 0.0;
            double score = minDist - 0.001 * (double)sweep.powerAt(cand.freqMHz);

            if (score > bestScore) { bestScore = score; bestF = cand.freqMHz; bestName = cand.presetName; found = true; }
        }

        if (found)
        {
            as.freqMHz = bestF;
            as.presetName = bestName;
            as.placed = true;
            assigned.push_back(bestF);
            recomputeProducts();
        }
        else
        {
            as.placed = false;
            if (!w.cands.empty())
                result.warnings.add(w.dev->deviceId + ": no conflict-free frequency found (spacing / intermodulation).");
            else if (w.dev->candidates.empty())
                result.warnings.add(w.dev->deviceId + ": no candidate frequencies defined.");
        }

        result.assignments.push_back(as);
    }

    return result;
}
