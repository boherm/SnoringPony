/*
  ==============================================================================

    Cuelist.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Cuelist.h"

Cuelist::Cuelist() :
    BaseItem("Cuelist")
{
  askConfirmationBeforeRemove = true;
  setCanBeDisabled(false);
  setHasCustomColor(true);
	itemColor->setDefaultValue(BG_COLOR.brighter(.2f));
}

Cuelist::~Cuelist()
{
}