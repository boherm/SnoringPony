/*
  ==============================================================================

    LookAndFeelTable.h
    Created: 01 Oct 2025 07:40:48pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class LookAndFeelTable : public LookAndFeelOO
{
    public:
        LookAndFeelTable();

        void drawTableHeaderColumn(Graphics & g, TableHeaderComponent &, const String & columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags);

};
