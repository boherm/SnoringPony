/*
  ==============================================================================

    MIDIDevice.h
    Created: 16 Apr 2026
    Author:  boherm

    Adapted from Chataigne's MIDI device infrastructure (Ben Kuper).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MIDIInputDevice :
    public juce::MidiInputCallback
{
public:
    MIDIInputDevice(const juce::MidiDeviceInfo& info);
    ~MIDIInputDevice();

    juce::String id;
    juce::String name;
    std::unique_ptr<juce::MidiInput> device;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    class Listener
    {
    public:
        virtual ~Listener() {}
        virtual void midiMessageReceived(const juce::MidiMessage& message) {}
    };

    juce::ListenerList<Listener> inputListeners;
    void addMIDIInputListener(Listener* newListener);
    void removeMIDIInputListener(Listener* listener);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIInputDevice)
};

class MIDIOutputDevice
{
public:
    MIDIOutputDevice(const juce::MidiDeviceInfo& info);
    ~MIDIOutputDevice();

    juce::String id;
    juce::String name;
    std::unique_ptr<juce::MidiOutput> device;

    int useCount = 0;

    void addUser();
    void removeUser();
    bool isOpen() const { return device != nullptr; }

    void sendMessage(const juce::MidiMessage& message);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIOutputDevice)
};
