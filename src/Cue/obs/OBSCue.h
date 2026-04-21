/*
  ==============================================================================

    OBSCue.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"
#include "OBSCueRequest.h"

class OBSCue :
    public Cue,
    public BaseManager<OBSCueRequest>::ManagerListener
{
public:
    OBSCue(var params = var());
    virtual ~OBSCue();

    std::unique_ptr<BaseManager<OBSCueRequest>> requestsManager;

    String getTypeString() const override { return "OBS Cue"; }
    String getCueType() const override { return "OBS"; }
    static OBSCue* create(var params) { return new OBSCue(params); }

    void playInternal() override;

    void itemAdded(OBSCueRequest*) override;
    void itemsAdded(juce::Array<OBSCueRequest*>) override;

    String autoDescriptionInternal() override;
};
