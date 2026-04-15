/*
  ==============================================================================

    MixerChannel.h
    Created: 14 Apr 2026
    Author:  boherm

    A MixerChannel maps to a physical channel on the mixing console. It holds
    a list of Characters (multiple characters may share a channel).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "Character.h"

class MixerInterface;

class MixerChannel :
    public BaseItem
{
public:
    MixerChannel(var params = var());
    virtual ~MixerChannel();

    MixerInterface* parentMixer = nullptr;

    IntParameter* channelNumber;
    std::unique_ptr<BaseManager<Character>> characters;

    // Effective string pushed to /ch/<n>/name: comma-joined character names.
    String getEffectiveName() const;

    // Called by children Characters when their name changes.
    void notifyNameChanged();

    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Channel"; }
    static MixerChannel* create(var params) { return new MixerChannel(params); }
};
