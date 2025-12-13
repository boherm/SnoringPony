/*
  ==============================================================================

    OSCCommand.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCCommand.h"
#include "OSCInterface.h"
#include "ui/OSCCommandEditor.h"
#include "ui/OSCCommandArgumentsEditor.h"

OSCCommandArguments::OSCCommandArguments() :
    GenericControllableManager("Arguments", false, false, false, false)
{
    editable = addBoolParameter("Editable", "If true, arguments can be edited directly in cues", false);
    editable->hideInEditor = true;

    factory.defs.remove(6);
    factory.defs.remove(5);
    factory.defs.remove(4);
}

InspectableEditor* OSCCommandArguments::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new OSCCommandArgumentsEditor(this, isRoot);
}

// --------------------------------------------------------------------

OSCCommand::OSCCommand(String name):
    BaseItem(name)
{
    canBeDisabled = false;
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

    for (int i = 0; i < argumentsContainer->items.size(); i++)
    {
        GenericControllableItem* p = static_cast<GenericControllableItem*>(argumentsContainer->items[i]);
        auto param = static_cast<Parameter*>(p->controllable);

        if (IntParameter::getTypeStringStatic() == param->getTypeString())
        {
            msg.addInt32(param->intValue());
            continue;
        }

        if (FloatParameter::getTypeStringStatic() == param->getTypeString())
        {
            msg.addFloat32(param->floatValue());
            continue;
        }

        if (StringParameter::getTypeStringStatic() == param->getTypeString())
        {
            msg.addString(param->stringValue());
            continue;
        }

        if (BoolParameter::getTypeStringStatic() == param->getTypeString())
        {
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
