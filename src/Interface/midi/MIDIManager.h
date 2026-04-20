/*
  ==============================================================================

    MIDIManager.h
    Created: 16 Apr 2026
    Author:  boherm

    Singleton that manages MIDI input devices with hot-plug support.
    Adapted from Chataigne's MIDIManager (Ben Kuper).

  ==============================================================================
*/

#pragma once

#include "MIDIDevice.h"

class MIDIManager
{
public:
    juce_DeclareSingleton(MIDIManager, true)

    MIDIManager();
    ~MIDIManager();

    bool initialized = false;
    void init();

    juce::OwnedArray<MIDIInputDevice> inputs;
    juce::OwnedArray<MIDIOutputDevice> outputs;

    void checkDevices();

    void addInputDeviceIfNotThere(const juce::MidiDeviceInfo& info);
    void removeInputDevice(MIDIInputDevice* d);
    MIDIInputDevice* getInputDeviceWithID(const juce::String& id);
    MIDIInputDevice* getInputDeviceWithName(const juce::String& name);

    void addOutputDeviceIfNotThere(const juce::MidiDeviceInfo& info);
    void removeOutputDevice(MIDIOutputDevice* d);
    MIDIOutputDevice* getOutputDeviceWithID(const juce::String& id);
    MIDIOutputDevice* getOutputDeviceWithName(const juce::String& name);

    class Listener
    {
    public:
        virtual ~Listener() {}
        virtual void midiDeviceInAdded(MIDIInputDevice*) {}
        virtual void midiDeviceInRemoved(MIDIInputDevice*) {}
        virtual void midiDeviceOutAdded(MIDIOutputDevice*) {}
        virtual void midiDeviceOutRemoved(MIDIOutputDevice*) {}
    };

    juce::ListenerList<Listener> listeners;
    void addMIDIManagerListener(Listener* l) { listeners.add(l); }
    void removeMIDIManagerListener(Listener* l) { listeners.remove(l); }

    juce::MidiDeviceListConnection connection;

    JUCE_DECLARE_NON_COPYABLE(MIDIManager)
};
