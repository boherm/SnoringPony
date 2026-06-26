/*
  ==============================================================================

    FrequencyOptimizer.h
    Created: 26 Jun 2026
    Author:  boherm

    Pure (UI-free) frequency coordinator. Given a measured occupancy spectrum and
    each device's candidate frequencies, it assigns one frequency per device while
    avoiding occupied spectrum, keeping a minimum spacing between carriers, and
    avoiding 3rd-order intermodulation products landing on a used carrier.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "RFSpectrumSource.h"
#include <vector>
#include <functional>

class FrequencyOptimizer
{
public:
    enum ImdMode { IMD_OFF = 0, IMD_2TX = 1, IMD_2TX_3TX = 2, IMD_2TX_3TX_5TX = 3 };

    struct Candidate { double freqMHz; juce::String presetName; };

    struct DeviceInput
    {
        juce::String deviceId;            // RFDevice niceName (for reporting)
        std::vector<Candidate> candidates;
    };

    struct Constraints
    {
        double minSpacingKHz   = 400.0;
        double guardKHz        = 100.0;   // guard around occupied peaks and IMD products
        int    imdMode         = IMD_2TX_3TX;
        double occupancyMarginDb = 12.0;  // power above noise floor that counts as "occupied"

        // Legally usable ranges (min,max in MHz). Empty = unrestricted.
        std::vector<std::pair<double, double>> allowedRanges;
    };

    struct Assignment
    {
        juce::String deviceId;
        double       freqMHz = 0.0;
        juce::String presetName;
        bool         placed  = false;
    };

    struct Result
    {
        std::vector<Assignment> assignments;
        juce::StringArray       warnings;
    };

    // Runs as many greedy attempts (random-restart) as it can and keeps the best
    // (most devices placed, then best spacing). It stops when every placeable device
    // is placed, after a plateau with no improvement, or at maxSeconds. onProgress,
    // if set, is called with a 0..1 fraction and should return false to cancel.
    static Result optimize(const RFSpectrumSweep& sweep,
                           const std::vector<DeviceInput>& devices,
                           const Constraints& c,
                           double maxSeconds = 1.0,
                           std::function<bool(float)> onProgress = {});
};
