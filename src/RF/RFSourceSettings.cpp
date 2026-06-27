/*
  ==============================================================================

    RFSourceSettings.cpp
    Created: 27 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFSourceSettings.h"
#include "RtlSdrSpectrumSource.h"

RFSourceSettings::RFSourceSettings() :
    ControllableContainer("Source")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;

    sourceType = addEnumParameter("Source Type", "Spectrum source used by the scan");
    sourceType->addOption("Simulated", (int)SIMULATED)->addOption("RTL-SDR", (int)RTLSDR);

    device = addEnumParameter("Device", "RTL-SDR dongle to use");
    device->isSavable = false;   // runtime list (names/order can change between sessions)
    refreshDevices = addTrigger("Refresh Devices", "Re-scan the connected RTL-SDR dongles");

    gainDb = addFloatParameter("Gain", "Tuner gain in dB", 30.0f, 0.0f, 50.0f);
    ppmCorrection = addIntParameter("PPM Correction", "Tuner frequency correction (ppm)", 0, -200, 200);
    scanBinKHz = addFloatParameter("Scan Resolution", "RTL-SDR bin width (kHz) at the widest view; zooming in goes finer", 1.0f, 0.2f, 200.0f);

    populateDevices();
}

RFSourceSettings::~RFSourceSettings()
{
}

void RFSourceSettings::populateDevices()
{
    const juce::String cur = device->getValueKey();
    device->clearOptions();

    if (!RtlSdrSpectrumSource::isSupported())
    {
        device->addOption("RTL-SDR not available", -1);
        return;
    }

    juce::StringArray names = RtlSdrSpectrumSource::listDevices();
    if (names.isEmpty())
    {
        device->addOption("No RTL-SDR found", -1);
        return;
    }

    for (int i = 0; i < names.size(); ++i)
        device->addOption(names[i], i);

    if (cur.isNotEmpty() && device->getAllKeys().contains(cur))
        device->setValueWithKey(cur, true);
}

int RFSourceSettings::selectedDeviceIndex() const
{
    return juce::jmax(0, (int)device->getValueData());
}

void RFSourceSettings::onContainerTriggerTriggered(Trigger* t)
{
    if (t == refreshDevices)
    {
        populateDevices();
        juce::StringArray names = RtlSdrSpectrumSource::isSupported() ? RtlSdrSpectrumSource::listDevices()
                                                                      : juce::StringArray();
        if (!RtlSdrSpectrumSource::isSupported()) NLOGWARNING(niceName, "RTL-SDR support is not compiled in.");
        else if (names.isEmpty())                 NLOGWARNING(niceName, "No RTL-SDR dongle found.");
        else for (auto& n : names)                NLOG(niceName, "RTL-SDR " + n);
    }
}

void RFSourceSettings::clear()
{
    sourceType->resetValue();
    gainDb->resetValue();
    ppmCorrection->resetValue();
    scanBinKHz->resetValue();
}
