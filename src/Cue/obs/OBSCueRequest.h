/*
  ==============================================================================

    OBSCueRequest.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class OBSCue;
class OBSCommand;

class OBSCueRequestArguments :
    public GenericControllableManager
{
public:
    OBSCueRequestArguments();
    virtual ~OBSCueRequestArguments() {}
};

// -----------------------------------------------------

class OBSCueRequest :
    public BaseItem,
    public ContainerAsyncListener
{
public:
    OBSCueRequest(String name = "OBS Request 1");
    virtual ~OBSCueRequest();

    OBSCue* cue = nullptr;
    Trigger* testBtn;
    TargetParameter* targetTemplate;

    StringParameter* requestTypeDisplay;
    std::unique_ptr<OBSCueRequestArguments> argumentsContainer;

    // Custom request fields (used when targeting an OBSInterface directly)
    StringParameter* customRequestType;
    StringParameter* customJSON;

    OBSCommand* templateCommand = nullptr;

    String getTypeString() const override { return "OBS Request"; }
    static OBSCueRequest* create(var params) { return new OBSCueRequest(params.getProperty("name", "OBS Request 1")); }

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables) override;
    void showMenuForTargetOBSCommand(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc);

    void onContainerTriggerTriggered(Trigger* t) override;
    void onContainerParameterChanged(Parameter* p) override;

    void loadJSONDataItemInternal(juce::var data) override;

    void rebuildArgumentsFromTemplate();
    var buildRequestData();
    void sendTestRequest();

    void newMessage(const ContainerAsyncEvent& e) override;
};
