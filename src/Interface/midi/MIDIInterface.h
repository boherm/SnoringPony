/*
  ==============================================================================

    MIDIInterface.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

    MIDI input interface: listens on a selected MIDI device and dispatches
    incoming messages to MIDIMappings which trigger MappingActions.

  ==============================================================================
*/

#pragma once

#include "../Interface.h"
#include "MIDIMapping.h"
#include "MIDIManager.h"

class MIDIInterface :
    public Interface,
    public MIDIManager::Listener,
    public MIDIInputDevice::Listener
{
public:
    MIDIInterface();
    ~MIDIInterface();

    StringParameter* deviceName;
    StringParameter* deviceIdentifier;
    Trigger* selectDeviceBtn;
    BoolParameter* autoAdd;

    MIDIInputDevice* currentDevice = nullptr;

    StringParameter* outputDeviceName;
    StringParameter* outputDeviceIdentifier;
    Trigger* selectOutputDeviceBtn;
    MIDIOutputDevice* currentOutputDevice = nullptr;

    std::unique_ptr<BaseManager<MIDIMapping>> mappings;

    void setInputDevice(MIDIInputDevice* device);
    void setOutputDevice(MIDIOutputDevice* device);
    void sendMessage(const juce::MidiMessage& msg);

    // MIDIManager::Listener
    void midiDeviceInAdded(MIDIInputDevice* d) override;
    void midiDeviceInRemoved(MIDIInputDevice* d) override;
    void midiDeviceOutAdded(MIDIOutputDevice* d) override;
    void midiDeviceOutRemoved(MIDIOutputDevice* d) override;

    // MIDIInputDevice::Listener
    void midiMessageReceived(const juce::MidiMessage& message) override;

    MIDIMapping* findExistingMapping(const juce::MidiMessage& msg) const;
    MIDIMapping* createMappingFromMessage(const juce::MidiMessage& msg);

    void triggerTriggered(Trigger* t) override;
    void loadJSONDataInternal(var data) override;

    String getTypeString() const override { return "MIDI"; }
    static MIDIInterface* create(var params) { return new MIDIInterface(); }
};
