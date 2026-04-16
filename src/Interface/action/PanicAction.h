/*
  ==============================================================================

    PanicAction.h
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MappingAction.h"

class PanicAction :
    public MappingAction
{
public:
    PanicAction();
    ~PanicAction();

    BoolParameter* useMainCuelist;
    TargetParameter* targetCuelist;

    void execute() override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Panic"; }
    static PanicAction* create(var params) { return new PanicAction(); }
};
