/*
  ==============================================================================

    ActiveCuesPanel.h
    Created: 25 Jun 2026
    Author:  boherm

    Panel listing every cue currently active across all cuelists, with a per-cue
    STOP button that behaves like panic: first click fades out over a few seconds
    then stops, second click stops immediately. The list is scrollable.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cue;

// Scrollable content holding one row per active cue.
class ActiveCuesRows : public juce::Component
{
public:
    static constexpr int rowH = 64;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override;

    juce::Rectangle<int> getStopRect(int rowIndex) const;
    juce::Rectangle<int> getProgressRect(int rowIndex) const; // clickable track area (for seeking)
};

class ActiveCuesPanelUI : public ShapeShifterContent {
public:
    ActiveCuesPanelUI(const String &contentName);
    ~ActiveCuesPanelUI();

    static ActiveCuesPanelUI *create(const String &name) { return new ActiveCuesPanelUI(name); }
};

class ActiveCuesPanel :
    public juce::Component,
    juce::Timer
{
public:
    juce_DeclareSingleton(ActiveCuesPanel, true);
    ActiveCuesPanel();
    ~ActiveCuesPanel() override;

    static constexpr int headerH = 34;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void timerCallback() override;

    juce::Rectangle<int> getStopAllRect() const;

private:
    juce::Viewport viewport;
    ActiveCuesRows rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActiveCuesPanel)
};
