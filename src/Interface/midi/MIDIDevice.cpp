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
