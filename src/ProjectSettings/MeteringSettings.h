/*
  ==============================================================================

    MeteringSettings.h
    Created: 25 Jun 2026
    Author:  boherm

    Holds the metering configuration AND the input AudioDeviceManager. Everything
    is configured from the project settings (the input device is chosen through
    JUCE's own AudioDeviceSelectorComponent via the "Audio Setup" trigger). The
    metering input runs whenever "Active" is on. The panel is just a display with
    a Start/Stop button.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class MeteringSettings :
    public ControllableContainer,
    public juce::ChangeListener
{
public:
    MeteringSettings();
    virtual ~MeteringSettings();

    enum Weighting { A = 0, B = 1, C = 2 };

    juce::AudioDeviceManager deviceManager;

    StringParameter* deviceState;          // juce AudioDeviceManager XML (hidden)
    Trigger*         audioSetup;           // opens the JUCE audio device selector
    FloatParameter*  calibration;          // dB SPL at 0 dBFS
    EnumParameter*   weighting;            // A / B / C
    FloatParameter*  threshold;            // dB SPL over which the readout turns red
    BoolParameter*   verticalOrientation;
    BoolParameter*   active;

    void reconfigure(); // open/close the device to match "active"

    void onContainerParameterChanged(Parameter* p) override;
    void onContainerTriggerTriggered(Trigger* t) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void clear();

private:
    void openDevice();
    void closeDevice();
    void saveDeviceState();
    void openSetupDialog();

    bool ignoreDeviceCallbacks { false };
    bool ignoreSettingsCallback { false };
};
