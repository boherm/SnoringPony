/*
  ==============================================================================

    RFSourceSettings.h
    Created: 27 Jun 2026
    Author:  boherm

    "Source" sub-container of the RF Coordination settings: everything about the
    spectrum source (simulated vs RTL-SDR) and the dongle (device picker, gain,
    ppm, scan resolution), grouped so the RF settings stay tidy.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class RFSourceSettings :
    public ControllableContainer
{
public:
    RFSourceSettings();
    virtual ~RFSourceSettings();

    enum Source { SIMULATED = 0, RTLSDR = 1 };

    EnumParameter*  sourceType;
    EnumParameter*  device;            // detected RTL-SDR dongles (dropdown)
    Trigger*        refreshDevices;
    FloatParameter* gainDb;
    IntParameter*   ppmCorrection;
    FloatParameter* scanBinKHz;

    int  selectedDeviceIndex() const;
    void populateDevices();

    void onContainerTriggerTriggered(Trigger* t) override;
    void clear();
};
