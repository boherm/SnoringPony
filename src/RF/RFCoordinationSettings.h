/*
  ==============================================================================

    RFCoordinationSettings.h
    Created: 26 Jun 2026
    Author:  boherm

    Project settings for the RF (wireless mic / IEM) frequency coordination
    feature. Owns the list of declared devices (RFDeviceManager) plus the scan
    band and the optimizer constraints. Everything here is saved with the show;
    the RF Coordination panel reads it to drive the scan and the optimizer.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "AllowedRange.h"
#include "RFSourceSettings.h"

class RFCoordinationSettings :
    public ControllableContainer
{
public:
    RFCoordinationSettings();
    virtual ~RFCoordinationSettings();

    enum ImdMode { IMD_OFF = 0, IMD_2TX = 1, IMD_2TX_3TX = 2, IMD_2TX_3TX_5TX = 3 };

    // Source + dongle settings, grouped in a "Source" sub-container.
    std::unique_ptr<RFSourceSettings> source;

    // Scan band
    FloatParameter* scanMinMHz;
    FloatParameter* scanMaxMHz;

    // Optimizer constraints
    FloatParameter* minSpacingKHz;
    FloatParameter* guardKHz;
    EnumParameter*  imdOrder;
    FloatParameter* occupancyMarginDb;
    FloatParameter* occupancyCooldownSec;   // peak-hold cool-down time
    FloatParameter* scanDwellSec;           // seconds per band window during the guided scan

    // Legally usable assignment ranges; empty = unrestricted.
    std::unique_ptr<BaseManager<AllowedRange>> allowedRanges;

    void clear();
};
