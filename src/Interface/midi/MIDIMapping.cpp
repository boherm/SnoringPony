/*
  ==============================================================================

    MIDIMapping.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MIDIMapping.h"
#include "../action/MappingActionFactory.h"

MIDIMapping::MIDIMapping(var params) :
    BaseItem(params.getProperty("name", "MIDI Mapping"))
{
    saveAndLoadRecursiveData = true;
    canBeDisabled = true;

    eventType = addEnumParameter("Event Type", "Type of MIDI event to match");
    eventType->addOption("Note On", "NoteOn");
    eventType->addOption("Note Off", "NoteOff");
    eventType->addOption("Control Change", "CC");

    channel = addIntParameter("Channel", "MIDI channel (0 = Any)", 0, 0, 16);
    number = addIntParameter("Number", "Note pitch or CC number", 0, 0, 127);

    actions.reset(new BaseManager<MappingAction>("Actions"));
    actions->managerFactory = &MappingActionFactory::getInstance()->factory;
    actions->selectItemWhenCreated = false;
    addChildControllableContainer(actions.get());
}

MIDIMapping::~MIDIMapping()
{
}

bool MIDIMapping::matches(const juce::MidiMessage& msg) const
{
    if (!enabled->boolValue()) return false;

    String type = eventType->getValueData().toString();

    if (type == "NoteOn" && !msg.isNoteOn()) return false;
    if (type == "NoteOff" && !msg.isNoteOff()) return false;
    if (type == "CC" && !msg.isController()) return false;

    int ch = channel->intValue();
    if (ch > 0 && msg.getChannel() != ch) return false;

    int num = number->intValue();
    if (type == "CC")
    {
        if (msg.getControllerNumber() != num) return false;
    }
    else
    {
        if (msg.getNoteNumber() != num) return false;
    }

    return true;
}

void MIDIMapping::executeActions()
{
    for (auto* action : actions->items)
        action->execute();
}
