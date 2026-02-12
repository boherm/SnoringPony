/*
  ==============================================================================

    OSCCue.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCCue.h"
#include "OSCCueMessage.h"
#include "../../Interface/InterfaceManager.h"
#include "../../Interface/osc/OSCInterface.h"
#include "../../Interface/osc/OSCCommand.h"

OSCCue::OSCCue(var params) :
    Cue(params)
{
	itemDataType = "OSC Cue";
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    messagesManager.reset(new BaseManager<OSCCueMessage>("OSC Messages"));
    messagesManager->addBaseManagerListener(this);
    addChildControllableContainer(messagesManager.get());

    if (messagesManager->items.isEmpty()) {
        messagesManager->addItemFromData(var());
    }
}

OSCCue::~OSCCue()
{
    messagesManager->removeBaseManagerListener(this);
}

void OSCCue::playInternal()
{
    for (OSCCueMessage* msg : messagesManager->items) {
        ControllableContainer* targetContainer = msg->targetTemplate->getTargetContainer();
        OSCCommand* oscCommand = dynamic_cast<OSCCommand*>(targetContainer);
        OSCInterface* oscInterface = dynamic_cast<OSCInterface*>(targetContainer);

        if (oscCommand != nullptr) {
            oscInterface = oscCommand->interface;
        }

        OSCMessage oscMsg = msg->buildMessage();
        oscInterface->sendOSC(oscMsg);
    }
    endCue();
}

void OSCCue::itemAdded(OSCCueMessage* newItem)
{
    newItem->cue = this;
    newItem->warningResolveInspectable = this;
}

void OSCCue::itemsAdded(juce::Array<OSCCueMessage *> newItems)
{
    for (OSCCueMessage* msg : newItems) {
        msg->cue = this;
        msg->warningResolveInspectable = this;
    }
}

String OSCCue::autoDescriptionInternal()
{
    String desc = "OSC Cue: ";

    for (int i = 0; i < messagesManager->items.size(); i++)
    {
        if (i > 0)
            desc += ", ";
        OSCCueMessage* msg = messagesManager->items[i];
        desc += msg->address->stringValue();

        if (!msg->argumentsContainer->items.isEmpty()) {
            desc += " (";

            for (int y = 0; y < msg->argumentsContainer->items.size(); y++) {
                if (y > 0)
                    desc += ", ";

                GenericControllableItem* p = static_cast<GenericControllableItem*>(msg->argumentsContainer->items[y]);
                auto param = static_cast<Parameter*>(p->controllable);
                desc += param->stringValue();
            }

            desc += ")";
        }
    }

    return desc;
}
