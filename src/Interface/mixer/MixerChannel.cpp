/*
  ==============================================================================

    MixerChannel.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MixerChannel.h"
#include "MixerInterface.h"

MixerChannel::MixerChannel(var params) :
    BaseItem(params.getProperty("name", "Channel"))
{
    itemDataType = "Channel";
    saveAndLoadRecursiveData = true;
    canBeDisabled = false;

    channelNumber = addIntParameter("Channel", "Channel number on the mixing console",
                                    params.getProperty("channel", 1), 1, 128);

    characters.reset(new BaseManager<Character>("Characters"));
    characters->selectItemWhenCreated = false;
    addChildControllableContainer(characters.get());
}

MixerChannel::~MixerChannel()
{
}

String MixerChannel::getEffectiveName() const
{
    StringArray names;
    for (auto* c : characters->items)
    {
        String n = c->characterName->stringValue();
        if (n.isNotEmpty()) names.add(n);
    }
    return names.joinIntoString(", ");
}

void MixerChannel::notifyNameChanged()
{
    if (parentMixer == nullptr) return;
    parentMixer->pushChannel(this);
}

void MixerChannel::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == channelNumber)
    {
        if (Engine::mainEngine->isLoadingFile) return;

        setNiceName("Ch " + channelNumber->stringValue());
        if (parentMixer != nullptr)
            parentMixer->pushChannel(this);
    }
}
