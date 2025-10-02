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

class Cuelist :
    public BaseItem ,
    public ChangeBroadcaster
{
    public:
        Cuelist(var params = var());
        virtual ~Cuelist();

        String objectType;
        var objectData;

        CueManager cues;

        Trigger* goBtn;
        Trigger* stopBtn;

        juce::String getTypeString() const override { return "Cuelist"; }
        static Cuelist *create(var params) { return new Cuelist(params); }
};
