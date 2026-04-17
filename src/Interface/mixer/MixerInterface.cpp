/*
  ==============================================================================

    MixerInterface.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "MixerInterface.h"
#include "wing/WingMixerSettings.h"

MixerInterface::MixerInterface() :
    Interface("Mixer Interface 1")
{
    logIncomingData->hideInEditor = true;

    vendor = addEnumParameter("Mixer", "Mixing console brand/model");
    vendor->addOption("Behringer Wing", "Wing");

    rebuildMixerSettings();

    channels.reset(new BaseManager<MixerChannel>("Channels"));
    channels->selectItemWhenCreated = false;
    channels->addBaseManagerListener(this);
    addChildControllableContainer(channels.get());

    fxs.reset(new BaseManager<MixerFX>("FX"));
    fxs->selectItemWhenCreated = false;
    addChildControllableContainer(fxs.get());
}

MixerInterface::~MixerInterface()
{
    if (channels != nullptr) channels->removeBaseManagerListener(this);
    if (mixerSettings != nullptr) mixerSettings->disconnect();
}

int MixerInterface::getNumDCAs() const
{
    if (mixerSettings != nullptr) return mixerSettings->getNumDCAs();
    return 0;
}

void MixerInterface::rebuildMixerSettings()
{
    if (mixerSettings != nullptr)
    {
        mixerSettings->disconnect();
        removeChildControllableContainer(mixerSettings.get());
        mixerSettings.reset();
    }

    String v = vendor->getValueData().toString();

    if (v == "Wing")
        mixerSettings.reset(new WingMixerSettings());

    if (mixerSettings != nullptr)
    {
        mixerSettings->logOutgoing = [this](const String& msg)
        {
            if (logOutgoingData->boolValue())
                NLOG(niceName, "Send: " << msg);
        };
        addChildControllableContainer(mixerSettings.get(), false, 1);
    }
}

void MixerInterface::attemptConnect()
{
    if (mixerSettings == nullptr) return;
    mixerSettings->connect();

    if (mixerSettings->isConnected())
        NLOG(niceName, "Connected");
    else
        NLOGWARNING(niceName, "Failed to connect");
}

void MixerInterface::attemptDisconnect()
{
    if (mixerSettings == nullptr) return;
    mixerSettings->disconnect();
}

void MixerInterface::pushChannel(MixerChannel* c)
{
    if (mixerSettings == nullptr || !mixerSettings->isConnected()) return;
    if (c == nullptr) return;
    mixerSettings->sendChannelName(c->channelNumber->intValue(), c->getEffectiveName());
}

void MixerInterface::pushAllChannels()
{
    if (channels == nullptr) return;
    for (auto* c : channels->items) pushChannel(c);
}

MixerChannel* MixerInterface::getChannelOfCharacter(Character* c) const
{
    if (c == nullptr || channels == nullptr) return nullptr;
    for (auto* ch : channels->items)
        if (ch->characters->items.indexOf(c) >= 0) return ch;
    return nullptr;
}

void MixerInterface::applyDCAMembership(const Array<Array<int>>& membership,
                                        const StringArray& dcaNames,
                                        const std::map<int, String>& activeChannelNames,
                                        const std::map<int, std::set<int>>& channelFXBuses,
                                        const Array<bool>& dcaHasFX,
                                        const std::map<int, float>& dcaForcedFaders)
{
    if (mixerSettings == nullptr) return;

    Array<int> definedChannels;
    if (channels != nullptr)
    {
        for (auto* ch : channels->items)
            definedChannels.addIfNotAlreadyThere(ch->channelNumber->intValue());
    }

    Array<int> definedBuses;
    if (fxs != nullptr)
    {
        for (auto* fx : fxs->items)
            definedBuses.addIfNotAlreadyThere(fx->busNumber->intValue());
    }

    mixerSettings->applyDCAMembership(membership, dcaNames, definedChannels,
                                      activeChannelNames, channelFXBuses,
                                      definedBuses, dcaHasFX, dcaForcedFaders);
}

void MixerInterface::applyLineCheckBaseline()
{
    if (mixerSettings == nullptr || channels == nullptr) return;

    for (auto* ch : channels->items)
    {
        int chNum = ch->channelNumber->intValue();
        String firstName;
        if (!ch->characters->items.isEmpty())
            firstName = ch->characters->items[0]->characterName->stringValue();

        mixerSettings->sendChannelIcon(chNum, 0);
        mixerSettings->sendChannelName(chNum, firstName);
    }
}

void MixerInterface::itemAdded(MixerChannel* c)
{
    c->parentMixer = this;
    if (!Engine::mainEngine->isLoadingFile) pushChannel(c);
}

void MixerInterface::itemsAdded(Array<MixerChannel*> items)
{
    for (auto* c : items)
    {
        c->parentMixer = this;
        if (!Engine::mainEngine->isLoadingFile) pushChannel(c);
    }
}

void MixerInterface::onContainerParameterChangedInternal(Parameter* p)
{
    if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile) return;

    if (p == vendor)
    {
        rebuildMixerSettings();
    }
}

void MixerInterface::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile) return;

    if (mixerSettings != nullptr && cc == mixerSettings.get()
        && mixerSettings->shouldReconnectOnChange(c))
    {
        attemptConnect();
    }
}

void MixerInterface::loadJSONDataInternal(var data)
{
    Interface::loadJSONDataInternal(data);
    attemptConnect();
}
