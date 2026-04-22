/*
  ==============================================================================

    OBSCommand.cpp
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OBSCommand.h"
#include "OBSInterface.h"
#include "ui/OBSCommandEditor.h"

OBSCommand::OBSCommand(String name) :
    BaseItem(name)
{
    saveAndLoadRecursiveData = true;

    requestType = addEnumParameter("Request Type", "OBS WebSocket request type");
    requestType->addOption("Set Current Program Scene", "SetCurrentProgramScene");
    requestType->addOption("Set Current Preview Scene", "SetCurrentPreviewScene");
    requestType->addOption("Set Current Scene Transition", "SetCurrentSceneTransition");
    requestType->addOption("Set Current Scene Transition Duration", "SetCurrentSceneTransitionDuration");
    requestType->addOption("Trigger Studio Mode Transition", "TriggerStudioModeTransition");
    requestType->addOption("Set Scene Item Enabled", "SetSceneItemEnabled");
    requestType->addOption("Set Input Mute", "SetInputMute");
    requestType->addOption("Custom", "Custom");

    testBtn = addTrigger("Test", "Send this request now");
    testBtn->hideInEditor = true;

    editable = addBoolParameter("Editable", "If true, parameters can be edited directly in cues", false);

    rebuildParams();
}

OBSCommand::~OBSCommand()
{
}

void OBSCommand::rebuildParams()
{
    if (params != nullptr)
    {
        removeChildControllableContainer(params.get());
        params.reset();
    }

    params.reset(new ControllableContainer("Request Parameters"));
    addChildControllableContainer(params.get());

    String type = requestType->getValueData().toString();

    if (type == "SetCurrentProgramScene" || type == "SetCurrentPreviewScene")
    {
        params->addStringParameter("sceneName", "Name of the scene", "");
    }
    else if (type == "SetCurrentSceneTransition")
    {
        params->addStringParameter("transitionName", "Name of the transition", "");
        params->addIntParameter("transitionDuration", "Transition duration in milliseconds (-1 = use default)", -1, -1, 60000);
    }
    else if (type == "SetCurrentSceneTransitionDuration")
    {
        params->addIntParameter("transitionDuration", "Transition duration in milliseconds", 300, 0, 60000);
    }
    else if (type == "TriggerStudioModeTransition")
    {
        // No parameters needed
    }
    else if (type == "SetSceneItemEnabled")
    {
        params->addStringParameter("sceneName", "Name of the scene", "");
        params->addIntParameter("sceneItemId", "Scene item ID", 0, 0, 99999);
        params->addBoolParameter("sceneItemEnabled", "Enable or disable the scene item", true);
    }
    else if (type == "SetInputMute")
    {
        params->addStringParameter("inputName", "Name of the input", "");
        params->addBoolParameter("inputMuted", "Mute or unmute the input", false);
    }
    else if (type == "Custom")
    {
        params->addStringParameter("customRequestType", "OBS request type string", "");
        auto* customJSONParam = params->addStringParameter("customJSON", "Request data as raw JSON (optional)", "");
        customJSONParam->multiline = true;
    }

    params->queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, params.get()));
    queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

var OBSCommand::buildRequestData()
{
    String type = requestType->getValueData().toString();

    auto* obj = new DynamicObject();

    if (type == "SetCurrentProgramScene" || type == "SetCurrentPreviewScene")
    {
        obj->setProperty("sceneName", params->getParameterByName("sceneName")->stringValue());
    }
    else if (type == "SetCurrentSceneTransition")
    {
        obj->setProperty("transitionName", params->getParameterByName("transitionName")->stringValue());
    }
    else if (type == "SetCurrentSceneTransitionDuration")
    {
        obj->setProperty("transitionDuration", params->getParameterByName("transitionDuration")->intValue());
    }
    else if (type == "TriggerStudioModeTransition")
    {
        // Empty request data
    }
    else if (type == "SetSceneItemEnabled")
    {
        obj->setProperty("sceneName", params->getParameterByName("sceneName")->stringValue());
        obj->setProperty("sceneItemId", (int)params->getParameterByName("sceneItemId")->intValue());
        obj->setProperty("sceneItemEnabled", params->getParameterByName("sceneItemEnabled")->boolValue());
    }
    else if (type == "SetInputMute")
    {
        obj->setProperty("inputName", params->getParameterByName("inputName")->stringValue());
        obj->setProperty("inputMuted", params->getParameterByName("inputMuted")->boolValue());
    }
    else if (type == "Custom")
    {
        String customJSON = params->getParameterByName("customJSON")->stringValue();
        if (customJSON.isNotEmpty())
        {
            var parsed = JSON::parse(customJSON);
            if (parsed.isObject())
            {
                delete obj;
                return parsed;
            }
        }
    }

    return var(obj);
}

void OBSCommand::execute()
{
    if (interface == nullptr) return;

    String type = requestType->getValueData().toString();
    String actualType = (type == "Custom")
        ? params->getParameterByName("customRequestType")->stringValue()
        : type;

    // For SetCurrentSceneTransition, also send duration if specified
    if (type == "SetCurrentSceneTransition")
    {
        int duration = (int)params->getParameterByName("transitionDuration")->intValue();
        if (duration >= 0)
        {
            auto* durObj = new DynamicObject();
            durObj->setProperty("transitionDuration", duration);
            interface->sendRequest("SetCurrentSceneTransitionDuration", var(durObj));
        }
    }

    interface->sendRequest(actualType, buildRequestData());
}

void OBSCommand::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == requestType && !Engine::mainEngine->isLoadingFile)
    {
        rebuildParams();
    }
}

void OBSCommand::loadJSONDataItemInternal(var data)
{
    // Rebuild params for the now-loaded requestType
    rebuildParams();

    // Re-load saved param values from the JSON containers data
    var containersData = data.getProperty("containers", var());
    if (containersData.isObject())
    {
        var requestParamsData = containersData.getProperty("requestParameters", var());
        if (requestParamsData.isObject() && params != nullptr)
        {
            params->loadJSONData(requestParamsData);
        }
    }
}

void OBSCommand::onContainerTriggerTriggered(Trigger* t)
{
    if (t == testBtn)
    {
        execute();
    }
}

InspectableEditor* OBSCommand::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new OBSCommandEditor(this, isRoot);
}
