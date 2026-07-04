/*
  ==============================================================================

    DCACueMultiSync.h
    Created: 04 Jul 2026
    Author:  boherm

    Multi-edit sync for DCA cues: mirrors DCA assignments between ALL selected
    DCA cues, upserting by DCA number. Editing any selected cue (from the
    Inspector or the cues table) propagates to the others:

    - Add / modify a DCA on one cue  -> upserted into every other cue (same
      characters / name / fader), keeping each cue's other DCAs intact.
    - Remove a DCA from one cue      -> removed from every other cue.

    Character references resolve against each cue's target mixer, so cues must
    target the same mixer for references to carry over.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../ui/MultiCueSync.h"

#include <map>

class Cue;
class DCACue;
class DCAAssignment;
class Character;
class MixerFX;

class DCACueMultiSync :
    public MultiCueSync,
    public ContainerAsyncListener
{
public:
    DCACueMultiSync(const juce::Array<Cue*>& dcaCues);
    ~DCACueMultiSync() override;

    juce::Array<juce::WeakReference<Inspectable>> cues;

    bool isSyncing = false;

    void newMessage(const ContainerAsyncEvent& e) override;

private:
    // Snapshot of one DCA assignment: characters (identified by shared Character*, with
    // their optional FX) + assignment-level properties. Compared to compute per-character
    // deltas so each cue keeps its own membership.
    struct DcaInfo
    {
        juce::String name;
        bool force = false;
        float fader = 0.0f;
        std::map<Character*, MixerFX*> chars;

        bool operator==(const DcaInfo& o) const
        {
            return name == o.name && force == o.force && fader == o.fader && chars == o.chars;
        }
        bool operator!=(const DcaInfo& o) const { return !(*this == o); }
    };

    typedef std::map<int, DcaInfo> DcaState; // dcaNumber -> assignment snapshot
    std::map<DCACue*, DcaState> lastState;

    // Structural events (assignment / character add-remove) don't bubble to the cue, so we
    // listen directly to each assignment's Characters manager too, refreshed as they change.
    juce::Array<juce::WeakReference<ControllableContainer>> listenedCharManagers;

    void detachAll();
    void refreshCharManagers();
    bool isListeningTo(ControllableContainer* cm) const;

    DcaState stateOf(DCACue* c) const;
    void snapshotAll();

    DCACue* asDca(const juce::WeakReference<Inspectable>& w) const;
    DCACue* cueAncestor(ControllableContainer* cc) const;
    bool isUnderDcaAssignments(DCACue* owner, ControllableContainer* node) const;

    void propagateFrom(DCACue* source);
    void applyDcaDelta(DCACue* peer, int dcaNumber, const DcaInfo& prev, const DcaInfo& cur, bool inCur);
    void setCharacterFx(DCAAssignment* pa, Character* ch, MixerFX* fx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DCACueMultiSync)
};
