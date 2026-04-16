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
#include "MixerFX.h"
#include "MixerConnectionService.h"
#include <map>
#include <set>

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
    std::unique_ptr<BaseManager<MixerFX>> fxs;
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
    // activeChannelNames maps channel number → active character name for this cue.
    // channelFXBuses maps channel number → bus numbers that should be enabled for it.
    // dcaHasFX[i]   = true if DCA i+1 contains at least one character with an FX.
    void applyDCAMembership(const Array<Array<int>>& membership,
                            const StringArray& dcaNames,
                            const std::map<int, String>& activeChannelNames,
                            const std::map<int, std::set<int>>& channelFXBuses,
                            const Array<bool>& dcaHasFX,
                            const std::map<int, float>& dcaForcedFaders);

    // Called by the "Line check" cue: resets channel icons and pushes the first
    // character name of each declared channel as its base label.
    void applyLineCheckBaseline();

    void itemAdded(MixerChannel* c) override;
    void itemsAdded(Array<MixerChannel*> items) override;

    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "Mixer"; }
    static MixerInterface* create(var /*params*/) { return new MixerInterface(); }
};
