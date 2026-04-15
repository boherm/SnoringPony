/*
  ==============================================================================

    CharacterRef.h
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Character;
class MixerChannel;
class MixerFX;

class CharacterRef :
    public BaseItem
{
public:
    CharacterRef(var params = var());
    virtual ~CharacterRef();

    TargetParameter* characterRef;
    TargetParameter* fxRef;

    Character* getCharacter() const;
    MixerChannel* getChannel() const;
    MixerFX* getFX() const;

    // Restrict the two TargetParameters' menus to the mixer selected in the
    // parent DCACue (its channels and FX lists).
    void updateRootFromCue();

    String getTypeString() const override { return "CharacterRef"; }
    static CharacterRef* create(var params) { return new CharacterRef(params); }
};
