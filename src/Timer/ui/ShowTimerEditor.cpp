/*
  ==============================================================================

    ShowTimerEditor.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "ShowTimerEditor.h"
#include "../ShowTimer.h"

ShowTimerEditor::ShowTimerEditor(Array<ShowTimer*> timers, bool isRoot) :
    BaseItemEditor(Inspectable::getArrayAs<ShowTimer, BaseItem>(timers), isRoot),
    timer(timers.getFirst())
{
    playBtnUI = timer->playTrigger->createButtonUI();
    playBtnUI->customBGColor = Colours::darkgreen;
    playBtnUI->useCustomBGColor = true;

    pauseBtnUI = timer->pauseTrigger->createButtonUI();
    pauseBtnUI->customBGColor = Colours::darkorange.darker(0.3f);
    pauseBtnUI->useCustomBGColor = true;

    resetBtnUI = timer->resetTrigger->createButtonUI();
    resetBtnUI->customBGColor = Colours::darkred;
    resetBtnUI->useCustomBGColor = true;

    addAndMakeVisible(playBtnUI);
    addAndMakeVisible(pauseBtnUI);
    addAndMakeVisible(resetBtnUI);

    timeLabel.setJustificationType(Justification::centred);
    timeLabel.setColour(Label::backgroundColourId, BG_COLOR.darker(.2f).withAlpha(.6f));
    timeLabel.setColour(Label::outlineColourId, BG_COLOR.brighter(.1f));
    timeLabel.setColour(Label::textColourId, Colours::white);
    timeLabel.setFont(Font(15.0f, Font::bold));
    timeLabel.setText(timer->getDisplayString(true), dontSendNotification);
    addAndMakeVisible(&timeLabel);

    startTimerHz(30);
}

ShowTimerEditor::~ShowTimerEditor()
{
    stopTimer();
    delete playBtnUI;
    delete pauseBtnUI;
    delete resetBtnUI;
}

void ShowTimerEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    resetBtnUI->setBounds(r.removeFromRight(46).reduced(2));
    pauseBtnUI->setBounds(r.removeFromRight(46).reduced(2));
    playBtnUI->setBounds(r.removeFromRight(46).reduced(2));
    r.removeFromRight(4);
    timeLabel.setBounds(r.removeFromRight(110).reduced(1, 2));
}

void ShowTimerEditor::timerCallback()
{
    String t = timer->getDisplayString(true);
    if (t != timeLabel.getText())
        timeLabel.setText(t, dontSendNotification);

    Colour col;
    if (timer->isOverrun())
    {
        // Blink only while running; stay solid red when paused in overrun.
        if (timer->isRunning())
        {
            bool blinkOn = (juce::Time::getMillisecondCounter() / 400) % 2 == 0;
            col = blinkOn ? Colours::red : Colours::limegreen;
        }
        else
            col = Colours::red;
    }
    else if (timer->isRunning())
        col = Colours::limegreen;
    else
        col = Colours::white;

    timeLabel.setColour(Label::textColourId, col);
    timeLabel.repaint();
}
