/*
  ==============================================================================

    GoAction.h
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MappingAction.h"

class GoAction :
    public MappingAction
{
public:
    GoAction();
    ~GoAction();

    BoolParameter* useMainCuelist;
    TargetParameter* targetCuelist;

    void execute() override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Go"; }
    static GoAction* create(var params) { return new GoAction(); }
};
