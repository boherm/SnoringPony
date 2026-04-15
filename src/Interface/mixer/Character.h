/*
  ==============================================================================

    Character.h
    Created: 14 Apr 2026
    Author:  boherm

    A Character belongs to a MixerChannel. Multiple characters can share a
    channel (e.g., an actor swapping roles across scenes).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class MixerChannel;

class Character :
    public BaseItem
{
public:
    Character(var params = var());
    virtual ~Character();

    StringParameter* characterName;

    MixerChannel* getChannel() const;

    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Character"; }
    static Character* create(var params) { return new Character(params); }
};
