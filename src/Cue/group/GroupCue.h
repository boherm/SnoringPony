/*
  ==============================================================================

    GroupCue.h
    Created: 04 Jul 2026
    Author:  boherm

    A GroupCue owns a set of sub-cues (the contiguous cues that follow it in the
    cuelist and reference it via parentGroup). GO'ing the group fires all of them,
    either all at once (Parallel) or one after another (Sequential). Grouped sub-cues
    are never auto-selected as a cuelist's next cue: the group stands in for them.

    The group stays "playing" as long as any member is active (playing / pre- or
    post-waiting) and ends as soon as they all stop — including on panic or stop.
    It learns of member state changes by listening to the cue manager's feedback
    (the same mechanism Cuelist uses for its own Is Playing), so it reacts to every
    transition, not just clean completions.

  ==============================================================================
*/

#pragma once

#include "../Cue.h"

class GroupCue :
    public Cue,
    public ContainerAsyncListener,
    public juce::Timer
{
public:
    GroupCue(var params = var());
    virtual ~GroupCue();

    enum FireMode
    {
        PARALLEL = 0,
        SEQUENTIAL = 1,
        TIMELINE = 2
    };

    EnumParameter* fireMode;
    // Folded state in the Deck (hides the members). Persisted, driven from the deck.
    BoolParameter* collapsed;
    // Stable identity used by members (Cue::parentGroupUID) to re-attach after load.
    StringParameter* groupUID;

    String getTypeString() const override { return "Group Cue"; }
    String getCueType() const override { return "Group"; }
    static GroupCue* create(var params) { return new GroupCue(params); }

    // Timeline mode gets a custom inspector editor (draggable timeline of the sub-cues).
    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables) override;

    // Every cue in the cuelist whose parentGroup == this, in cuelist order.
    Array<Cue*> getMembers() const;

    // Recompute the group's total duration from its members (mode + pre/post-waits) and
    // publish it to the duration parameter. Safe to call any time; also starts listening
    // to member changes so it stays up to date. Called by CueManager on membership changes.
    void refreshDuration();

    void playInternal() override;
    void stopInternal() override;
    void panicInternal() override;

    // Seek the whole group timeline: hard-stop members and (re)start playback at the
    // sub-cue (and offset) matching t, so the Active Cues panel can scrub the group.
    bool canSeekTo() const override;
    void seekToTime(double t) override;

    String autoDescriptionInternal() override;

    void parameterValueChanged(Parameter* p) override;
    void newMessage(const ContainerAsyncEvent& e) override;
    void timerCallback() override; // advances currentTime while playing

private:
    // Sequential-fire cursor (index into seqMembers). Unused in parallel mode.
    Array<Cue*> seqMembers;
    int seqIndex = -1;
    bool sequencing = false;
    // Set while the group itself is being stopped/panicked: tick() then ends the group as
    // soon as every member has settled (faded out), ignoring the wall-clock timeline.
    bool aborting = false;

    // Whether we're currently subscribed to the cue manager's feedback.
    bool listening = false;

    void ensureListening();
    void stopListening();

    // Total group duration from members: sum of spans (Sequential) or the longest span
    // (Parallel), each member's span accounting for its pre/post-waits.
    double computeDuration() const;

    // Re-evaluate member state: advance a sequential chain, or end the group once no
    // member is active anymore.
    void tick();
    void finishGroup();

    static bool isMemberActive(const Cue* m);
    // A member's occupied time on the group timeline (pre-wait + cue + post-wait, per type).
    static double memberSpan(const Cue* m);

    // Progress clock: currentTime = baseElapsed + wall-clock elapsed since playStartMs.
    juce::int64 playStartMs = 0;
    double baseElapsed = 0.0;
    void startProgressClock(double fromTime);

    // Seeking a member repositions its audio transport, which briefly flips the member's
    // isPlaying off/on. Until this time, tick() ignores that transient inactivity so it
    // doesn't wrongly advance or end the group. Set at the end of seekToTime().
    juce::int64 settleUntilMs = 0;
};
