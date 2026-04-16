/*
  ==============================================================================

    MappingAction.h
    Created: 16 Apr 2026
    Author:  boherm

    Abstract base class for actions triggered by interface mappings (MIDI, etc.).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class MappingAction :
    public BaseItem
{
public:
    MappingAction(const String& name = "Action");
    virtual ~MappingAction();

    virtual void execute() {}

    String getTypeString() const override { return "Action"; }
};
