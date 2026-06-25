/*
  ==============================================================================

    ShowTimerManagerItemUI.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "ShowTimerManagerItemUI.h"

ShowTimerManagerItemUI::ShowTimerManagerItemUI(ShowTimer* item) :
    BaseItemUI(item)
{
    // Hide the color picker in the manager list (it stays available in the inspector).
    showColorUI = false;
    if (itemColorUI != nullptr) itemColorUI->setVisible(false);
}

ShowTimerManagerItemUI::~ShowTimerManagerItemUI()
{
}

void ShowTimerManagerItemUI::paint(Graphics& g)
{
    BaseItemUI::paint(g);

    // Live value, right-aligned (leaving room for the remove button only; the color
    // picker is hidden in the list).
    Rectangle<int> r = getLocalBounds().withTrimmedLeft(8).withTrimmedRight(22);

    String val = item->getDisplayString(true);
    Font valFont(14.0f);

    Colour col;
    if (item->isOverrun())
    {
        // Blink only while running; stay solid red when paused in overrun.
        if (item->isRunning())
        {
            bool blinkOn = (juce::Time::getMillisecondCounter() / 400) % 2 == 0;
            col = blinkOn ? Colours::red : Colours::limegreen;
        }
        else
            col = Colours::red;
    }
    else if (item->isRunning())
        col = Colours::limegreen;
    else
        col = Colours::white.withAlpha(.85f);

    // Draw the value inside a darker rounded box so it stands out.
    int valW = valFont.getStringWidth(val);
    int boxW = valW + 24;
    int boxH = jmin(r.getHeight() - 4, 22);
    Rectangle<int> box = r.removeFromRight(boxW).withSizeKeepingCentre(boxW, boxH);

    g.setColour(BG_COLOR.darker(.2f).withAlpha(.6f));
    g.fillRoundedRectangle(box.toFloat(), 4.0f);
    g.setColour(BG_COLOR.brighter(.1f));
    g.drawRoundedRectangle(box.toFloat().reduced(0.5f), 4.0f, 1.0f);

    g.setFont(valFont);
    g.setColour(col);
    g.drawText(val, box, Justification::centred);

    // "G" tag: to the right of the title, but left of the value box.
    if (item->isGeneralTimer())
    {
        Rectangle<int> tagArea = r.withTrimmedRight(6);
        g.setFont(Font(11.0f, Font::bold));
        g.setColour(Colours::white.withAlpha(.5f));
        g.drawText("G", tagArea, Justification::centredRight);
    }
}
