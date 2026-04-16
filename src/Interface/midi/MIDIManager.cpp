/*
  ==============================================================================

    MIDIManager.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MIDIManager.h"

juce_ImplementSingleton(MIDIManager)

MIDIManager::MIDIManager()
{
    checkDevices();
}

MIDIManager::~MIDIManager()
{
}

void MIDIManager::checkDevices()
{
    juce::Array<juce::MidiDeviceInfo> inputInfos = juce::MidiInput::getAvailableDevices();

    // Remove devices that are no longer present
    juce::Array<MIDIInputDevice*> toRemove;
    for (auto* d : inputs)
    {
        bool found = false;
        for (const auto& info : inputInfos)
        {
            if (info.identifier == d->id)
            {
                found = true;
                break;
            }
        }
        if (!found) toRemove.add(d);
    }

    for (auto* d : toRemove)
        removeInputDevice(d);

    // Add new devices
    for (const auto& info : inputInfos)
        addInputDeviceIfNotThere(info);
}

void MIDIManager::addInputDeviceIfNotThere(const juce::MidiDeviceInfo& info)
{
    if (getInputDeviceWithID(info.identifier) != nullptr) return;

    MIDIInputDevice* d = new MIDIInputDevice(info);
    inputs.add(d);
    NLOG("MIDI", "Device In Added : " << d->name);
    listeners.call(&Listener::midiDeviceInAdded, d);
}

void MIDIManager::removeInputDevice(MIDIInputDevice* d)
{
    inputs.removeObject(d, false);
    NLOG("MIDI", "Device In Removed : " << d->name);
    listeners.call(&Listener::midiDeviceInRemoved, d);
    delete d;
}

MIDIInputDevice* MIDIManager::getInputDeviceWithID(const juce::String& id)
{
    for (auto* d : inputs)
        if (d->id == id) return d;
    return nullptr;
}

MIDIInputDevice* MIDIManager::getInputDeviceWithName(const juce::String& name)
{
    for (auto* d : inputs)
        if (d->name == name) return d;
    return nullptr;
}
