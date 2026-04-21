/*
  ==============================================================================

    OBSCue.cpp
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OBSCue.h"
#include "OBSCueRequest.h"
#include "../../Interface/InterfaceManager.h"
#include "../../Interface/obs/OBSInterface.h"
#include "../../Interface/obs/OBSCommand.h"

OBSCue::OBSCue(var params) :
    Cue(params)
{
    itemDataType = "OBS Cue";
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    requestsManager.reset(new BaseManager<OBSCueRequest>("OBS Requests"));
    requestsManager->addBaseManagerListener(this);
    addChildControllableContainer(requestsManager.get());

    if (requestsManager->items.isEmpty()) {
        requestsManager->addItemFromData(var());
    }
}

OBSCue::~OBSCue()
{
    requestsManager->removeBaseManagerListener(this);
}

void OBSCue::playInternal()
{
    for (OBSCueRequest* req : requestsManager->items) {
        if (!req->enabled->boolValue()) continue;
        ControllableContainer* targetContainer = req->targetTemplate->getTargetContainer();
        OBSCommand* obsCommand = dynamic_cast<OBSCommand*>(targetContainer);
        OBSInterface* obsInterface = dynamic_cast<OBSInterface*>(targetContainer);

        if (obsCommand != nullptr) {
            if (obsCommand->editable->boolValue())
            {
                String type = obsCommand->requestType->getValueData().toString();
                String actualType = (type == "Custom")
                    ? obsCommand->params->getParameterByName("customRequestType")->stringValue()
                    : type;
                obsCommand->interface->sendRequest(actualType, req->buildRequestData());
            }
            else
            {
                obsCommand->execute();
            }
        } else if (obsInterface != nullptr) {
            String reqType = req->customRequestType->stringValue();
            if (reqType.isNotEmpty()) {
                obsInterface->sendRequest(reqType, req->buildRequestData());
            }
        }
    }
    endCue();
}

void OBSCue::itemAdded(OBSCueRequest* newItem)
{
    newItem->cue = this;
    newItem->warningResolveInspectable = this;
}

void OBSCue::itemsAdded(juce::Array<OBSCueRequest*> newItems)
{
    for (OBSCueRequest* req : newItems) {
        req->cue = this;
        req->warningResolveInspectable = this;
    }
}

String OBSCue::autoDescriptionInternal()
{
    String desc = "OBS Cue: ";

    for (int i = 0; i < requestsManager->items.size(); i++)
    {
        if (i > 0)
            desc += ", ";
        OBSCueRequest* req = requestsManager->items[i];

        ControllableContainer* targetContainer = req->targetTemplate->getTargetContainer();
        OBSCommand* obsCommand = dynamic_cast<OBSCommand*>(targetContainer);

        if (obsCommand != nullptr) {
            desc += obsCommand->niceName + " (" + obsCommand->requestType->getValueKey() + ")";

            if (!req->argumentsContainer->items.isEmpty()) {
                desc += " [";
                for (int y = 0; y < req->argumentsContainer->items.size(); y++) {
                    if (y > 0) desc += ", ";
                    GenericControllableItem* p = static_cast<GenericControllableItem*>(req->argumentsContainer->items[y]);
                    auto param = static_cast<Parameter*>(p->controllable);
                    desc += param->stringValue();
                }
                desc += "]";
            }
        } else {
            desc += req->customRequestType->stringValue();
        }
    }

    return desc;
}
