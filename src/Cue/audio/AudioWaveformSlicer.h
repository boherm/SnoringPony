/*
  ==============================================================================

    AudioWaveformSlicer.h
    Created: 22 Dec 2025 04:26:23am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class AudioCue;
class AudioWaveformSlicerEditor;

class AudioWaveformSlicer :
    public ControllableContainer
{
public:
    AudioWaveformSlicer(AudioCue* audioCue);
    virtual ~AudioWaveformSlicer();

    AudioWaveformSlicerEditor* editor;

    AudioCue* audioCue;

    Trigger* zoomInBtn;
    Trigger* zoomOutBtn;

    InspectableEditor* getEditorInternal(bool isRoot, juce::Array<Inspectable*> inspectables) override;

    void triggerTriggered(Trigger* t) override;
};
