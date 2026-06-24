/*
  ==============================================================================

    SetMainCuelistCue.h
    Created: 24 Jun 2026
    Author:  boherm

    A SetMainCuelistCue changes the *current* main cuelist on GO. It does NOT
    touch the startup main cuelist (ShowProperties::startupMainCuelist).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class SetMainCuelistCue :
    public Cue
{
public:
    SetMainCuelistCue(var params = var());
    virtual ~SetMainCuelistCue();

    TargetParameter* targetCuelist;
    // Optional: a cue of the target cuelist to declare as its next cue when this cue
    // plays. Leave empty to only switch the main cuelist.
    TargetParameter* nextCue;

    void playInternal() override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Set Main Cuelist Cue"; }
    String getCueType() const override { return "SetMainCuelist"; }
    static SetMainCuelistCue* create(var params) { return new SetMainCuelistCue(params); }

    String autoDescriptionInternal() override;
};
