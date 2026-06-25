/*
  ==============================================================================

    ShowTimerManagerUI.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "ShowTimerManagerUI.h"

ShowTimerManagerUI::ShowTimerManagerUI(const String& contentName) :
    BaseManagerShapeShifterUI(contentName, ShowTimerManager::getInstance())
{
    addItemText = "Add a new Timer";
    noItemText = "Create timers to chronometer parts of your show.";

    setShowSearchBar(false);
    addExistingItems();

    resetAllBtn.setColour(TextButton::buttonColourId, BG_COLOR.brighter(.08f));
    resetAllBtn.setColour(TextButton::textColourOffId, TEXT_COLOR);
    resetAllBtn.setTooltip("Pause and reset all timers");
    resetAllBtn.addListener(this);
    addAndMakeVisible(resetAllBtn);

    startTimerHz(30);
}

ShowTimerManagerUI::~ShowTimerManagerUI()
{
    stopTimer();
}

void ShowTimerManagerUI::resizedInternalHeader(Rectangle<int>& hr)
{
    // Let the base place the "+" add button at the far right, then put Reset to its left.
    BaseManagerShapeShifterUI<ShowTimerManager, ShowTimer, ShowTimerManagerItemUI>::resizedInternalHeader(hr);
    resetAllBtn.setBounds(hr.removeFromRight(58).reduced(2));
}

void ShowTimerManagerUI::buttonClicked(Button* b)
{
    if (b == &resetAllBtn)
    {
        for (auto* t : ShowTimerManager::getInstance()->items)
        {
            t->pause();
            t->resetTimer();
        }
        return;
    }

    // Let the base handle its own buttons (e.g. the "+" add button).
    BaseManagerShapeShifterUI<ShowTimerManager, ShowTimer, ShowTimerManagerItemUI>::buttonClicked(b);
}

void ShowTimerManagerUI::resizedInternalFooter(Rectangle<int>& r)
{
    // Reserve a strip at the bottom for the general total; the remaining r goes to the list.
    footerBounds = r.removeFromBottom(22);
}

void ShowTimerManagerUI::paintOverChildren(Graphics& g)
{
    if (footerBounds.isEmpty()) return;

    g.setColour(BG_COLOR.darker(.2f));
    g.fillRect(footerBounds);
    g.setColour(BG_COLOR.brighter(.15f));
    g.drawLine((float)footerBounds.getX(), (float)footerBounds.getY(),
               (float)footerBounds.getRight(), (float)footerBounds.getY());

    Rectangle<int> fr = footerBounds.reduced(8, 2);
    g.setColour(Colours::white.withAlpha(.6f));
    g.setFont(12.0f);
    g.drawText("General total", fr, Justification::centredLeft);

    g.setColour(ShowTimerManager::getInstance()->isAnyGeneralRunning() ? Colours::limegreen : Colours::white);
    g.setFont(Font(14.0f, Font::bold));
    g.drawText(ShowTimer::secondsToString(ShowTimerManager::getInstance()->getGeneralTotalSeconds(), true),
               fr, Justification::centredRight);
}

void ShowTimerManagerUI::timerCallback()
{
    for (auto* ui : itemsUI)
        if (ui != nullptr) ui->repaint();

    if (!footerBounds.isEmpty())
        repaint(footerBounds);
}
