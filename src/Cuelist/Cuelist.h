/*
  ==============================================================================

    Cuelist.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "../Cue/CueManager.h"
#include "ui/CuelistEditor.h"

class Cuelist :
    public BaseItem,
    public ChangeBroadcaster,
    public ParameterListener
{
    public:
        Cuelist(var params = var());
        virtual ~Cuelist();

        String objectType;
        var objectData;

        CueManager cues;

        TargetParameter* nextCue;

        Trigger* goBtn;
        Trigger* stopBtn;

        void go();
        void stop();

        juce::String getTypeString() const override { return "Cuelist"; }
        static Cuelist *create(var params) { return new Cuelist(params); }

        CuelistEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);
        void loadJSONDataInternal(var data) override;

        void parameterControlModeChanged(Parameter* p) override;
        void triggerTriggered(Trigger* t) override;
};
