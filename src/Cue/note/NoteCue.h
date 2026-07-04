/*
  ==============================================================================

    NoteCue.h
    Created: 04 Jul 2026
    Author:  boherm

    A NoteCue holds a free text note inside a cuelist playback. It performs no
    action and is never picked as a cuelist's next cue automatically, so it can
    never be GO'd by accident: it is purely informational (a comment/section
    marker in the cue list).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class NoteCue :
    public Cue
{
public:
    NoteCue(var params = var());
    virtual ~NoteCue();

    String getTypeString() const override { return "Note Cue"; }
    String getCueType() const override { return "Note"; }
    static NoteCue* create(var params) { return new NoteCue(params); }

    // Notes are never auto-selected as a next cue (post-play advance, keyboard
    // next/prev, auto-follow, load). They can still be targeted manually.
    bool canBeNextCueAuto() const override { return false; }

    void playInternal() override;

    String autoDescriptionInternal() override;
};
