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
    // Note cues override this so they are never queued automatically; they can
    // still be targeted manually (context menu / double-click).
    virtual bool canBeNextCueAuto() const { return true; }

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

    void stop();
    virtual void stopInternal() {}
    virtual void retriggerStop();

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
