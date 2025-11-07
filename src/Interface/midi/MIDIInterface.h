/*
  ==============================================================================

    MIDIInterface.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../Interface.h"

class MIDIInterface :
    public Interface
{
public:
    MIDIInterface();
    ~MIDIInterface();

    // MIDIDeviceParameter* deviceParam;

    // MIDIMappingManager mappingManager;
    // MIDIFeedbackManager feedbackManager;
    // MIDIInputDevice* inputDevice;
    // MIDIOutputDevice* outputDevice;

    // BoolParameter* autoAdd;

    // IntParameter* numBytes;
    // ControllableContainer dataContainer;

    // StringParameter* infos;
    // HashMap<String, juce::uint32> TSLastReceived;
    // HashMap<String, int> delayedValue;

    // void updateDevices();
    // void updateBytesParams();
    // void onContainerParameterChangedInternal(Parameter *) override;

    // void sendStartupBytes();

    // void noteOnReceived(const int &channel, const int &pitch, const int &velocity) override;
    // void noteOffReceived(const int &channel, const int &pitch, const int &velocity) override;
    // void controlChangeReceived(const int& channel, const int& number, const int& value) override;
    // void pitchWheelReceived(const int& channel, const int& value) override;

    // void feedback(String address, var value, String origin);

    String getTypeString() const override { return "MIDI"; }
    static MIDIInterface* create(var params) { return new MIDIInterface(); };
};
