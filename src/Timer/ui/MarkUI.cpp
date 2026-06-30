/*
  ==============================================================================

    MarkUI.cpp
    Created: 30 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "MarkUI.h"
#include "../ShowTimer.h"

MarkUI::MarkUI(Mark* mark) :
    BaseItemUI<Mark>(mark)
{
    // A mark only needs its label + values: drop the enable / color / remove chrome.
    showColorUI = false;
    if (itemColorUI != nullptr) itemColorUI->setVisible(false);

    showEnableBT = false;

    // Marks are managed by the timer (created by cue/button, cleared on reset): no
    // per-item remove button.
    showRemoveBT = false;
    if (removeBT != nullptr) removeBT->setVisible(false);
}

MarkUI::~MarkUI()
{
}

void MarkUI::paintOverChildren(Graphics& g)
{
    BaseItemUI<Mark>::paintOverChildren(g);

    // Header row. Two right-aligned columns: duration (since previous mark) then
    // cumulative (total elapsed at this mark), distinguished by colour.
    Rectangle<int> h = getMainBounds().reduced(margin).removeFromTop(headerHeight);

    const int boxW = 70;
    Rectangle<int> cumBox = h.removeFromRight(boxW);
    h.removeFromRight(8);
    Rectangle<int> durBox = h.removeFromRight(boxW);

    g.setFont(Font(12.0f, Font::bold));
    g.setColour(Colours::white.withAlpha(.85f));
    g.drawText(ShowTimer::secondsToString(item->duration->doubleValue()), durBox, Justification::centredRight);

    g.setColour(BG_COLOR.brighter(.35f));
    g.drawText(ShowTimer::secondsToString(item->cumulative->doubleValue()), cumBox, Justification::centredRight);
}
