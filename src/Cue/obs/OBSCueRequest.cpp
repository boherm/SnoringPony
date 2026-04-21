/*
  ==============================================================================

    OBSCueRequest.cpp
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OBSCueRequest.h"
#include "ui/OBSCueRequestEditor.h"
#include "../../Interface/obs/OBSInterface.h"
#include "../../Interface/obs/OBSCommand.h"
#include "../../Interface/InterfaceManager.h"

OBSCueRequestArguments::OBSCueRequestArguments() :
    GenericControllableManager("Arguments", false, false, false, false)
{
}

// -----------------------------------------------------

OBSCueRequest::OBSCueRequest(String name) :
    BaseItem(name)
{
    saveAndLoadRecursiveData = true;
    isSelectable = false;

    testBtn = addTrigger("Test", "Send test OBS request", true);
    testBtn->hideInEditor = true;

    targetTemplate = addTargetParameter("OBS Template", "OBS Template to use for this request", InterfaceManager::getInstance());
    targetTemplate->targetType = TargetParameter::CONTAINER;
    std::function<void(ControllableContainer*, std::function<void(ControllableContainer*)>)> getListFunc = std::bind(&OBSCueRequest::showMenuForTargetOBSCommand, this, std::placeholders::_1, std::placeholders::_2);
    targetTemplate->customGetTargetContainerFunc = getListFunc;
    targetTemplate->hideInEditor = true;

    requestTypeDisplay = addStringParameter("Request Type", "OBS request type", "", false);
    requestTypeDisplay->setEnabled(false);
    requestTypeDisplay->isSavable = false;

    customRequestType = addStringParameter("Custom Request Type", "OBS request type for custom requests", "", false);
    customRequestType->hideInEditor = true;

    customJSON = addStringParameter("Custom JSON", "Request data as raw JSON (optional)", "", false);
    customJSON->hideInEditor = true;

    setWarningMessage("No OBS Template selected");

    argumentsContainer.reset(new OBSCueRequestArguments());
    argumentsContainer->userCanAddItemsManually = false;
    addChildControllableContainer(argumentsContainer.get());
}

OBSCueRequest::~OBSCueRequest()
{
    if (templateCommand != nullptr) {
        templateCommand->removeAsyncContainerListener(this);
    }
}

InspectableEditor* OBSCueRequest::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new OBSCueRequestEditor(this, isRoot);
}

void OBSCueRequest::showMenuForTargetOBSCommand(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc)
{
    PopupMenu menu;

    Array<OBSInterface*> obsInterfaces = InterfaceManager::getInstance()->getItemsWithType<OBSInterface>();

    for (int i = 1; i <= obsInterfaces.size(); ++i)
    {
        PopupMenu sub;
        OBSInterface* obsInterface = obsInterfaces[i - 1];
        BaseManager<OBSCommand>* commandManager = obsInterface->templateManager.get();

        for (int j = 1; j <= commandManager->items.size(); ++j)
        {
            OBSCommand* command = commandManager->items[j - 1];
            sub.addItem((i * 1000) + j, command->niceName);
        }
        sub.addSeparator();
        sub.addItem(i, "Custom OBS request");
        menu.addSubMenu(obsInterface->niceName, sub);
    }

    menu.showMenuAsync(PopupMenu::Options(), [obsInterfaces, returnFunc](int result)
        {
            if (result <= 0) return;

            if (result <= 1000)
            {
                int interfaceIdx = result - 1;
                OBSInterface* obsInterface = obsInterfaces[interfaceIdx];
                returnFunc(obsInterface);
                return;
            }

            int commandIdx = (result % 1000) - 1;
            int interfaceIdx = ((result - commandIdx) / 1000) - 1;

            OBSInterface* obsInterface = obsInterfaces[interfaceIdx];
            returnFunc(obsInterface->templateManager->items[commandIdx]);
        }
    );
}

void OBSCueRequest::onContainerTriggerTriggered(Trigger* t)
{
    if (t == testBtn) sendTestRequest();
}

void OBSCueRequest::rebuildArgumentsFromTemplate()
{
    argumentsContainer->clear();

    if (templateCommand == nullptr) return;

    bool isEditable = templateCommand->editable->boolValue();

    if (templateCommand->params != nullptr)
    {
        for (auto* c : templateCommand->params->controllables)
        {
            Parameter* p = dynamic_cast<Parameter*>(c);
            if (p == nullptr) continue;

            GenericControllableItem* nGci = argumentsContainer->addItemFrom(p, true);
            nGci->controllable->setEnabled(isEditable);
            if (!isEditable) {
                nGci->isSavable = false;
            }
            nGci->userCanRemove = false;
            nGci->canBeReorderedInEditor = false;
            nGci->canBeCopiedAndPasted = false;
            nGci->userCanDuplicate = false;
        }
    }
}

var OBSCueRequest::buildRequestData()
{
    if (templateCommand != nullptr)
    {
        if (!templateCommand->editable->boolValue())
        {
            return templateCommand->buildRequestData();
        }

        // Editable mode: build request data from the cue's own arguments
        auto* obj = new DynamicObject();
        for (int i = 0; i < argumentsContainer->items.size(); i++)
        {
            GenericControllableItem* gci = static_cast<GenericControllableItem*>(argumentsContainer->items[i]);
            Parameter* param = static_cast<Parameter*>(gci->controllable);

            if (IntParameter::getTypeStringStatic() == param->getTypeString())
                obj->setProperty(param->niceName, param->intValue());
            else if (FloatParameter::getTypeStringStatic() == param->getTypeString())
                obj->setProperty(param->niceName, (double)param->floatValue());
            else if (BoolParameter::getTypeStringStatic() == param->getTypeString())
                obj->setProperty(param->niceName, param->boolValue());
            else
                obj->setProperty(param->niceName, param->stringValue());
        }
        return var(obj);
    }

    // Custom mode
    String json = customJSON->stringValue();
    if (json.isNotEmpty())
    {
        var parsed = JSON::parse(json);
        if (parsed.isObject()) return parsed;
    }
    return var(new DynamicObject());
}

void OBSCueRequest::sendTestRequest()
{
    ControllableContainer* targetContainer = targetTemplate->getTargetContainer();
    OBSCommand* obsCommand = dynamic_cast<OBSCommand*>(targetContainer);
    OBSInterface* obsInterface = dynamic_cast<OBSInterface*>(targetContainer);

    if (obsCommand != nullptr)
    {
        if (obsCommand->editable->boolValue())
        {
            String type = obsCommand->requestType->getValueData().toString();
            String actualType = (type == "Custom")
                ? obsCommand->params->getParameterByName("customRequestType")->stringValue()
                : type;
            obsCommand->interface->sendRequest(actualType, buildRequestData());
        }
        else
        {
            obsCommand->execute();
        }
    }
    else if (obsInterface != nullptr)
    {
        String reqType = customRequestType->stringValue();
        if (reqType.isNotEmpty())
        {
            obsInterface->sendRequest(reqType, buildRequestData());
        }
    }
}

void OBSCueRequest::onContainerParameterChanged(Parameter* p)
{
    clearWarning();

    if (p == targetTemplate)
    {
        ControllableContainer* targetContainer = targetTemplate->getTargetContainer();
        OBSCommand* obsCommand = dynamic_cast<OBSCommand*>(targetContainer);
        OBSInterface* obsInterface = dynamic_cast<OBSInterface*>(targetContainer);
        argumentsContainer->clear();

        if (obsCommand == nullptr && obsInterface == nullptr) {
            setWarningMessage("No OBS Template selected");
            requestTypeDisplay->setValue("");
            requestTypeDisplay->hideInEditor = true;
            customRequestType->hideInEditor = true;
            customJSON->hideInEditor = true;
        } else if (obsCommand != nullptr) {
            if (templateCommand != nullptr) {
                templateCommand->removeAsyncContainerListener(this);
            }
            templateCommand = obsCommand;
            templateCommand->addAsyncContainerListener(this);

            requestTypeDisplay->setValue(obsCommand->requestType->getValueKey());
            requestTypeDisplay->setEnabled(false);
            requestTypeDisplay->isSavable = false;
            requestTypeDisplay->hideInEditor = false;

            customRequestType->hideInEditor = true;
            customJSON->hideInEditor = true;
            argumentsContainer->userCanAddItemsManually = false;

            if (!Engine::mainEngine->isLoadingFile) {
                rebuildArgumentsFromTemplate();
            }
        } else if (obsInterface != nullptr) {
            if (templateCommand != nullptr) {
                templateCommand->removeAsyncContainerListener(this);
                templateCommand = nullptr;
            }

            requestTypeDisplay->hideInEditor = true;

            customRequestType->hideInEditor = false;
            customRequestType->setEnabled(true);
            customJSON->hideInEditor = false;
            customJSON->setEnabled(true);
            argumentsContainer->userCanAddItemsManually = false;
        }
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }
}

void OBSCueRequest::loadJSONDataItemInternal(juce::var data)
{
    BaseItem::loadJSONDataItemInternal(data);

    ControllableContainer* targetContainer = targetTemplate->getTargetContainer();
    OBSCommand* obsCommand = dynamic_cast<OBSCommand*>(targetContainer);
    OBSInterface* obsInterface = dynamic_cast<OBSInterface*>(targetContainer);

    if (obsCommand != nullptr) {
        if (templateCommand != nullptr) {
            templateCommand->removeAsyncContainerListener(this);
        }
        templateCommand = obsCommand;
        templateCommand->addAsyncContainerListener(this);

        requestTypeDisplay->setValue(obsCommand->requestType->getValueKey());
        requestTypeDisplay->setEnabled(false);
        requestTypeDisplay->isSavable = false;
        requestTypeDisplay->hideInEditor = false;

        customRequestType->hideInEditor = true;
        customJSON->hideInEditor = true;
        argumentsContainer->userCanAddItemsManually = false;

        if (!obsCommand->editable->boolValue()) {
            rebuildArgumentsFromTemplate();
        } else {
            // Editable: keep the values loaded by BaseItem, just set UI flags
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
    } else if (obsInterface != nullptr) {
        requestTypeDisplay->hideInEditor = true;

        customRequestType->hideInEditor = false;
        customRequestType->setEnabled(true);
        customJSON->hideInEditor = false;
        customJSON->setEnabled(true);
    }
    queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

void OBSCueRequest::newMessage(const ContainerAsyncEvent& e)
{
    if (e.source == templateCommand && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate)
    {
        requestTypeDisplay->setValue(templateCommand->requestType->getValueKey());

        HashMap<String, var> existingItems;

        // If editable, save current values to restore after rebuild
        if (templateCommand->editable->boolValue()) {
            for (int i = 0; i < argumentsContainer->items.size(); i++)
            {
                GenericControllableItem* gci = static_cast<GenericControllableItem*>(argumentsContainer->items[i]);
                Parameter* param = static_cast<Parameter*>(gci->controllable);
                existingItems.set(gci->niceName, param->getValue());
            }
        }

        rebuildArgumentsFromTemplate();

        // Restore saved values
        if (templateCommand->editable->boolValue()) {
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
