/*
  ==============================================================================

    MixerInterface.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../Interface.h"
#include "../../MainIncludes.h"
#include "MixerChannel.h"
#include "MixerConnectionService.h"

class MixerInterface :
    public Interface,
    public BaseManager<MixerChannel>::ManagerListener
{
public:
    MixerInterface();
    ~MixerInterface();

    EnumParameter* vendor;
    StringParameter* remoteHost;
    IntParameter* remotePort;
    IntParameter* numDCAs;
    BoolParameter* isConnected;

    std::unique_ptr<BaseManager<MixerChannel>> channels;
    std::unique_ptr<MixerConnectionService> connection;

    void attemptConnect();
    void attemptDisconnect();

    void loadJSONDataInternal(var data) override;

    // Pushes the effective (joined) name of a channel to /ch/<n>/name.
    void pushChannel(MixerChannel* c);
    void pushAllChannels();

    // Finds the MixerChannel that contains this Character, or nullptr.
    MixerChannel* getChannelOfCharacter(Character* c) const;

    // Called by DCACue::playInternal.
    void applyDCAMembership(const Array<Array<int>>& membership,
                            const StringArray& dcaNames);

    void itemAdded(MixerChannel* c) override;
    void itemsAdded(Array<MixerChannel*> items) override;

    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Mixer"; }
    static MixerInterface* create(var /*params*/) { return new MixerInterface(); }
};
