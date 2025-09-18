/*
  ==============================================================================

    CueList.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CueList.h"

CueList::CueList() :
    BaseItem("CueList")
{
  askConfirmationBeforeRemove = true;
  setCanBeDisabled(false);
  setHasCustomColor(true);
	itemColor->setDefaultValue(BG_COLOR.brighter(.2f));
}

CueList::~CueList()
{
}