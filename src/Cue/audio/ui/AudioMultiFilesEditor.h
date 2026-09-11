/*
  ==============================================================================

    AudioMultiFilesEditor.h
    Created: 04 Jul 2026
    Author:  boherm

    Shown in the multi-cue editor for a group of Audio cues, in place of the
    per-cue file lists (so it sits at their position, right before the plugin
    chain). Exposes a single Output and Files Volume control that are applied to
    every AudioFile of every selected Audio cue.

  ==============================================================================
*/

#pragma once

#include "../../ui/CueMultiBulkEditor.h"

class Cue;

class AudioMultiFilesEditor :
    public CueMultiBulkEditor
{
public:
    AudioMultiFilesEditor(const juce::Array<Cue*>& audioCues);
    ~AudioMultiFilesEditor() override;

    TargetParameter* proxyOutput = nullptr;
    FloatParameter* proxyVolume = nullptr;

protected:
    void applyProxyChange(Parameter* changedProxyParam) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioMultiFilesEditor)
};
