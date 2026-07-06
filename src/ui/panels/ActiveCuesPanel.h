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

// A single active-cue row: holds a (non-owning) pointer to the cue it renders and paints itself.
// For now it just draws its box; the progress fill, text, STOP button and fader will be rebuilt
// here point by point.
class ActiveCuesRow : public juce::Component,
                      public juce::SettableTooltipClient
{
public:
    static constexpr int rowH = 71;      // base height
    static constexpr int waitBarH = 14;  // extra height per pre/post-wait bar at the bottom
    static constexpr int faderW = 10;    // width of the vertical volume fader on the right

    void setCue(Cue* c) { cue = c; repaint(); }
    Cue* getCue() const { return cue; }

    // Height this row wants: base + one bar per active wait + the fader (when the cue has one).
    int getDesiredHeight() const;

    void paint(juce::Graphics&) override;

    // Volume fader interactions (audio / playlist cues): drag to set, double-click to type.
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    juce::String getTooltip() override;

private:
    // The vertical volume fader strip on the right, or empty when the cue has no fader.
    juce::Rectangle<int> getFaderRect() const;
    // The cue playback progress area (content box, minus fader and wait bars) — clickable to seek.
    juce::Rectangle<int> getProgressRect() const;
    // The circular STOP button, top-right of the row (left of the fader).
    juce::Rectangle<int> getStopRect() const;

    Cue* cue = nullptr;
    bool draggingFader = false;
};

// Scrollable content holding one ActiveCuesRow child per active cue.
class ActiveCuesRows : public juce::Component
{
public:
    // Sync the child rows to `cues` (create / remove as needed), lay them out at `width`, and
    // resize to fit (at least `minHeight`). Called from the panel's timer.
    void setCues(const juce::Array<Cue*>& cues, int width, int minHeight);

    void paint(juce::Graphics&) override;

private:
    juce::OwnedArray<ActiveCuesRow> rowComps;
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
