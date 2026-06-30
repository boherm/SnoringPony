/*
  ==============================================================================

    MeteringSettings.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "MeteringSettings.h"
#include "../ui/panels/MeteringPanel.h"

MeteringSettings::MeteringSettings() :
    ControllableContainer("Metering Settings")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;

    deviceState = addStringParameter("Device State", "Saved audio input device configuration", "");
    deviceState->hideInEditor = true;

    audioSetup = addTrigger("Audio Setup", "Choose the input device and channels (JUCE audio setup)");

    calibration = addFloatParameter("SPL Calibration", "dB SPL corresponding to 0 dBFS", 100.0f, 0.0f, 140.0f);

    referenceLevel = addFloatParameter("Calibration Reference", "Known dB SPL of the current input, used by Auto-Calibrate", 40.0f, 30.0f, 140.0f);
    calibrate = addTrigger("Auto-Calibrate", "Set SPL Calibration so the current input level reads the reference above");

    weighting = addEnumParameter("Weighting", "Frequency weighting used for the dB SPL reading");
    weighting->addOption("dB(A)", (int)A)->addOption("dB(B)", (int)B)->addOption("dB(C)", (int)C);

    laeqWindow = addEnumParameter("LAeq Window", "Averaging window for the LAeq reading");
    laeqWindow->addOption("1 min", 60)->addOption("5 min", 300)->addOption("15 min", 900)->addOption("60 min", 3600);
    laeqWindow->setDefaultValue("15 min");

    threshold = addFloatParameter("Threshold", "dB SPL over which the reading is shown in red", 90.0f, 40.0f, 140.0f);

    resetMaxPeak = addTrigger("Reset Max/Peak", "Clear the held Max and Peak readouts");

    displayMode = addEnumParameter("Display Mode", "Left column: scrolling spectrogram, real-time analyzer (RTA), or dB SPL time graph");
    displayMode->addOption("Spectrogram", (int)Spectrogram)->addOption("RTA", (int)RTA)->addOption("dB SPL", (int)SPL);
    displayMode->setValueWithKey("RTA"); // default left column

    displayMode2 = addEnumParameter("Second Display", "Optional second column shown to the right (None to hide it)");
    displayMode2->addOption("None", -1)->addOption("Spectrogram", (int)Spectrogram)->addOption("RTA", (int)RTA)->addOption("dB SPL", (int)SPL);
    displayMode2->setValueWithKey("Spectrogram"); // default right column

    // Adjusted by dragging the splitter between the two columns; hidden from the inspector
    // but saved in the project file.
    splitPosition = addFloatParameter("Split Position", "Horizontal split between the two metering columns", 0.5f, 0.15f, 0.85f);
    splitPosition->hideInEditor = true;

    verticalOrientation = addBoolParameter("Vertical Spectrogram", "Spectrogram time axis vertical instead of horizontal", false);
    active = addBoolParameter("Active", "Whether the metering input is running", false);

    deviceManager.addChangeListener(this);
}

MeteringSettings::~MeteringSettings()
{
    deviceManager.removeChangeListener(this);
    deviceManager.closeAudioDevice();
}

void MeteringSettings::openDevice()
{
    ignoreDeviceCallbacks = true;
    std::unique_ptr<juce::XmlElement> xml;
    if (deviceState->stringValue().isNotEmpty())
        xml = juce::parseXML(deviceState->stringValue());

    deviceManager.initialise(64, 0, xml.get(), true);
    ignoreDeviceCallbacks = false;

    saveDeviceState();
}

void MeteringSettings::closeDevice()
{
    ignoreDeviceCallbacks = true;
    deviceManager.closeAudioDevice();
    ignoreDeviceCallbacks = false;
}

void MeteringSettings::reconfigure()
{
    if (active->boolValue()) openDevice();
    else closeDevice();
}

void MeteringSettings::saveDeviceState()
{
    std::unique_ptr<juce::XmlElement> xml = deviceManager.createStateXml();
    ignoreSettingsCallback = true;
    deviceState->setValue(xml != nullptr ? xml->toString() : String());
    ignoreSettingsCallback = false;
}

void MeteringSettings::openSetupDialog()
{
    // Make sure the device is open so the selector can configure it.
    if (deviceManager.getCurrentAudioDevice() == nullptr)
    {
        ignoreDeviceCallbacks = true;
        std::unique_ptr<juce::XmlElement> xml;
        if (deviceState->stringValue().isNotEmpty()) xml = juce::parseXML(deviceState->stringValue());
        deviceManager.initialise(64, 0, xml.get(), true);
        ignoreDeviceCallbacks = false;
    }

    // Up to 64 input channels so any input of a multi-channel interface can be
    // enabled individually (e.g. a measurement mic on input 3).
    auto* selector = new juce::AudioDeviceSelectorComponent(deviceManager, 1, 64, 0, 0, false, false, false, false);
    selector->setSize(460, 440);

    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned(selector);
    o.dialogTitle = "Metering Audio Setup";
    o.dialogBackgroundColour = BG_COLOR;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = false;
    o.resizable = true;

    juce::Component::SafePointer<juce::Component> safeThis;
    if (auto* dw = o.launchAsync())
        dw->setAlwaysOnTop(true);
}

void MeteringSettings::onContainerParameterChanged(Parameter* p)
{
    if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile) return;

    if (p == active) reconfigure();
}

void MeteringSettings::onContainerTriggerTriggered(Trigger* t)
{
    if (t == audioSetup) openSetupDialog();
    else if (t == calibrate)
    {
        // calibration (dB SPL at 0 dBFS) = reference SPL - current uncalibrated level,
        // so the "Now" reading then matches the known reference level. Needs a live
        // signal: when stopped the measured level is stale, so refuse to calibrate.
        auto* panel = MeteringPanel::getInstanceWithoutCreating();
        if (panel == nullptr || !panel->isInputRunning())
        {
            NLOGWARNING(niceName, "Auto-Calibrate: start the metering input first (no live signal).");
            return;
        }
        calibration->setValue(referenceLevel->floatValue() - panel->getMeasuredDbFs());
    }
    else if (t == resetMaxPeak)
    {
        if (auto* panel = MeteringPanel::getInstanceWithoutCreating())
            panel->resetMaxPeak();
    }
}

void MeteringSettings::changeListenerCallback(juce::ChangeBroadcaster*)
{
    if (ignoreDeviceCallbacks) return;
    saveDeviceState();
}

void MeteringSettings::clear()
{
    deviceState->resetValue();
    calibration->resetValue();
    referenceLevel->resetValue();
    weighting->resetValue();
    laeqWindow->resetValue();
    threshold->resetValue();
    displayMode->setValueWithKey("RTA");          // default: RTA on the left
    displayMode2->setValueWithKey("Spectrogram"); // default: spectrogram on the right
    splitPosition->resetValue();
    verticalOrientation->resetValue();
    active->resetValue();
}
