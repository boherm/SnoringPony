/*
  ==============================================================================

    Cue.h
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "juce_organicui/controllable/parameter/BoolParameter.h"

class Cuelist;

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

    Cuelist* parentCuelist;

    BoolParameter* isPlaying;

    FloatParameter* id;
    FloatParameter* currentTime;
    FloatParameter* duration;
    StringParameter* description;
    StringParameter* notes;

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

    EnablingControllableContainer* autoFollowCC;
    FloatParameter* autoFollowDuration;
    FloatParameter* autoFollowCurrentTime;
    EnumParameter* autoFollowType;
    BoolParameter* autoFollowActive;

    CueTimer* preWaitTimer = nullptr;
    CueTimer* autoFollowTimer = nullptr;

    Trigger* setNextBtn;

    String getTypeString() const override { return "Cue"; }
    virtual String getCueType() const { return "Cue"; }
    static Cue* create(var params) { return new Cue(params); }

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);

    void parameterValueChanged(Parameter* p) override;
    void parameterControlModeChanged(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
    void triggerTriggered(Trigger* t) override;

    void play();
    virtual void playInternal() { }
    virtual bool canBePlayed() { return !isPlaying->boolValue(); }
    void stop();
    virtual void stopInternal() {}
    void panic();
    virtual void panicInternal() {}
    virtual void fade(double targetGain, double duration) {}
    void setGoNext();
    void onCueTimerFinished(Cue::CueTimer* timer);
};
