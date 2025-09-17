//
// Created by Boris Hermans on 12/05/2025.
//

#pragma once

#include "../../MainIncludes.h"

class ClockUI : public ShapeShifterContent {
    public:
        ClockUI(const String &contentName);
        ~ClockUI();

        static ClockUI *create(const String &name) { return new ClockUI(name); }
};

class Clock  : public Component,
    Timer
{
public:
    juce_DeclareSingleton(Clock, true);
    Clock();
    ~Clock() override;

    Label hms;

    void paint (Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void fillText();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Clock)
};