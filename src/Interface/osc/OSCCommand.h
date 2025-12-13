/*
  ==============================================================================

    OSCCommand.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class OSCInterface;

class OSCCommandArguments :
    public GenericControllableManager
{
public:
    OSCCommandArguments();
    virtual ~OSCCommandArguments() {}

    BoolParameter* editable;
    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);
};

// -----------------------------------------------------

class OSCCommand :
    public BaseItem
{
public:
    OSCCommand(String name = "OSC Message 1");
    virtual ~OSCCommand() {}

    OSCInterface *interface;
    Trigger* testBtn;

    StringParameter* address;

    std::unique_ptr<OSCCommandArguments> argumentsContainer;

    void onContainerTriggerTriggered(Trigger* t) override;

    OSCMessage buildMessage();
    virtual void execute();

    String getTypeString() const override { return "Command"; }
    static OSCCommand* create(var params) { return new OSCCommand(params.getProperty("name", "OSC Message 1")); };
    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);
};
