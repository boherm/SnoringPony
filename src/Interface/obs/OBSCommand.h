/*
  ==============================================================================

    OBSCommand.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class OBSInterface;

class OBSCommand :
    public BaseItem
{
public:
    OBSCommand(String name = "OBS Request 1");
    virtual ~OBSCommand();

    OBSInterface* interface = nullptr;
    Trigger* testBtn;
    BoolParameter* editable;

    EnumParameter* requestType;
    std::unique_ptr<ControllableContainer> params;

    void rebuildParams();
    var buildRequestData();
    void execute();

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onContainerTriggerTriggered(Trigger* t) override;
    void loadJSONDataItemInternal(var data) override;

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables) override;

    String getTypeString() const override { return "OBS Command"; }
    static OBSCommand* create(var params) { return new OBSCommand(params.getProperty("name", "OBS Request 1")); }
};
