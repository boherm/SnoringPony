/*
  ==============================================================================

    AudioWaveformSlicer.cpp
    Created: 22 Dec 2025 04:26:23am
    Author:  boherm

  ==============================================================================
*/

#include "AudioWaveformSlicer.h"
#include "AudioSlices.h"
#include "ui/AudioWaveformSlicerEditor.h"

AudioWaveformSlicer::AudioWaveformSlicer(AudioCue* audioCue) :
    ControllableContainer("Waveform")
{
    this->audioCue = audioCue;
    editorCanBeCollapsed = false;
    editorIsCollapsed = false;

    zoomInBtn = addTrigger("+", "Zoom in the waveform");
    zoomOutBtn = addTrigger("-", "Zoom out the waveform");
}

AudioWaveformSlicer::~AudioWaveformSlicer()
{
}

InspectableEditor* AudioWaveformSlicer::getEditorInternal(bool isRoot, juce::Array<Inspectable*> inspectables)
{
	Array<ControllableContainer*> containers = Inspectable::getArrayAs<Inspectable, ControllableContainer>(inspectables);
	this->editor = new AudioWaveformSlicerEditor(containers, isRoot);
    return this->editor;
}

void AudioWaveformSlicer::triggerTriggered(Trigger* t)
{
    if (t == zoomInBtn) {
        if (editor != nullptr) {
            editor->zoomIn();
        }
    }
    if (t == zoomOutBtn) {
        if (editor != nullptr) {
            editor->zoomOut();
        }
    }
}
