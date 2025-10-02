/*
  ==============================================================================

    LookAndFeelTable.cpp
    Created: 01 Oct 2025 07:40:48pm
    Author:  boherm

  ==============================================================================
*/

#include "LookAndFeelTable.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_organicui/ui/LookAndFeelOO.h"
#include "juce_organicui/ui/Style.h"

LookAndFeelTable::LookAndFeelTable() : LookAndFeelOO()
{
    setColour(TableListBox::backgroundColourId, BG_COLOR);
}

void LookAndFeelTable::drawTableHeaderColumn(Graphics & g, TableHeaderComponent &, const String & columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
 juce::Rectangle<int> area(width, height);

    g.fillAll(BG_COLOR);
    Colour c = BG_COLOR.brighter(0.1f);
	g.setColour(c);
    g.fillRect(area.getX() + 0.5f, area.getY() + 1.0f, area.getWidth() - 1.0f, area.getHeight() - 2.0f);

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

void LookAndFeelTable::drawTableHeaderBackground (Graphics& g, TableHeaderComponent& header)
{
	juce::Rectangle<int> area (header.getLocalBounds());
    area.removeFromTop (area.getHeight() / 2);

	g.fillAll(BG_COLOR.darker(0.05f));

    // g.setColour(BG_COLOR);
    // g.drawLine(0.0f, 0.0f, header.getWidth(), 0.0f, 1.0f);
}
