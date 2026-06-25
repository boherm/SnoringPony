/*
  ==============================================================================

    MTCMonitor.h
    Created: 25 Jun 2026
    Author:  boherm

    Panel that displays every MTC timecode currently being sent, one row per MIDI
    interface (several audio cues can stream MTC on different interfaces at once).

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class MTCMonitorUI : public ShapeShifterContent {
public:
    MTCMonitorUI(const String &contentName);
    ~MTCMonitorUI();

    static MTCMonitorUI *create(const String &name) { return new MTCMonitorUI(name); }
};

class MTCMonitor :
    public Component,
    Timer
{
public:
    juce_DeclareSingleton(MTCMonitor, true);
    MTCMonitor();
    ~MTCMonitor() override;

    void paint(Graphics&) override;
    void timerCallback() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MTCMonitor)
};
