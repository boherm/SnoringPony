/*
  ==============================================================================

    RFCoordinationSettings.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFCoordinationSettings.h"
#include "RFDeviceManager.h"
#include "RtlSdrSpectrumSource.h"

RFCoordinationSettings::RFCoordinationSettings() :
    ControllableContainer("RF Coordination Settings")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;

    // Legally usable assignment ranges (e.g. 823-832 MHz). Empty = unrestricted.
    // Declared above the devices so the legal frame is set first.
    allowedRanges.reset(new BaseManager<AllowedRange>("Allowed Ranges"));
    allowedRanges->selectItemWhenCreated = false;
    addChildControllableContainer(allowedRanges.get());

    // The declared devices (mics + IEMs) live here so they are saved with the show.
    // Hidden from the settings editor since they are edited in the RF Coordination
    // panel; this only hides the generic editor, the panel's manager UI still shows them.
    addChildControllableContainer(RFDeviceManager::getInstance());
    RFDeviceManager::getInstance()->hideInEditor = true;

    scanMinMHz = addFloatParameter("Scan Min", "Lower edge of the scanned band, in MHz", 470.0f, 1.0f, 6000.0f);
    scanMaxMHz = addFloatParameter("Scan Max", "Upper edge of the scanned band, in MHz", 870.0f, 1.0f, 6000.0f);

    sourceType = addEnumParameter("Source", "Spectrum source used by the scan");
    sourceType->addOption("Simulated", (int)SIMULATED)->addOption("RTL-SDR", (int)RTLSDR);

    deviceIndex = addIntParameter("Device Index", "RTL-SDR dongle index (0 = first)", 0, 0, 16);
    autoGain = addBoolParameter("Auto Gain", "Let the tuner choose its gain", true);
    gainDb = addFloatParameter("Gain", "Manual tuner gain in dB (when Auto Gain is off)", 30.0f, 0.0f, 50.0f);
    ppmCorrection = addIntParameter("PPM Correction", "Tuner frequency correction (ppm)", 0, -200, 200);
    scanBinKHz = addFloatParameter("Scan Resolution", "RTL-SDR scan bin width in kHz", 25.0f, 2.0f, 200.0f);
    listDevices = addTrigger("List Devices", "Log the connected RTL-SDR dongles");

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
}

RFCoordinationSettings::~RFCoordinationSettings()
{
}

void RFCoordinationSettings::onContainerTriggerTriggered(Trigger* t)
{
    if (t == listDevices)
    {
        if (!RtlSdrSpectrumSource::isSupported())
        {
            NLOGWARNING(niceName, "RTL-SDR support is not compiled in (librtlsdr not vendored).");
            return;
        }
        juce::StringArray devs = RtlSdrSpectrumSource::listDevices();
        if (devs.isEmpty()) NLOGWARNING(niceName, "No RTL-SDR dongle found.");
        else for (auto& d : devs) NLOG(niceName, "RTL-SDR " + d);
    }
}

void RFCoordinationSettings::clear()
{
    RFDeviceManager::getInstance()->clear();

    scanMinMHz->resetValue();
    scanMaxMHz->resetValue();
    sourceType->resetValue();
    deviceIndex->resetValue();
    autoGain->resetValue();
    gainDb->resetValue();
    ppmCorrection->resetValue();
    scanBinKHz->resetValue();
    minSpacingKHz->resetValue();
    guardKHz->resetValue();
    imdOrder->resetValue();
    occupancyMarginDb->resetValue();
    occupancyCooldownSec->resetValue();
    allowedRanges->clear();
}
