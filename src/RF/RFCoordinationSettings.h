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

class RFCoordinationSettings :
    public ControllableContainer
{
public:
    RFCoordinationSettings();
    virtual ~RFCoordinationSettings();

    enum Source  { SIMULATED = 0, RTLSDR = 1 };
    enum ImdMode { IMD_OFF = 0, IMD_2TX = 1, IMD_2TX_3TX = 2, IMD_2TX_3TX_5TX = 3 };

    // Scan band
    FloatParameter* scanMinMHz;
    FloatParameter* scanMaxMHz;
    EnumParameter*  sourceType;

    // RTL-SDR hardware
    IntParameter*   deviceIndex;       // dongle index
    BoolParameter*  autoGain;
    FloatParameter* gainDb;            // manual tuner gain (when autoGain off)
    IntParameter*   ppmCorrection;     // frequency correction
    FloatParameter* scanBinKHz;        // hardware scan resolution
    Trigger*        listDevices;       // log connected dongles

    // Optimizer constraints
    FloatParameter* minSpacingKHz;
    FloatParameter* guardKHz;
    EnumParameter*  imdOrder;
    FloatParameter* occupancyMarginDb;
    FloatParameter* occupancyCooldownSec;   // peak-hold cool-down time

    // Legally usable assignment ranges; empty = unrestricted.
    std::unique_ptr<BaseManager<AllowedRange>> allowedRanges;

    void onContainerTriggerTriggered(Trigger* t) override;
    void clear();
};
