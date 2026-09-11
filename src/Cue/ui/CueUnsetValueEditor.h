/*
  ==============================================================================

    CueUnsetValueEditor.h
    Created: 11 Sep 2026
    Author:  boherm

    Multi-edit substitute for a top-level cue parameter whose shared display would be
    misleading (a Volume, typically: the section would show one arbitrary cue's level).

    It renders a single bare control, at the substituted parameter's own position, that
    starts at the parameter's minimum and stays "unset": nothing is written to the cues
    until the user actually moves it, after which the value is pushed to every cue in
    scope in one undo step.

    Driven by Cue::getMultiEditUnsetProxyNames().

  ==============================================================================
*/

#pragma once

#include "CueMultiBulkEditor.h"

class Cue;

class CueUnsetValueEditor :
    public CueMultiBulkEditor
{
public:
    // `anchorParam` is the representative cue's parameter; the same shortName is written on
    // every cue of `scopeCues` that exposes it.
    CueUnsetValueEditor(Parameter* anchorParam, const juce::Array<Cue*>& scopeCues);
    ~CueUnsetValueEditor() override;

    // Only parameters this editor knows how to mirror can be substituted.
    static bool canSubstitute(Controllable* c);

    juce::String targetName;
    Parameter* proxyParam = nullptr;

protected:
    void applyProxyChange(Parameter* changedProxyParam) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CueUnsetValueEditor)
};
