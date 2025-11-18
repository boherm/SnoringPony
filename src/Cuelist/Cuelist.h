/*
  ==============================================================================

    Cuelist.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class CueManager;

class Cuelist :
    public BaseItem,
    public ChangeBroadcaster,
    public ContainerAsyncListener
{
public:
    Cuelist(var params = var());
    virtual ~Cuelist();

    String objectType;
    var objectData;

    CueManager* cues;

    BoolParameter* isPlaying; // Todo: implement this!

    TargetParameter* nextCue;

    Trigger* goBtn;
    Trigger* stopBtn;

    void go();
    void stop();
    void panic();

    juce::String getTypeString() const override { return "Cuelist"; }
    static Cuelist *create(var params) { return new Cuelist(params); }

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);
    void loadJSONDataInternal(var data) override;

    void parameterControlModeChanged(Parameter* p) override;
    void triggerTriggered(Trigger* t) override;

    void newMessage(const ContainerAsyncEvent& e) override;
};
