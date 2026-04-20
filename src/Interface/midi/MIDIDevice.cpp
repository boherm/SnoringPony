/*
  ==============================================================================

    MIDIDevice.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MIDIDevice.h"

MIDIInputDevice::MIDIInputDevice(const juce::MidiDeviceInfo& info) :
    id(info.identifier),
    name(info.name)
{
}

MIDIInputDevice::~MIDIInputDevice()
{
    if (device != nullptr) device->stop();
}

void MIDIInputDevice::addMIDIInputListener(Listener* newListener)
{
    inputListeners.add(newListener);
    if (inputListeners.size() == 1)
    {
        device.reset();
        device = juce::MidiInput::openDevice(id, this);
        if (device != nullptr)
        {
            device->start();
            DBG("MIDI In " + device->getName() + " opened");
        }
        else
        {
            DBG("MIDI In " + name + " open error!");
        }
    }
}

void MIDIInputDevice::removeMIDIInputListener(Listener* listener)
{
    inputListeners.remove(listener);
    if (inputListeners.size() == 0)
    {
        if (device != nullptr) device->stop();
        device.reset();
        DBG("MIDI In " + name + " closed");
    }
}

void MIDIInputDevice::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    if (source != device.get()) return;
    inputListeners.call(&Listener::midiMessageReceived, message);
}

// -------------------------------------------------------
// MIDIOutputDevice
// -------------------------------------------------------

MIDIOutputDevice::MIDIOutputDevice(const juce::MidiDeviceInfo& info) :
    id(info.identifier),
    name(info.name)
{
}

MIDIOutputDevice::~MIDIOutputDevice()
{
    device.reset();
}

void MIDIOutputDevice::addUser()
{
    useCount++;
    if (useCount == 1)
    {
        device = juce::MidiOutput::openDevice(id);
        if (device != nullptr)
            DBG("MIDI Out " + device->getName() + " opened");
        else
            DBG("MIDI Out " + name + " open error!");
    }
}

void MIDIOutputDevice::removeUser()
{
    useCount = juce::jmax(0, useCount - 1);
    if (useCount == 0)
    {
        device.reset();
        DBG("MIDI Out " + name + " closed");
    }
}

void MIDIOutputDevice::sendMessage(const juce::MidiMessage& message)
{
    if (device != nullptr)
        device->sendMessageNow(message);
}
