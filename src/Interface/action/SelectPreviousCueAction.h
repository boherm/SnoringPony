/*
  ==============================================================================

    SelectPreviousCueAction.h
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MappingAction.h"

class SelectPreviousCueAction :
    public MappingAction
{
public:
    SelectPreviousCueAction();
    ~SelectPreviousCueAction();

    BoolParameter* useMainCuelist;
    TargetParameter* targetCuelist;

    void execute() override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Select Previous Cue"; }
    static SelectPreviousCueAction* create(var params) { return new SelectPreviousCueAction(); }
};
