/*
  ==============================================================================

    OSCMapping.h
    Created: 20 Apr 2026
    Author:  boherm

    Binds an OSC address pattern to a list of MappingActions.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../action/MappingAction.h"

class OSCMapping :
    public BaseItem
{
public:
    OSCMapping(var params = var());
    virtual ~OSCMapping();

    StringParameter* addressPattern;

    std::unique_ptr<BaseManager<MappingAction>> actions;

    bool matches(const OSCMessage& msg) const;
    void executeActions();

    String getTypeString() const override { return "OSC Mapping"; }
    static OSCMapping* create(var params) { return new OSCMapping(params); }
};
