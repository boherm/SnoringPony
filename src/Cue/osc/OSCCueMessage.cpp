/*
  ==============================================================================

    OSCCueMessage.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCCueMessage.h"
#include "ui/OSCCueMessageEditor.h"
#include "../../Interface/osc/OSCInterface.h"
#include "../../Interface/osc/OSCCommand.h"
#include "../../Interface/InterfaceManager.h"

OSCCueMessageArguments::OSCCueMessageArguments() :
    GenericControllableManager("Arguments", false, false, false, false)
{
}

// -----------------------------------------------------

OSCCueMessage::OSCCueMessage(String name) :
    BaseItem(name)
{
    saveAndLoadRecursiveData = true;
    isSelectable = false;
    miniMode->setValue(true);

    testBtn = addTrigger("Test", "Send test OSC message", true);
    testBtn->hideInEditor = true;

    targetTemplate = addTargetParameter("OSC Template", "OSC Template to use for this message", InterfaceManager::getInstance());
    targetTemplate->targetType = TargetParameter::CONTAINER;
    std::function<void(ControllableContainer*, std::function<void(ControllableContainer*)>)> getListFunc = std::bind(&OSCCueMessage::showMenuForTargetOscMessage, this, std::placeholders::_1, std::placeholders::_2);
    targetTemplate->customGetTargetContainerFunc = getListFunc;
    targetTemplate->hideInEditor = true;

    address = addStringParameter("Address", "OSC Address", "", false);

    setWarningMessage("No OSC Template selected");

    argumentsContainer.reset(new OSCCueMessageArguments());
    argumentsContainer->userCanAddItemsManually = false;
    addChildControllableContainer(argumentsContainer.get());
}

OSCCueMessage::~OSCCueMessage()
{
    if (templateMessage != nullptr) {
        templateMessage->removeAsyncContainerListener(this);
    }
}

InspectableEditor* OSCCueMessage::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new OSCCueMessageEditor(this, isRoot);
}

void OSCCueMessage::showMenuForTargetOscMessage(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc)
{
    PopupMenu menu;

    Array<OSCInterface*> oscInterfaces = InterfaceManager::getInstance()->getItemsWithType<OSCInterface>();

    for (int i = 1 ; i <= oscInterfaces.size(); ++i)
    {
        PopupMenu sub;
        OSCInterface* oscInterface = oscInterfaces[i-1];
        BaseManager<OSCCommand>* commandManager = oscInterface->templateManager.get();

        for (int j = 1 ; j <= commandManager->items.size(); ++j)
        {
            OSCCommand* command = commandManager->items[j-1];
            sub.addItem((i * 1000) + j, command->niceName);
        }
        sub.addSeparator();
        sub.addItem(i, "Custom OSC message");
        menu.addSubMenu(oscInterface->niceName, sub);
    }

    menu.showMenuAsync(PopupMenu::Options(), [oscInterfaces, returnFunc](int result)
        {
            if (result <= 0) return;

            if (result <= 1000)
            {
                int interfaceIdx = result - 1;
                OSCInterface* oscInterface = oscInterfaces[interfaceIdx];
                returnFunc(oscInterface);
                return;
            }

            int messageIdx = (result % 1000) - 1;
            int interfaceIdx = ((result - messageIdx) / 1000) - 1;

            OSCInterface* oscInterface = oscInterfaces[interfaceIdx];
            returnFunc(oscInterface->templateManager->items[messageIdx]);
        }
    );
}

void OSCCueMessage::onContainerTriggerTriggered(Trigger* t)
{
    if (t == testBtn) sendTestMessage();
}

void OSCCueMessage::sendTestMessage()
{
    OSCMessage msg = buildMessage();

    OSCInterface* oscInterface = nullptr;
    ControllableContainer* targetContainer = targetTemplate->getTargetContainer();
    oscInterface = dynamic_cast<OSCInterface*>(targetContainer);
    if (oscInterface == nullptr)
    {
        OSCCommand* oscCommand = dynamic_cast<OSCCommand*>(targetContainer);
        if (oscCommand != nullptr)
        {
            oscInterface = oscCommand->interface;
        }
    }

    if (oscInterface != nullptr)
    {
        oscInterface->sendOSC(msg);
    }
}

void OSCCueMessage::onContainerParameterChanged(Parameter * p)
{
    clearWarning();

    if (p == targetTemplate)
    {
        ControllableContainer* targetContainer = targetTemplate->getTargetContainer();
        OSCCommand* oscCommand = dynamic_cast<OSCCommand*>(targetContainer);
        OSCInterface* oscInterface = dynamic_cast<OSCInterface*>(targetContainer);
        argumentsContainer->clear();

        if (oscCommand == nullptr && oscInterface == nullptr) {
            setWarningMessage("No OSC Template selected");
        } else if (oscCommand != nullptr) {
            if (templateMessage != nullptr) {
                templateMessage->removeAsyncContainerListener(this);
            }
            templateMessage = oscCommand;
            templateMessage->addAsyncContainerListener(this);

            address->setValue(oscCommand->address->stringValue());
            address->setEnabled(false);
            address->isSavable = false;
            argumentsContainer->userCanAddItemsManually = false;

            if (!Engine::mainEngine->isLoadingFile) {
                for (int i = 0; i < oscCommand->argumentsContainer->items.size(); i++)
                {
                    GenericControllableItem* gci = static_cast<GenericControllableItem*>(oscCommand->argumentsContainer->items[i]);
                    GenericControllableItem* nGci = argumentsContainer->addItemFrom(gci->controllable, true);
                    nGci->controllable->setEnabled(oscCommand->argumentsContainer->editable->boolValue());
                    if (!nGci->controllable->enabled) {
                        nGci->isSavable = false;
                    }
                    nGci->userCanRemove = false;
                    nGci->canBeReorderedInEditor = false;
                    nGci->canBeCopiedAndPasted = false;
                    nGci->userCanDuplicate = false;
                }
            }
        } else if (oscInterface != nullptr) {
            if (templateMessage != nullptr) {
                templateMessage->removeAsyncContainerListener(this);
                templateMessage = nullptr;
            }

            address->setValue("/");
            address->setEnabled(true);
            address->isSavable = true;
            argumentsContainer->userCanAddItemsManually = true;
        }
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}

void OSCCueMessage::loadJSONDataItemInternal(juce::var data)
{
    BaseItem::loadJSONDataItemInternal(data);

    ControllableContainer* targetContainer = targetTemplate->getTargetContainer();
    OSCCommand* oscCommand = dynamic_cast<OSCCommand*>(targetContainer);
    OSCInterface* oscInterface = dynamic_cast<OSCInterface*>(targetContainer);

    if (oscCommand == nullptr && oscInterface == nullptr) {
    } else if (oscCommand != nullptr) {
        address->setEnabled(false);
        address->isSavable = false;
        argumentsContainer->userCanAddItemsManually = false;
        address->setValue(oscCommand->address->getValue());

        if (!oscCommand->argumentsContainer->editable->boolValue()) {
            for (int i = 0; i < oscCommand->argumentsContainer->items.size(); i++)
            {
                GenericControllableItem* gci = static_cast<GenericControllableItem*>(oscCommand->argumentsContainer->items[i]);
                GenericControllableItem* nGci = argumentsContainer->addItemFrom(gci->controllable, true);
                nGci->controllable->setEnabled(false);
                nGci->isSavable = false;
                nGci->userCanRemove = false;
                nGci->canBeReorderedInEditor = false;
                nGci->canBeCopiedAndPasted = false;
                nGci->userCanDuplicate = false;
            }
        } else {
            for (int i = 0; i < argumentsContainer->items.size(); i++)
            {
                GenericControllableItem* gci = static_cast<GenericControllableItem*>(argumentsContainer->items[i]);
                gci->controllable->setEnabled(true);
                gci->userCanRemove = false;
                gci->canBeReorderedInEditor = false;
                gci->canBeCopiedAndPasted = false;
                gci->userCanDuplicate = false;
            }
        }
    } else if (oscInterface != nullptr) {
        address->setEnabled(true);
        address->isSavable = true;
        argumentsContainer->userCanAddItemsManually = true;
    }
    queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

OSCMessage OSCCueMessage::buildMessage()
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

void OSCCueMessage::newMessage(const ContainerAsyncEvent& e)
{
    if (e.source == templateMessage && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate)
    {
        HashMap<String, var> existingItems;
        address->setValue(templateMessage->address->stringValue());

        // If editable, we save current values to set them back after rebuilding the arguments
        if (templateMessage->argumentsContainer->editable->boolValue()) {
            for (int i = 0; i < argumentsContainer->items.size(); i++)
            {
                GenericControllableItem* gci = static_cast<GenericControllableItem*>(argumentsContainer->items[i]);
                Parameter* param = static_cast<Parameter*>(gci->controllable);
                existingItems.set(gci->niceName, param->getValue());
            }
        }

        argumentsContainer->clear();

        for (int i = 0; i < templateMessage->argumentsContainer->items.size(); i++)
        {
            GenericControllableItem* gci = static_cast<GenericControllableItem*>(templateMessage->argumentsContainer->items[i]);
            GenericControllableItem* nGci = argumentsContainer->addItemFrom(gci->controllable, true);
            nGci->controllable->setEnabled(templateMessage->argumentsContainer->editable->boolValue());
            if (!nGci->controllable->enabled) {
                nGci->isSavable = false;
            }
            nGci->userCanRemove = false;
            nGci->canBeReorderedInEditor = false;
            nGci->canBeCopiedAndPasted = false;
            nGci->userCanDuplicate = false;
        }

        // Resetting saved values before

        if (templateMessage->argumentsContainer->editable->boolValue()) {

            for (int i = 0; i < argumentsContainer->items.size(); i++)
            {
                if (!existingItems.contains(argumentsContainer->items[i]->niceName)) continue;

                var savedValue = existingItems.getReference(argumentsContainer->items[i]->niceName);
                GenericControllableItem* gci = static_cast<GenericControllableItem*>(argumentsContainer->items[i]);
                auto param = static_cast<Parameter*>(gci->controllable);
                param->setValue(savedValue);
            }

        }
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}
