/*
  ==============================================================================

    OSCMapping.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OSCMapping.h"
#include "../action/MappingActionFactory.h"

OSCMapping::OSCMapping(var params) :
    BaseItem(params.getProperty("name", "OSC Mapping"))
{
    saveAndLoadRecursiveData = true;
    canBeDisabled = true;

    addressPattern = addStringParameter("Address", "OSC address pattern to match (e.g. /go)", "/");

    actions.reset(new BaseManager<MappingAction>("Actions"));
    actions->managerFactory = &MappingActionFactory::getInstance()->factory;
    actions->selectItemWhenCreated = false;
    addChildControllableContainer(actions.get());
}

OSCMapping::~OSCMapping()
{
}

bool OSCMapping::matches(const OSCMessage& msg) const
{
    if (!enabled->boolValue()) return false;
    return msg.getAddressPattern().matches(OSCAddress(addressPattern->stringValue()));
}

void OSCMapping::executeActions()
{
    for (auto* action : actions->items)
        action->execute();
}
