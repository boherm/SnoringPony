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
}

MIDIManager::~MIDIManager()
{
    connection = {};
}

void MIDIManager::init()
{
    if (initialized) return;
    initialized = true;

    checkDevices();

    connection = juce::MidiDeviceListConnection::make([this]
    {
        checkDevices();
    });
}

void MIDIManager::checkDevices()
{
    // --- Inputs ---
    juce::Array<juce::MidiDeviceInfo> inputInfos = juce::MidiInput::getAvailableDevices();

    juce::Array<MIDIInputDevice*> inputsToRemove;
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
        if (!found) inputsToRemove.add(d);
    }

    for (auto* d : inputsToRemove)
        removeInputDevice(d);

    for (const auto& info : inputInfos)
        addInputDeviceIfNotThere(info);

    // --- Outputs ---
    juce::Array<juce::MidiDeviceInfo> outputInfos = juce::MidiOutput::getAvailableDevices();

    juce::Array<MIDIOutputDevice*> outputsToRemove;
    for (auto* d : outputs)
    {
        bool found = false;
        for (const auto& info : outputInfos)
        {
            if (info.identifier == d->id)
            {
                found = true;
                break;
            }
        }
        if (!found) outputsToRemove.add(d);
    }

    for (auto* d : outputsToRemove)
        removeOutputDevice(d);

    for (const auto& info : outputInfos)
        addOutputDeviceIfNotThere(info);
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

// --- Output devices ---

void MIDIManager::addOutputDeviceIfNotThere(const juce::MidiDeviceInfo& info)
{
    if (getOutputDeviceWithID(info.identifier) != nullptr) return;

    MIDIOutputDevice* d = new MIDIOutputDevice(info);
    outputs.add(d);
    NLOG("MIDI", "Device Out Added : " << d->name);
    listeners.call(&Listener::midiDeviceOutAdded, d);
}

void MIDIManager::removeOutputDevice(MIDIOutputDevice* d)
{
    outputs.removeObject(d, false);
    NLOG("MIDI", "Device Out Removed : " << d->name);
    listeners.call(&Listener::midiDeviceOutRemoved, d);
    delete d;
}

MIDIOutputDevice* MIDIManager::getOutputDeviceWithID(const juce::String& id)
{
    for (auto* d : outputs)
        if (d->id == id) return d;
    return nullptr;
}

MIDIOutputDevice* MIDIManager::getOutputDeviceWithName(const juce::String& name)
{
    for (auto* d : outputs)
        if (d->name == name) return d;
    return nullptr;
}
