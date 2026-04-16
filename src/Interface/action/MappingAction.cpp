/*
  ==============================================================================

    MappingAction.cpp
    Created: 16 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MappingAction.h"

MappingAction::MappingAction(const String& name) :
    BaseItem(name)
{
    saveAndLoadRecursiveData = true;
    canBeDisabled = false;
    userCanDuplicate = false;
}

MappingAction::~MappingAction()
{
}
