/*
  ==============================================================================

    Character.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "Character.h"
#include "MixerChannel.h"

Character::Character(var params) :
    BaseItem(params.getProperty("name", "Character"))
{
    itemDataType = "Character";
    saveAndLoadRecursiveData = true;
    canBeDisabled = false;

    characterName = addStringParameter("Character Name", "Name of the character on this channel", params.getProperty("characterName", ""));
}

Character::~Character()
{
}

MixerChannel* Character::getChannel() const
{
    ControllableContainer* mgr = parentContainer.get();
    if (mgr == nullptr) return nullptr;
    return dynamic_cast<MixerChannel*>(mgr->parentContainer.get());
}

void Character::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == characterName)
    {
        String n = characterName->stringValue();
        if (n.isNotEmpty()) setNiceName(n);

        if (Engine::mainEngine->isLoadingFile) return;
        if (auto* ch = getChannel()) ch->notifyNameChanged();
    }
}
