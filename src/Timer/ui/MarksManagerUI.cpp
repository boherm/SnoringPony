/*
  ==============================================================================

    MarksManagerUI.cpp
    Created: 30 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "MarksManagerUI.h"

MarksManagerUI::MarksManagerUI(MarksManager* manager) :
    BaseManagerUI<MarksManager, Mark, MarkUI>("Marks", manager, false) // no viewport: self-size to content
{
    headerSize = 0;   // no header strip
    minHeight = 0;    // collapse fully when there are no marks
    gap = 1;
    transparentBG = true;
    animateItemOnAdd = false;

    // Embedded read-only list: no rubber-band selector (it crashes outside a real
    // manager panel) and the container itself isn't selectable.
    useDedicatedSelector = false;
    manager->isSelectable = false;

    setShowSearchBar(false);
    addExistingItems();
}

MarksManagerUI::~MarksManagerUI()
{
}
