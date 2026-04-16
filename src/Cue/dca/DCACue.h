/*
  ==============================================================================

    DCACue.h
    Created: 14 Apr 2026
    Author:  boherm

    DCACue applies, on GO, a full (replacement) DCA membership to its target mixer.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"
#include "DCAAssignment.h"

class MixerInterface;
class MixerChannel;
class Character;

class DCACue :
    public Cue,
    public BaseManager<DCAAssignment>::ManagerListener
{
public:
    DCACue(var params = var());
    virtual ~DCACue();

    TargetParameter* targetMixer;
    std::unique_ptr<BaseManager<DCAAssignment>> dcaAssignments;

    MixerInterface* getMixer() const;
    int getMixerNumDCAs() const;

    // Find the assignment for a given DCA number (1..N).
    DCAAssignment* findAssignment(int dcaNumber) const;
    // Create and register a new DCAAssignment for the given DCA number.
    DCAAssignment* createAssignment(int dcaNumber);

    // Is character `c` already assigned to any DCA in this cue other than `excluding`?
    bool isCharacterUsedInOtherDCA(Character* c, DCAAssignment* excluding) const;
    // Is any character of channel `ch` assigned to any DCA other than `excluding`?
    bool isChannelUsedInOtherDCA(MixerChannel* ch, DCAAssignment* excluding) const;
    // Find the first DCA assignment containing `c`, or nullptr.
    DCAAssignment* findAssignmentContaining(Character* c) const;

    void itemAdded(DCAAssignment* a) override;

    void refreshCharacterRefRoots();
    void checkBrokenRefs();
    void loadJSONDataInternal(var data) override;

    void playInternal() override;

    String getTypeString() const override { return "DCA Cue"; }
    String getCueType() const override { return "DCA"; }
    static DCACue* create(var params) { return new DCACue(params); }

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

    String autoDescriptionInternal() override;
};
