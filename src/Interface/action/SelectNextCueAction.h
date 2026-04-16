/*
  ==============================================================================

    SelectNextCueAction.h
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MappingAction.h"

class SelectNextCueAction :
    public MappingAction
{
public:
    SelectNextCueAction();
    ~SelectNextCueAction();

    BoolParameter* useMainCuelist;
    TargetParameter* targetCuelist;

    void execute() override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Select Next Cue"; }
    static SelectNextCueAction* create(var params) { return new SelectNextCueAction(); }
};
