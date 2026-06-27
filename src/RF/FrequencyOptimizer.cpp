/*
  ==============================================================================

    FrequencyOptimizer.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "FrequencyOptimizer.h"
#include <algorithm>
#include <random>

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
                                                        const Constraints& c,
                                                        double maxSeconds,
                                                        std::function<bool(float)> onProgress)
{
    Result result;

    const double guardMHz   = c.guardKHz / 1000.0;
    const double spacingMHz  = c.minSpacingKHz / 1000.0;
    const float  occThreshold = estimateNoiseFloor(sweep) + (float)c.occupancyMarginDb;

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

    auto isAllowed = [&](double freqMHz) -> bool
    {
        if (c.allowedRanges.empty()) return true;
        for (const auto& r : c.allowedRanges)
            if (freqMHz >= r.first && freqMHz <= r.second) return true;
        return false;
    };

    // --- Filter each device's candidates to the legal / un-occupied ones.
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
        work.push_back(std::move(w));
    }

    const bool doImd = c.imdMode != IMD_OFF;
    const bool do3Tx = c.imdMode == IMD_2TX_3TX || c.imdMode == IMD_2TX_3TX_5TX;
    const bool do5Tx = c.imdMode == IMD_2TX_3TX_5TX;

    auto withinGuard = [&](double a, double b) { return std::abs(a - b) <= guardMHz; };

    // Intermod products among a set of assigned carriers.
    auto computeProducts = [&](const std::vector<double>& a, std::vector<double>& out)
    {
        out.clear();
        const int m = (int)a.size();
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < m; ++j)
                if (i != j)
                {
                    out.push_back(2.0 * a[i] - a[j]);           // 3rd order, 2-Tx
                    if (do5Tx) out.push_back(3.0 * a[i] - 2.0 * a[j]); // 5th order, 2-Tx
                }
        if (do3Tx)
            for (int i = 0; i < m; ++i)
                for (int j = i + 1; j < m; ++j)
                    for (int k = 0; k < m; ++k)
                        if (k != i && k != j)
                            out.push_back(a[i] + a[j] - a[k]);  // 3rd order, 3-Tx
    };

    // Is candidate f compatible with the already-assigned carriers?
    auto candidateOk = [&](double f, const std::vector<double>& assigned, const std::vector<double>& prod) -> bool
    {
        for (double a : assigned)
            if (std::abs(f - a) < spacingMHz) return false;       // spacing

        if (!doImd) return true;

        for (double p : prod)                                     // f on an existing product
            if (withinGuard(f, p)) return false;

        std::vector<double> S = assigned; S.push_back(f);

        for (double a : assigned)                                 // new 2-Tx products with f
        {
            double p3a = 2.0 * f - a, p3b = 2.0 * a - f;
            for (double s : S) if (withinGuard(p3a, s) || withinGuard(p3b, s)) return false;
            if (do5Tx)                                            // new 5th-order 2-Tx with f
            {
                double p5a = 3.0 * f - 2.0 * a, p5b = 3.0 * a - 2.0 * f;
                for (double s : S) if (withinGuard(p5a, s) || withinGuard(p5b, s)) return false;
            }
        }

        if (do3Tx)                                                // new 3-Tx products with f
        {
            const int m = (int)assigned.size();
            for (int i = 0; i < m; ++i)
                for (int j = i + 1; j < m; ++j)
                {
                    double pa = f + assigned[i] - assigned[j];
                    double pb = f + assigned[j] - assigned[i];
                    double pc = assigned[i] + assigned[j] - f;
                    for (double s : S) if (withinGuard(pa, s) || withinGuard(pb, s) || withinGuard(pc, s)) return false;
                }
        }

        return true;
    };

    // One greedy assignment given a device visiting order; randomize picks random
    // valid candidates (random-restart) instead of the deterministic best. Outputs
    // the sorted-ascending list of gaps between consecutive carriers (for leximin).
    auto runPass = [&](const std::vector<int>& order, juce::Random& rng, bool randomize,
                       std::vector<Assignment>& out, int& placed, std::vector<double>& gaps)
    {
        std::vector<double> assigned, prod;
        out.assign(work.size(), {});
        for (size_t wi = 0; wi < work.size(); ++wi) out[wi].deviceId = work[wi].dev->deviceId;

        placed = 0;
        for (int idx : order)
        {
            const Work& w = work[(size_t)idx];
            double bestF = 0.0; juce::String bestName; bool found = false;

            if (randomize)
            {
                // Random-restart: pick a random valid candidate so different passes
                // genuinely explore different assignments (not just orderings).
                std::vector<int> valid;
                for (int ci = 0; ci < (int)w.cands.size(); ++ci)
                    if (candidateOk(w.cands[(size_t)ci].freqMHz, assigned, prod))
                        valid.push_back(ci);

                if (!valid.empty())
                {
                    int ci = valid[(size_t)rng.nextInt((int)valid.size())];
                    bestF = w.cands[(size_t)ci].freqMHz;
                    bestName = w.cands[(size_t)ci].presetName;
                    found = true;
                }
            }
            else
            {
                // Deterministic: the candidate that's farthest from already-placed
                // carriers (best spread) and in the quietest spectrum.
                double bestScore = -1.0e18;
                for (const auto& cand : w.cands)
                {
                    if (!candidateOk(cand.freqMHz, assigned, prod)) continue;

                    double minDist = 1.0e9;
                    for (double a : assigned) minDist = juce::jmin(minDist, std::abs(cand.freqMHz - a));
                    if (assigned.empty()) minDist = 0.0;

                    double s = minDist - 0.001 * (double)sweep.powerAt(cand.freqMHz);
                    if (s > bestScore) { bestScore = s; bestF = cand.freqMHz; bestName = cand.presetName; found = true; }
                }
            }

            if (found)
            {
                out[(size_t)idx] = { w.dev->deviceId, bestF, bestName, true };
                assigned.push_back(bestF);
                computeProducts(assigned, prod);
                ++placed;
            }
        }

        // Quality of this solution = the gaps between consecutive carriers, sorted
        // ascending. Compared leximin (biggest smallest-gap first) so the spacing is
        // as even / safe as possible, not just spread to the band edges.
        std::sort(assigned.begin(), assigned.end());
        gaps.clear();
        for (size_t i = 1; i < assigned.size(); ++i)
            gaps.push_back(assigned[i] - assigned[i - 1]);
        std::sort(gaps.begin(), gaps.end());
    };

    // Leximin: a is better than b if, comparing the sorted gaps in order, the first
    // difference is larger in a (i.e. its smallest constrained gap is bigger).
    auto leximinBetter = [](const std::vector<double>& a, const std::vector<double>& b)
    {
        const size_t n = juce::jmin(a.size(), b.size());
        for (size_t i = 0; i < n; ++i)
        {
            if (a[i] > b[i] + 1.0e-9) return true;
            if (a[i] < b[i] - 1.0e-9) return false;
        }
        return a.size() > b.size();
    };

    // Base order: most-constrained-first (fewest candidates).
    std::vector<int> baseOrder(work.size());
    for (int i = 0; i < (int)work.size(); ++i) baseOrder[(size_t)i] = i;
    std::stable_sort(baseOrder.begin(), baseOrder.end(),
                     [&](int a, int b) { return work[(size_t)a].cands.size() < work[(size_t)b].cands.size(); });

    std::vector<Assignment> best;
    int bestPlaced = -1;
    std::vector<double> bestGaps;

    // Devices that even have a legal/free candidate (the upper bound for placement).
    int placeable = 0;
    for (auto& w : work) if (!w.cands.empty()) ++placeable;

    const double startMs  = juce::Time::getMillisecondCounterHiRes();
    const double budgetMs = juce::jmax(0.05, maxSeconds) * 1000.0;
    const double plateauMs = 2000.0;        // stop after this long with no improvement
    const int    safetyCap = 5'000'000;     // hard backstop against an infinite loop
    double lastImproveMs = startMs;

    for (int p = 0; p < safetyCap; ++p)
    {
        std::vector<int> order = baseOrder;
        juce::Random rng((juce::int64)((unsigned)p * 2654435761u) + 1);
        if (p > 0) std::shuffle(order.begin(), order.end(), std::mt19937((unsigned)(p * 40503u + 7)));

        std::vector<Assignment> res; int placed = 0; std::vector<double> gaps;
        runPass(order, rng, p > 0, res, placed, gaps);

        if (placed > bestPlaced || (placed == bestPlaced && leximinBetter(gaps, bestGaps)))
        {
            best = res; bestPlaced = placed; bestGaps = gaps;
            lastImproveMs = juce::Time::getMillisecondCounterHiRes();
        }

        // Nothing left to optimise once everything is placed and there's no spacing
        // to improve (0 or 1 carrier).
        if (bestPlaced >= placeable && placeable < 2) break;

        const double now = juce::Time::getMillisecondCounterHiRes();
        const double el = now - startMs;
        if (onProgress && !onProgress((float)juce::jlimit(0.0, 1.0, el / budgetMs))) break;   // cancelled
        if (el >= budgetMs) break;                          // time budget reached
        // We keep exploring even after placing everyone, to find more preferable
        // spacing; we stop once no better solution has appeared for a while.
        if (now - lastImproveMs >= plateauMs && p > 0) break;
    }

    result.assignments = best;

    // Warnings for whatever the best solution couldn't place.
    for (size_t wi = 0; wi < work.size(); ++wi)
    {
        if (best[wi].placed) continue;
        const Work& w = work[wi];
        if (w.dev->candidates.empty())
            result.warnings.add(w.dev->deviceId + ": no candidate frequencies defined.");
        else if (w.cands.empty())
            result.warnings.add(w.dev->deviceId + ": no candidate frequency is both legal (allowed range) and free.");
        else
            result.warnings.add(w.dev->deviceId + ": no conflict-free frequency found (spacing / intermodulation).");
    }

    return result;
}
