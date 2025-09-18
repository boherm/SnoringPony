/*
  ==============================================================================

    CueListManagerUI.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CueListManagerUI.h"
#include "../CueListManager.h"

CueListManagerUI::CueListManagerUI(const String & contentName) :
	BaseManagerShapeShifterUI(contentName, CueListManager::getInstance())
{
	addItemText = "Add a new Cue List";
	noItemText = "You can manage your Cue Lists here,\nadd a new one to get started.";
	
	setShowSearchBar(true);
	addExistingItems();
}

CueListManagerUI::~CueListManagerUI()
{
}