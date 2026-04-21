/*
  ==============================================================================

    MTCContainerEditor.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class AudioCue;

class MTCContainerEditor :
    public EnablingControllableContainerEditor
{
public:
    MTCContainerEditor(Array<ControllableContainer*> cc, bool isRoot);
    ~MTCContainerEditor();

    AudioCue* audioCue;
    Label timecodeLabel;

    void resizedInternalHeader(Rectangle<int>& r) override;
    void controllableFeedbackUpdate(Controllable* c) override;

    static InspectableEditor* create(bool isRoot, Array<ControllableContainer*> cc) { return new MTCContainerEditor(cc, isRoot); }
};
