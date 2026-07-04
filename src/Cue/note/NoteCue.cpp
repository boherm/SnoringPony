/*
  ==============================================================================

    NoteCue.cpp
    Created: 04 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "NoteCue.h"

NoteCue::NoteCue(var params) :
    Cue(params)
{
    itemDataType = "Note Cue";

    // A note performs no action: hide every playback-related field.
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    currentTime->hideInEditor = true;

    preWaitCC->hideInEditor = true;
    postWaitCC->hideInEditor = true;
    retriggerStopCC->hideInEditor = true;
}

NoteCue::~NoteCue()
{
}

void NoteCue::playInternal()
{
    // A note does nothing; end immediately so it never stays "playing".
    endCue();
}

String NoteCue::autoDescriptionInternal()
{
    if (notes->stringValue().isNotEmpty())
        return notes->stringValue();
    return "Note";
}
