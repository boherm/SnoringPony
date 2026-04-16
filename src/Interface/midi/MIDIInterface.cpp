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

    selectDeviceBtn = addTrigger("Select Device", "Pick a MIDI input device");

    autoAdd = addBoolParameter("Auto-add", "When enabled, automatically creates a new mapping for each unrecognised MIDI event received", false);

    mappings.reset(new BaseManager<MIDIMapping>("Mappings"));
    mappings->selectItemWhenCreated = false;
    addChildControllableContainer(mappings.get());

    MIDIManager::getInstance()->addMIDIManagerListener(this);
}

MIDIInterface::~MIDIInterface()
{
    if (currentDevice != nullptr)
        currentDevice->removeMIDIInputListener(this);

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
}
