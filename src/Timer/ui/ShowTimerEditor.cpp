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

    markBtnUI = timer->markTrigger->createButtonUI();
    markBtnUI->customBGColor = BG_COLOR.brighter(.1f);
    markBtnUI->useCustomBGColor = true;

    addAndMakeVisible(playBtnUI);
    addAndMakeVisible(pauseBtnUI);
    addAndMakeVisible(resetBtnUI);
    addAndMakeVisible(markBtnUI);

    timeLabel.setJustificationType(Justification::centred);
    timeLabel.setColour(Label::backgroundColourId, BG_COLOR.darker(.2f).withAlpha(.6f));
    timeLabel.setColour(Label::outlineColourId, BG_COLOR.brighter(.1f));
    timeLabel.setColour(Label::textColourId, Colours::white);
    timeLabel.setFont(Font(15.0f, Font::bold));
    timeLabel.setText(timer->getDisplayString(true), dontSendNotification);
    addAndMakeVisible(&timeLabel);

    marksTitle.setText("Marks", dontSendNotification);
    marksTitle.setFont(Font(13.0f, Font::bold));
    marksTitle.setColour(Label::textColourId, TEXT_COLOR.withAlpha(.7f));
    marksTitle.setJustificationType(Justification::centredLeft);
    addChildComponent(marksTitle); // shown only when the timer has marks

    marksUI.reset(new MarksManagerUI(timer->marksManager.get()));
    addAndMakeVisible(marksUI.get());

    startTimerHz(30);
}

ShowTimerEditor::~ShowTimerEditor()
{
    stopTimer();
    delete playBtnUI;
    delete pauseBtnUI;
    delete resetBtnUI;
    delete markBtnUI;
}

void ShowTimerEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
    resetBtnUI->setBounds(r.removeFromRight(46).reduced(2));
    markBtnUI->setBounds(r.removeFromRight(46).reduced(2));
    pauseBtnUI->setBounds(r.removeFromRight(46).reduced(2));
    playBtnUI->setBounds(r.removeFromRight(46).reduced(2));
    r.removeFromRight(4);
    timeLabel.setBounds(r.removeFromRight(110).reduced(1, 2));
}

void ShowTimerEditor::resizedInternalContent(Rectangle<int>& r)
{
    // Lay out the standard parameters first, then the "Marks" title + container below.
    GenericControllableContainerEditor::resizedInternalContent(r);

    bool hasMarks = timer->getNumMarks() > 0;
    marksTitle.setVisible(hasMarks);
    marksUI->setVisible(hasMarks);

    int titleH = hasMarks ? 18 : 0;
    if (hasMarks) marksTitle.setBounds(r.getX(), r.getY(), r.getWidth(), titleH);

    // Keep the container at full width / its own non-zero height so it stays live and
    // can lay out marks when they appear; only reserve body space when there are marks.
    int mh = jmax(marksUI->getHeight(), 1);
    marksUI->setBounds(r.getX(), r.getY() + titleH, r.getWidth(), mh);

    if (hasMarks) r.translate(0, titleH + mh + 4);
}

void ShowTimerEditor::timerCallback()
{
    // Relayout the editor when the marks container changes size (marks added/cleared).
    int mh = timer->getNumMarks() > 0 ? marksUI->getHeight() : 0;
    if (mh != lastMarksContentHeight)
    {
        lastMarksContentHeight = mh;
        resized();
    }

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
