/*
  ==============================================================================

    InterfaceUI.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "InterfaceUI.h"
#include "../../ui/SPAssetManager.h"

InterfaceUI::InterfaceUI(Interface* item) :
    BaseItemUI(item)
{
    Image iconImg;
    iconImg = SPAssetManager::getInstance()->getInterfaceIcon(item->getTypeString());

    iconUI.setImage(iconImg);
    addAndMakeVisible(iconUI);
}

InterfaceUI::~InterfaceUI()
{
}

void InterfaceUI::resizedHeader(Rectangle<int>& r)
{
    iconUI.setBounds(r.removeFromLeft(r.getHeight()).reduced(1));
	BaseItemUI::resizedHeader(r);
}
