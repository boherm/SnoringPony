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

class CharacterRef :
    public BaseItem
{
public:
    CharacterRef(var params = var());
    virtual ~CharacterRef();

    TargetParameter* characterRef;

    Character* getCharacter() const;
    MixerChannel* getChannel() const;

    String getTypeString() const override { return "CharacterRef"; }
    static CharacterRef* create(var params) { return new CharacterRef(params); }
};
