/*
  ==============================================================================

    MIDIInterface.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "MIDIInterface.h"

MIDIInterface::MIDIInterface() :
    Interface("MIDI Interface 1")
{
    logOutgoingData->hideInEditor = true;

    deviceName = addStringParameter("Input Device", "Currently selected MIDI input device", "(none)");
    deviceName->setControllableFeedbackOnly(true);

    deviceIdentifier = addStringParameter("Device ID", "Stored device identifier for reconnection", "");
    deviceIdentifier->hideInEditor = true;

    selectDeviceBtn = addTrigger("Select Input Device", "Pick a MIDI input device");

    autoAdd = addBoolParameter("Auto-add", "When enabled, automatically creates a new mapping for each unrecognised MIDI event received", false);

    outputDeviceName = addStringParameter("Output Device", "Currently selected MIDI output device", "(none)");
    outputDeviceName->setControllableFeedbackOnly(true);

    outputDeviceIdentifier = addStringParameter("Output Device ID", "Stored output device identifier for reconnection", "");
    outputDeviceIdentifier->hideInEditor = true;

    selectOutputDeviceBtn = addTrigger("Select Output Device", "Pick a MIDI output device");

    mappings.reset(new BaseManager<MIDIMapping>("Mappings"));
    mappings->selectItemWhenCreated = false;
    addChildControllableContainer(mappings.get());

    MIDIManager::getInstance()->init();
    MIDIManager::getInstance()->addMIDIManagerListener(this);
}

MIDIInterface::~MIDIInterface()
{
    if (currentDevice != nullptr)
        currentDevice->removeMIDIInputListener(this);

    if (currentOutputDevice != nullptr)
        currentOutputDevice->removeUser();

    if (MIDIManager::getInstanceWithoutCreating() != nullptr)
        MIDIManager::getInstance()->removeMIDIManagerListener(this);
}

void MIDIInterface::setInputDevice(MIDIInputDevice* device)
{
    if (currentDevice == device) return;

    if (currentDevice != nullptr)
        currentDevice->removeMIDIInputListener(this);

    currentDevice = device;

    if (currentDevice != nullptr)
    {
        currentDevice->addMIDIInputListener(this);
        deviceName->setValue(currentDevice->name);
        deviceIdentifier->setValue(currentDevice->id);
        clearWarning();
        NLOG(niceName, "Connected to MIDI device: " << currentDevice->name);
    }
    else
    {
        String storedId = deviceIdentifier->stringValue();
        if (storedId.isNotEmpty())
            setWarningMessage("MIDI device disconnected");
        else
        {
            deviceName->setValue("(none)");
            clearWarning();
        }
    }
}

void MIDIInterface::midiDeviceInAdded(MIDIInputDevice* d)
{
    // Auto-reconnect if this is the device we were using
    if (currentDevice == nullptr && d->id == deviceIdentifier->stringValue())
    {
        setInputDevice(d);
    }
}

void MIDIInterface::midiDeviceInRemoved(MIDIInputDevice* d)
{
    if (currentDevice == d)
    {
        currentDevice->removeMIDIInputListener(this);
        currentDevice = nullptr;
        setWarningMessage("MIDI device disconnected");
    }
}

void MIDIInterface::setOutputDevice(MIDIOutputDevice* device)
{
    if (currentOutputDevice == device) return;

    if (currentOutputDevice != nullptr)
        currentOutputDevice->removeUser();

    currentOutputDevice = device;

    if (currentOutputDevice != nullptr)
    {
        currentOutputDevice->addUser();
        outputDeviceName->setValue(currentOutputDevice->name);
        outputDeviceIdentifier->setValue(currentOutputDevice->id);
        NLOG(niceName, "Connected to MIDI output: " << currentOutputDevice->name);
    }
    else
    {
        String storedId = outputDeviceIdentifier->stringValue();
        if (storedId.isEmpty())
        {
            outputDeviceName->setValue("(none)");
        }
    }
}

void MIDIInterface::sendMessage(const juce::MidiMessage& msg)
{
    if (currentOutputDevice != nullptr)
    {
        currentOutputDevice->sendMessage(msg);

        if (logOutgoingData->boolValue())
        {
            String desc = msg.getDescription();
            MessageManager::callAsync([this, desc]()
            {
                NLOG(niceName, "MIDI Out: " << desc);
            });
        }
    }
}

void MIDIInterface::midiDeviceOutAdded(MIDIOutputDevice* d)
{
    if (currentOutputDevice == nullptr && d->id == outputDeviceIdentifier->stringValue())
    {
        setOutputDevice(d);
    }
}

void MIDIInterface::midiDeviceOutRemoved(MIDIOutputDevice* d)
{
    if (currentOutputDevice == d)
    {
        currentOutputDevice->removeUser();
        currentOutputDevice = nullptr;
    }
}

void MIDIInterface::midiMessageReceived(const juce::MidiMessage& message)
{
    if (message.isSysEx() || message.isMidiClock() || message.isActiveSense()
        || message.isMidiStart() || message.isMidiStop() || message.isMidiContinue())
        return;

    if (logIncomingData->boolValue())
    {
        String desc = message.getDescription();
        MessageManager::callAsync([this, desc]()
        {
            NLOG(niceName, "MIDI In: " << desc);
        });
    }

    if (autoAdd->boolValue())
    {
        juce::MidiMessage msg(message);
        bool hasMatch = (findExistingMapping(msg) != nullptr);
        if (!hasMatch)
        {
            MessageManager::callAsync([this, msg]()
            {
                createMappingFromMessage(msg);
            });
            return;
        }
    }

    for (auto* m : mappings->items)
    {
        if (m->matches(message))
        {
            WeakReference<Inspectable> safeMapping(m);
            MessageManager::callAsync([safeMapping]()
            {
                if (safeMapping != nullptr)
                    dynamic_cast<MIDIMapping*>(safeMapping.get())->executeActions();
            });
        }
    }
}

MIDIMapping* MIDIInterface::findExistingMapping(const juce::MidiMessage& msg) const
{
    for (auto* m : mappings->items)
        if (m->matches(msg)) return m;
    return nullptr;
}

MIDIMapping* MIDIInterface::createMappingFromMessage(const juce::MidiMessage& msg)
{
    MIDIMapping* m = new MIDIMapping();

    if (msg.isNoteOn())
        m->eventType->setValueWithData("NoteOn");
    else if (msg.isNoteOff())
        m->eventType->setValueWithData("NoteOff");
    else if (msg.isController())
        m->eventType->setValueWithData("CC");

    m->channel->setValue(msg.getChannel());

    if (msg.isController())
        m->number->setValue(msg.getControllerNumber());
    else
        m->number->setValue(msg.getNoteNumber());

    mappings->addItem(m);
    return m;
}

void MIDIInterface::triggerTriggered(Trigger* t)
{
    if (t == selectDeviceBtn)
    {
        PopupMenu menu;
        auto* manager = MIDIManager::getInstance();

        menu.addItem(1, "(none)");
        menu.addSeparator();

        for (int i = 0; i < manager->inputs.size(); ++i)
        {
            auto* d = manager->inputs[i];
            bool isCurrent = (d == currentDevice);
            menu.addItem(10 + i, d->name, true, isCurrent);
        }

        menu.showMenuAsync(PopupMenu::Options(),
            [this, manager](int result)
            {
                if (result <= 0) return;

                if (result == 1)
                {
                    deviceIdentifier->setValue("");
                    setInputDevice(nullptr);
                }
                else
                {
                    int idx = result - 10;
                    if (idx >= 0 && idx < manager->inputs.size())
                        setInputDevice(manager->inputs[idx]);
                }
            });
        return;
    }

    if (t == selectOutputDeviceBtn)
    {
        PopupMenu menu;
        auto* manager = MIDIManager::getInstance();

        menu.addItem(1, "(none)");
        menu.addSeparator();

        for (int i = 0; i < manager->outputs.size(); ++i)
        {
            auto* d = manager->outputs[i];
            bool isCurrent = (d == currentOutputDevice);
            menu.addItem(10 + i, d->name, true, isCurrent);
        }

        menu.showMenuAsync(PopupMenu::Options(),
            [this, manager](int result)
            {
                if (result <= 0) return;

                if (result == 1)
                {
                    outputDeviceIdentifier->setValue("");
                    setOutputDevice(nullptr);
                }
                else
                {
                    int idx = result - 10;
                    if (idx >= 0 && idx < manager->outputs.size())
                        setOutputDevice(manager->outputs[idx]);
                }
            });
        return;
    }

    Interface::triggerTriggered(t);
}

void MIDIInterface::loadJSONDataInternal(var data)
{
    Interface::loadJSONDataInternal(data);

    String storedId = deviceIdentifier->stringValue();
    if (storedId.isNotEmpty())
    {
        MIDIInputDevice* d = MIDIManager::getInstance()->getInputDeviceWithID(storedId);
        if (d != nullptr)
            setInputDevice(d);
        else
            setWarningMessage("MIDI device not available");
    }

    String storedOutputId = outputDeviceIdentifier->stringValue();
    if (storedOutputId.isNotEmpty())
    {
        MIDIOutputDevice* d = MIDIManager::getInstance()->getOutputDeviceWithID(storedOutputId);
        if (d != nullptr)
            setOutputDevice(d);
    }
}
