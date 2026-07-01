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
class ActiveCuesRows : public juce::Component,
                       public juce::SettableTooltipClient
{
public:
    static constexpr int rowH = 66;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override; // type a volume value
    juce::String getTooltip() override; // volume value when hovering a fader

    juce::Rectangle<int> getStopRect(int rowIndex) const;
    juce::Rectangle<int> getProgressRect(int rowIndex) const; // clickable track area (for seeking)
    juce::Rectangle<int> getFaderRect(int rowIndex) const;     // draggable volume fader (audio/playlist)

private:
    int dragFaderRow = -1; // row whose volume fader is being dragged, -1 if none
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
