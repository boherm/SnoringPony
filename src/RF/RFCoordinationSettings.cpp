/*
  ==============================================================================

    RFCoordinationSettings.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFCoordinationSettings.h"
#include "RFDeviceManager.h"
#include "RFProfileManager.h"
#include "RtlSdrSpectrumSource.h"

RFCoordinationSettings::RFCoordinationSettings() :
    ControllableContainer("RF Coordination Settings")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;

    // Source + dongle settings grouped in a "Source" sub-container (shown first).
    source.reset(new RFSourceSettings());
    addChildControllableContainer(source.get());

    // Legally usable assignment ranges (e.g. 823-832 MHz). Empty = unrestricted.
    allowedRanges.reset(new BaseManager<AllowedRange>("Allowed Ranges"));
    allowedRanges->selectItemWhenCreated = false;
    addChildControllableContainer(allowedRanges.get());

    // Frequency profiles (assignable range / presets). Added before the devices so
    // device->profile references resolve on load. Edited in the panel, hidden here.
    addChildControllableContainer(RFProfileManager::getInstance());
    RFProfileManager::getInstance()->hideInEditor = true;

    // The declared devices (mics + IEMs) live here so they are saved with the show.
    // Hidden from the settings editor since they are edited in the RF Coordination
    // panel; this only hides the generic editor, the panel's manager UI still shows them.
    addChildControllableContainer(RFDeviceManager::getInstance());
    RFDeviceManager::getInstance()->hideInEditor = true;

    scanMinMHz = addFloatParameter("Scan Min", "Lower edge of the scanned band, in MHz", 470.0f, 1.0f, 6000.0f);
    scanMaxMHz = addFloatParameter("Scan Max", "Upper edge of the scanned band, in MHz", 870.0f, 1.0f, 6000.0f);

    minSpacingKHz = addFloatParameter("Min Spacing", "Minimum spacing between any two carriers, in kHz", 400.0f, 25.0f, 5000.0f);
    guardKHz = addFloatParameter("Guard", "Guard band around occupied peaks and intermodulation products, in kHz", 100.0f, 10.0f, 2000.0f);

    imdOrder = addEnumParameter("Intermodulation", "Intermodulation products to avoid");
    imdOrder->addOption("Off", (int)IMD_OFF)
            ->addOption("3rd order (2-Tx)", (int)IMD_2TX)
            ->addOption("3rd order (2-Tx + 3-Tx)", (int)IMD_2TX_3TX)
            ->addOption("3rd + 5th order", (int)IMD_2TX_3TX_5TX);
    imdOrder->setDefaultValue("3rd order (2-Tx + 3-Tx)");

    occupancyMarginDb = addFloatParameter("Occupancy Margin", "How far above the noise floor (dB) a signal counts as occupied", 12.0f, 1.0f, 60.0f);

    occupancyCooldownSec = addFloatParameter("Occupancy Cool-down", "Time (s) for the occupancy display to fade a transient/swept signal", 10.0f, 1.0f, 60.0f);

    scanDwellSec = addFloatParameter("Scan Dwell", "Seconds spent scanning each band window during the optimization scan (RTL-SDR)", 5.0f, 0.5f, 60.0f);
}

RFCoordinationSettings::~RFCoordinationSettings()
{
}

void RFCoordinationSettings::clear()
{
    RFDeviceManager::getInstance()->clear();
    RFProfileManager::getInstance()->clear();
    source->clear();

    scanMinMHz->resetValue();
    scanMaxMHz->resetValue();
    minSpacingKHz->resetValue();
    guardKHz->resetValue();
    imdOrder->resetValue();
    occupancyMarginDb->resetValue();
    occupancyCooldownSec->resetValue();
    scanDwellSec->resetValue();
    allowedRanges->clear();
}
