/*
  ==============================================================================

    ColorPresets.cpp
    Created: 04 Oct 2025 01:02:00pm
    Author:  boherm

  ==============================================================================
*/

#include "ColorPresets.h"

ColorPresets::ColorPresets() :
    ControllableContainer("Color Presets")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;
    userCanAddControllables = true;
    userAddControllablesFilters.add(ColorParameter::getTypeStringStatic());
    customUserCreateControllableFunc = &ColorPresets::createItem;
}

void ColorPresets::createItem(ControllableContainer* cc)
{
    ColorParameter* cp = new ColorParameter("Color", "", NORMAL_COLOR);
    cp->userCanChangeName = true;
    cp->isRemovableByUser = true;
	cp->saveValueOnly = false;
    cc->addControllable(cp);
}

void ColorPresets::clear()
{
    controllables.clear();
}

void ColorPresets::onControllableAdded(Controllable* c)
{
    Parameter* p = dynamic_cast<Parameter*>(c);
    p->userCanChangeName = true;
}
