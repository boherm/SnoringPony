/*
  ==============================================================================

    AudioMultiFilesEditor.h
    Created: 04 Jul 2026
    Author:  boherm

    Shown in the multi-cue editor for a group of Audio cues, in place of the
    per-cue file lists. Exposes a single Output and Volume control that, when
    changed, are applied to every AudioFile of every selected Audio cue.

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class Cue;

class AudioMultiFilesEditor :
    public juce::Component,
    public ContainerAsyncListener
{
public:
    AudioMultiFilesEditor(const juce::Array<Cue*>& audioCues);
    ~AudioMultiFilesEditor() override;

    // The audio cues whose files this editor drives (weak, as Inspectable).
    juce::Array<juce::WeakReference<Inspectable>> cues;

    // Detached proxy container backing the two bulk controls.
    std::unique_ptr<ControllableContainer> proxy;
    TargetParameter* proxyOutput = nullptr;
    FloatParameter* proxyVolume = nullptr;
    std::unique_ptr<GenericControllableContainerEditor> proxyEditor;

    bool isApplying = false;

    void resized() override;
    void childBoundsChanged(juce::Component* c) override;
    void newMessage(const ContainerAsyncEvent& e) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioMultiFilesEditor)
};
