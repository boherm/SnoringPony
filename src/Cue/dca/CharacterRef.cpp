/*
  ==============================================================================

    CharacterRef.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "CharacterRef.h"
#include "../../Interface/mixer/Character.h"
#include "../../Interface/mixer/MixerChannel.h"

CharacterRef::CharacterRef(var params) :
    BaseItem(params.getProperty("name", "CharacterRef"))
{
    itemDataType = "CharacterRef";
    saveAndLoadRecursiveData = true;
    canBeDisabled = false;
    isSelectable = false;

    characterRef = addTargetParameter("Character", "The character on the mixer", nullptr);
    characterRef->targetType = TargetParameter::CONTAINER;
}

CharacterRef::~CharacterRef()
{
}

Character* CharacterRef::getCharacter() const
{
    return characterRef->getTargetContainerAs<Character>();
}

MixerChannel* CharacterRef::getChannel() const
{
    if (auto* c = getCharacter()) return c->getChannel();
    return nullptr;
}
