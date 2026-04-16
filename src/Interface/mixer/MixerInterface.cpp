/*
  ==============================================================================

    MixerInterface.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "MixerInterface.h"
#include "wing/WingProtocol.h"

MixerInterface::MixerInterface() :
    Interface("Mixer Interface 1")
{
    logIncomingData->hideInEditor = true;

    vendor = addEnumParameter("Vendor", "Mixing console brand/model");
    vendor->addOption("Behringer Wing", "Wing");

    remoteHost = addStringParameter("Remote Host", "IP address of the mixing console", "192.168.1.10");
    remoteHost->autoTrim = true;

    remotePort = addIntParameter("Remote Port", "OSC send port on the console",
                                 WingProtocol::DEFAULT_SEND_PORT, 1, 65535);

    numDCAs = addIntParameter("Num DCAs", "Number of DCAs to expose for mixing cues",
                              8, 1, WingProtocol::MAX_DCAS);

    isConnected = addBoolParameter("Connected", "Connection status", false);
    isConnected->setControllableFeedbackOnly(true);

    connection.reset(new MixerConnectionService());
    connection->setVendor(vendor->getValueData().toString());
    connection->logSent = [this](const OSCMessage& m)
    {
        if (!logOutgoingData->boolValue()) return;
        String s;
        for (auto& a : m) s += " " + OSCHelpers::getStringArg(a);
        NLOG(niceName, "Send OSC: " << m.getAddressPattern().toString() << " :" << s);
    };

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
    if (connection != nullptr) connection->disconnect();
}

void MixerInterface::attemptConnect()
{
    if (connection == nullptr) return;
    connection->setVendor(vendor->getValueData().toString());
    bool ok = connection->connect(remoteHost->stringValue(), remotePort->intValue(), 0);
    isConnected->setValue(ok);

    if (ok)
    {
        NLOG(niceName, "Connected to " << remoteHost->stringValue() << ":" << remotePort->intValue());
    }
    else
    {
        NLOGWARNING(niceName, "Failed to connect to " << remoteHost->stringValue() << ":" << remotePort->intValue());
    }
}

void MixerInterface::attemptDisconnect()
{
    if (connection == nullptr) return;
    connection->disconnect();
    isConnected->setValue(false);
}

void MixerInterface::pushChannel(MixerChannel* c)
{
    if (connection == nullptr || !connection->isConnected()) return;
    if (c == nullptr) return;
    connection->sendChannelName(c->channelNumber->intValue(), c->getEffectiveName());
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
    if (connection == nullptr) return;

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

    connection->applyDCAMembership(membership, dcaNames, definedChannels,
                                   activeChannelNames, channelFXBuses,
                                   definedBuses, dcaHasFX, dcaForcedFaders);
}

void MixerInterface::applyLineCheckBaseline()
{
    if (connection == nullptr || channels == nullptr) return;

    for (auto* ch : channels->items)
    {
        int chNum = ch->channelNumber->intValue();
        String firstName;
        if (!ch->characters->items.isEmpty())
            firstName = ch->characters->items[0]->characterName->stringValue();

        connection->sendChannelIcon(chNum, 0);
        connection->sendChannelName(chNum, firstName);
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

    if (p == vendor && connection != nullptr)
    {
        connection->setVendor(vendor->getValueData().toString());
    }
    else if ((p == remoteHost || p == remotePort) && connection != nullptr)
    {
        attemptConnect();
    }
}

void MixerInterface::loadJSONDataInternal(var data)
{
    Interface::loadJSONDataInternal(data);
    attemptConnect();
}
