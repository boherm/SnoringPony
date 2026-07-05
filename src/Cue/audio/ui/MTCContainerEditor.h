/*
  ==============================================================================

    MTCContainerEditor.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class MTCContainerEditor :
    public EnablingControllableContainerEditor
{
public:
    MTCContainerEditor(Array<ControllableContainer*> cc, bool isRoot);
    ~MTCContainerEditor();

    // The "Timecode" read-only display parameter of the MTC container being edited. Found by
    // name so this editor works for any cue type that owns an MTC container (Audio, Group).
    StringParameter* timecodeDisplay;
    Label timecodeLabel;

    void resizedInternalHeader(Rectangle<int>& r) override;
    void controllableFeedbackUpdate(Controllable* c) override;

    static InspectableEditor* create(bool isRoot, Array<ControllableContainer*> cc) { return new MTCContainerEditor(cc, isRoot); }
};
