/*
  ==============================================================================

    Cue.h
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class Cuelist;
class MultiCueSync;
class GroupCue;

class Cue:
    public BaseItem,
    public ChangeBroadcaster
{
public:
    Cue(var params = var());
    virtual ~Cue();

    String objectType;
    var objectData;
    bool isFadable = false;
    bool isPanicking = false;
    bool isPreviewing = false;
    bool isRetriggerStopping = false;
    // Transient (not saved): true when this cue was last ended by an explicit stop/panic
    // rather than finishing on its own. A GroupCue reads it so a sub-cue stopped from the
    // Active Cues panel does not trigger the next sub-cue in sequential mode. Reset on play.
    bool wasStoppedManually = false;
    // Transient (not saved): when true, endCue() skips its post-wait auto-follow. Set by a
    // GroupCue that ends because of a stop/panic, so an aborted group doesn't fire its
    // post-wait (mirroring how a stopped/panicked normal cue never reaches its auto-follow).
    bool suppressAutoFollow = false;
    double retriggerFadeStartTime = 0.0;

    // Technical flag (not a parameter): when true, play() does not auto-advance the
    // parent cuelist's nextCue. Default false for every cue; set true by cues that
    // manage the next cue themselves (e.g. SetMainCuelistCue).
    bool notSetNextCueAuto = false;

    // Technical flag (not a parameter): when true, endCue() keeps this cue as its
    // cuelist's currentCue instead of clearing it. Used by instant cues that represent
    // a persistent state (e.g. DCACue), so the "current cue" reflects the last one fired.
    bool keepCurrentCueOnEnd = false;

    Cuelist* parentCuelist = nullptr;

    // Group this cue belongs to (null when ungrouped). Resolved from parentGroupUID after
    // load. A grouped cue is fired by its group, never selected as a cuelist's next cue
    // automatically (see canBeNextCueAuto), and is kept contiguous right after its group.
    GroupCue* parentGroup = nullptr;
    // Grouped sub-cues can't have a post-wait (they don't advance the cuelist and it would
    // skew the group's duration). Call after parentGroup changes: hides + disables the
    // post-wait section while grouped, and unhides it once ungrouped.
    void applyGroupMembershipConstraints();
    // Stable UID of the owning group, persisted so membership survives save/load. Empty
    // when ungrouped. Hidden from the editor; managed by CueManager add/removeFromGroup.
    StringParameter* parentGroupUID;
    bool isGroupMember() const { return parentGroup != nullptr; }

    BoolParameter* isPlaying;

    FloatParameter* id;
    FloatParameter* currentTime;
    FloatParameter* duration;
    StringParameter* description;
    StringParameter* notes;
    Trigger* previewBtn;

    class CueTimer :
        public Timer
    {
    public:
        CueTimer(Cue* cue);
        virtual ~CueTimer();

        Cue* parentCue;
        FloatParameter* currentTimeParam;
        float duration;

        void start(float duration, FloatParameter* currentTimeParam);
        void stop();
        void notify();

        void timerCallback() override;
    };

    EnablingControllableContainer* preWaitCC;
    FloatParameter* preWaitDuration;
    FloatParameter* preWaitCurrentTime;
    BoolParameter* preWaitActive;

    enum PostWaitType
    {
        IMMEDIATE = 0,
        AFTER_PRE = 1,
        AFTER_CUE = 2
    };

    EnablingControllableContainer* postWaitCC;
    FloatParameter* postWaitDuration;
    FloatParameter* postWaitCurrentTime;
    EnumParameter* postWaitType;
    BoolParameter* postWaitActive;

    EnablingControllableContainer* retriggerStopCC;
    FloatParameter* retriggerStopFadeOut;

    CueTimer* preWaitTimer = nullptr;
    CueTimer* postWaitTimer = nullptr;

    Trigger* setNextBtn;

    String getTypeString() const override { return "Cue"; }
    virtual String getCueType() const { return "Cue"; }
    static Cue* create(var params) { return new Cue(params); }

    // When false, automatic next-cue selection (post-play advance, auto-follow,
    // keyboard next/prev, initial load) skips this cue as if it weren't GO-able.
    // Grouped sub-cues opt out (their group fires them); Note cues override to false
    // unconditionally. Skipped cues can still be targeted manually.
    virtual bool canBeNextCueAuto() const { return parentGroup == nullptr; }

    // shortNames of controllables/containers this cue type wants hidden in the
    // multi-cue editor (e.g. per-file lists that make no sense to edit as a group).
    virtual juce::StringArray getMultiEditHiddenControllableNames() const { return {}; }

    // Optional extra component shown in the multi-cue editor for this cue type, driving
    // bulk edits across `scopeCues` (e.g. set output/volume of all files at once).
    // Ownership passes to the caller. Default: none.
    virtual juce::Component* createMultiEditExtraEditor(const juce::Array<Cue*>& scopeCues) { return nullptr; }

    // Optional structural sync for this cue type in multi-edit (mirrors state the generic
    // value mirror can't, e.g. DCA assignments). Ownership passes to the caller. Default: none.
    virtual MultiCueSync* createMultiEditSync(const juce::Array<Cue*>& scopeCues) { return nullptr; }

    // Top-level shortNames the generic value mirror must NOT propagate for this type even
    // though they are shown (e.g. handled by a custom sync, or genuinely per-cue).
    virtual juce::StringArray getMultiEditMirrorExcludedNames() const { return {}; }

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);

    void parameterValueChanged(Parameter* p) override;
    void parameterControlModeChanged(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
    void triggerTriggered(Trigger* t) override;

    void preview();
    virtual void previewInternal() { }
    virtual bool canBePreviewed() { return false; }

    void play();
    virtual void playInternal() { }
    virtual bool canBePlayed() { return !isPlaying->boolValue(); }

    // Group-seek helpers (used by GroupCue::seekToTime).
    // playSkippingPreWait: start the cue immediately, bypassing/cancelling any pre-wait.
    // startPreWaitAt: (re)enter the pre-wait with its current time set to `elapsed` seconds,
    // so the remaining wait matches the group's timeline.
    void playSkippingPreWait();
    void startPreWaitAt(double elapsed);

    void stop();
    virtual void stopInternal() {}
    virtual void retriggerStop();

    // Fade the cue out over `fadeTime` seconds and then stop it (used by GroupCue's
    // "Stop on retrigger" to fade every member over the group's fade time). The base cue
    // has nothing audible to fade, so it stops immediately; audio-backed cues override.
    virtual void fadeAndStop(double fadeTime);

    // Seeking from the Active Cues panel. canSeekTo() reports whether this cue supports it
    // right now; seekToTime(t) jumps playback to time t (seconds) within [0, duration].
    virtual bool canSeekTo() const { return false; }
    virtual void seekToTime(double t) {}

    void panic();
    virtual void panicInternal() {}

    virtual void fade(double targetGain, double duration) {}

    // Volume fader shown in the Active Cues panel (audio/playlist cues only).
    // getFaderVolume/setFaderVolume expose the user-set volume as an actual value in
    // [0, getMaxFaderVolume()] (the thumb); getOutputLevel is the live output level in the
    // same units including the fade/panic multiplier (the fill).
    virtual bool  hasVolumeFader() { return false; }
    virtual float getFaderVolume() { return 1.0f; }
    virtual float getMaxFaderVolume() { return 1.0f; }
    virtual void  setFaderVolume(float v) {}
    virtual float getOutputLevel() { return 1.0f; }

    void endCue();
    void playNextCue();
    bool isAutoStartCue();
    void setNextCue();
    void autoFollowProcess(PostWaitType type);

    String getDescription();
    virtual String autoDescriptionInternal() { return "Cue " + id->stringValue(); }

    void setGoNext();
    virtual void onCueTimerFinished(Cue::CueTimer* timer);
};
