/*
  ==============================================================================

    LookAndFeelTable.cpp
    Created: 01 Oct 2025 07:40:48pm
    Author:  boherm

  ==============================================================================
*/

#include "LookAndFeelTable.h"
#include "juce_organicui/ui/LookAndFeelOO.h"

LookAndFeelTable::LookAndFeelTable() : LookAndFeelOO()
{
    setColour(TableListBox::backgroundColourId, BG_COLOR);
}

void LookAndFeelTable::drawTableHeaderColumn(Graphics & g, TableHeaderComponent &, const String & columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
 juce::Rectangle<int> area(width, height);

    Colour c = BG_COLOR.brighter(0.2f);
	g.setColour(c);
    g.fillRect(area.reduced(1).toFloat());

    area.reduce (4, 0);

    if ((columnFlags & (TableHeaderComponent::sortedForwards | TableHeaderComponent::sortedBackwards)) != 0)
    {
        Path sortArrow;
        sortArrow.addTriangle (0.0f, 0.0f,
                               0.5f, (columnFlags & TableHeaderComponent::sortedForwards) != 0 ? -0.8f : 0.8f,
                               1.0f, 0.0f);

        g.setColour (Colour (0x99000000));
        g.fillPath (sortArrow, sortArrow.getTransformToScaleToFit (area.removeFromRight (height / 2).reduced (2).toFloat(), true));
    }

    g.setColour (FRONT_COLOR.withAlpha(0.8f));
    g.setFont (Font (height * 0.5f, Font::bold));
    g.drawFittedText (columnName, area, Justification::centred, 1);
}
