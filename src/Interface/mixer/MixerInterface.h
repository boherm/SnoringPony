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
#include "MixerSettings.h"
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

    std::unique_ptr<MixerSettings> mixerSettings;
    std::unique_ptr<BaseManager<MixerChannel>> channels;
    std::unique_ptr<BaseManager<MixerFX>> fxs;

    int getNumDCAs() const;

    void rebuildMixerSettings();
    void attemptConnect();
    void attemptDisconnect();

    void loadJSONDataInternal(var data) override;

    void pushChannel(MixerChannel* c);
    void pushAllChannels();

    MixerChannel* getChannelOfCharacter(Character* c) const;

    void applyDCAMembership(const Array<Array<int>>& membership,
                            const StringArray& dcaNames,
                            const std::map<int, String>& activeChannelNames,
                            const std::map<int, std::set<int>>& channelFXBuses,
                            const Array<bool>& dcaHasFX,
                            const std::map<int, float>& dcaForcedFaders);

    void applyLineCheckBaseline();

    // Reset the console to a clean state: line-check baseline + clear all DCA assignments.
    // Called after a file load so the Wing reflects the loaded project from the start.
    void applyLoadBaseline();

    void itemAdded(MixerChannel* c) override;
    void itemsAdded(Array<MixerChannel*> items) override;

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

    String getTypeString() const override { return "Mixer"; }
    static MixerInterface* create(var /*params*/) { return new MixerInterface(); }
};
