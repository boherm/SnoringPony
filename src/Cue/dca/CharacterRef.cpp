/*
  ==============================================================================

    CharacterRef.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "CharacterRef.h"
#include "DCACue.h"
#include "DCAAssignment.h"
#include "../../Interface/mixer/Character.h"
#include "../../Interface/mixer/MixerChannel.h"
#include "../../Interface/mixer/MixerFX.h"
#include "../../Interface/mixer/MixerInterface.h"

CharacterRef::CharacterRef(var params) :
    BaseItem(params.getProperty("name", "CharacterRef"))
{
    itemDataType = "CharacterRef";
    saveAndLoadRecursiveData = true;
    canBeDisabled = false;
    isSelectable = false;

    characterRef = addTargetParameter("Character", "The character on the mixer", nullptr);
    characterRef->targetType = TargetParameter::CONTAINER;

    fxRef = addTargetParameter("FX", "Optional effect routed for this character's channel", nullptr);
    fxRef->targetType = TargetParameter::CONTAINER;
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

MixerFX* CharacterRef::getFX() const
{
    return fxRef->getTargetContainerAs<MixerFX>();
}

void CharacterRef::updateRootFromCue()
{
    // Walk up: CharacterRef -> BaseManager<CharacterRef> -> DCAAssignment
    //          -> BaseManager<DCAAssignment> -> DCACue
    auto* charMgr = parentContainer.get();
    if (charMgr == nullptr) return;
    auto* assignment = dynamic_cast<DCAAssignment*>(charMgr->parentContainer.get());
    if (assignment == nullptr) return;
    DCACue* cue = assignment->getParentCue();
    if (cue == nullptr) return;
    MixerInterface* mixer = cue->getMixer();
    if (mixer == nullptr) return;

    characterRef->setRootContainer(mixer->channels.get(), false, false);
    fxRef->setRootContainer(mixer->fxs.get(), false, false);
}
