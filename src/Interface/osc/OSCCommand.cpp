/*
  ==============================================================================

    OSCCommand.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCCommand.h"
#include "OSCInterface.h"
#include "juce_osc/juce_osc.h"
#include "ui/OSCCommandEditor.h"

OSCCommandArguments::OSCCommandArguments() :
    ControllableContainer("Arguments")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;
    userCanAddControllables = true;
    allowSameChildrenNiceNames = false,
    userAddControllablesFilters.add(IntParameter::getTypeStringStatic());
    userAddControllablesFilters.add(FloatParameter::getTypeStringStatic());
    userAddControllablesFilters.add(StringParameter::getTypeStringStatic());
    userAddControllablesFilters.add(BoolParameter::getTypeStringStatic());
}

// --------------------------------------------------------------------

OSCCommand::OSCCommand(String name):
    BaseItem(name)
{
    isSavable = true;
	isSelectable = false;
    saveAndLoadRecursiveData = true;

    testBtn = addTrigger("Test", "Trigger to send the OSC Message for testing");
    testBtn->hideInEditor = true;

    address = addStringParameter("Address", "Adress of the OSC Message (e.g. /example)", "/example");

    argumentsContainer.reset(new OSCCommandArguments());
    addChildControllableContainer(argumentsContainer.get());
}

void OSCCommand::onContainerTriggerTriggered(Trigger* t)
{
    if (t == testBtn)
    {
        execute();
    }
}

OSCMessage OSCCommand::buildMessage()
{
    OSCMessage msg(address->stringValue());

    for (Parameter* c : argumentsContainer->getAllParameters())
    {

        if (c->getTypeString() == IntParameter::getTypeStringStatic())
        {
            auto param = static_cast<IntParameter*>(c);
            msg.addInt32(param->intValue());
            continue;
        }

        if (c->getTypeString() == FloatParameter::getTypeStringStatic())
        {
            auto param = static_cast<FloatParameter*>(c);
            msg.addFloat32(param->floatValue());
            continue;
        }

        if (c->getTypeString() == StringParameter::getTypeStringStatic())
        {
            auto param = static_cast<StringParameter*>(c);
            msg.addString(param->stringValue());
            continue;
        }

        if (c->getTypeString() == BoolParameter::getTypeStringStatic())
        {
            auto param = static_cast<BoolParameter*>(c);
            msg.addBool(param->boolValue());
            continue;
        }
    }

    return msg;
}

void OSCCommand::execute()
{
    interface->sendOSC(buildMessage());
}

InspectableEditor* OSCCommand::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new OSCCommandEditor(this, isRoot);
}

// --------------------------------------------------------------------

OSCCommandManager::OSCCommandManager() :
    BaseManager<OSCCommand>("OSC Message Templates")
{
    isSelectable = false;
    saveAndLoadRecursiveData = true;
}

OSCCommandManager::~OSCCommandManager()
{
}

void OSCCommandManager::addItemInternal(OSCCommand* command, var data)
{
    command->interface = interface;
    command->warningResolveInspectable = interface;
}
