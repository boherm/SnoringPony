/*
  ==============================================================================

    Clock.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Clock.h"

//==============================================================================
ClockUI::ClockUI(const String& contentName):
    ShapeShifterContent(Clock::getInstance(), contentName)
{

}

ClockUI::~ClockUI()
{
}

juce_ImplementSingleton(Clock);

Clock::Clock()
{
    addAndMakeVisible(hms);
    hms.setJustificationType(Justification::centred);
    fillText();
    Clock::resized();
    startTimer(250);
}

Clock::~Clock()
{
    stopTimer();
}

void Clock::paint (juce::Graphics& g)
{
}

void Clock::resized()
{
    hms.setBounds(getLocalBounds());
    Font f = Font(10, Font::plain);
    float h = f.getHeight();
    float w = f.getStringWidth(hms.getText());

    float sizeH = 10 * getHeight() / h - 10;
    float sizeW = 10 * getWidth() / w - 10;

    hms.setFont(Font(jmin(sizeW,sizeH), Font::plain));
    hms.setAlpha(0.7f);
}

void Clock::timerCallback()
{
    fillText();
}

void Clock::fillText()
{
    String hour = Time::getCurrentTime().formatted("%H:%M:%S");
    if (hour != hms.getText()) {
        hms.setText(hour, sendNotification);
    }
}