/*
  ==============================================================================

    CuelistManagerUI.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistManagerUI.h"
#include "../CuelistManager.h"
#include "../../Cuelist/Cuelist.h"

CuelistManagerUI::CuelistManagerUI(const String & contentName) :
	BaseManagerShapeShifterUI(contentName, CuelistManager::getInstance())
{
	addItemText = "Add a new Cuelist";
	noItemText = "You can manage your Cuelists here,\nadd a new one to get started.";

	setShowSearchBar(true);
	addExistingItems();
}

CuelistManagerUI::~CuelistManagerUI()
{
}
