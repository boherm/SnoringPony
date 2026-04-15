/*
  ==============================================================================

    GoCue.h
    Created: 15 Apr 2026
    Author:  boherm

    A GoCue triggers the GO action on a target cue (potentially in another
    cuelist), mirroring the behavior of pressing GO on that cuelist when the
    target cue is set as its next cue.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class GoCue :
    public Cue
{
public:
    GoCue(var params = var());
    virtual ~GoCue();

    TargetParameter* targetCue;

    void playInternal() override;

    String getTypeString() const override { return "Go Cue"; }
    String getCueType() const override { return "Go"; }
    static GoCue* create(var params) { return new GoCue(params); }

    String autoDescriptionInternal() override;
};
