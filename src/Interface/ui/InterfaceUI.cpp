/*
  ==============================================================================

    InterfaceUI.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "InterfaceUI.h"
#include "../../ui/SPAssetManager.h"
#include "../../Redundancy/RedundancyManager.h"

InterfaceUI::InterfaceUI(Interface* item) :
    BaseItemUI(item)
{
    Image iconImg;
    iconImg = SPAssetManager::getInstance()->getInterfaceIcon(item->getTypeString());

    iconUI.setImage(iconImg);
    addAndMakeVisible(iconUI);

    if (auto* rm = RedundancyManager::getInstanceWithoutCreating())
    {
        rmRoleParam = rm->role;
        rmEnabledParam = rm->enabled;
        rmRoleParam->addAsyncParameterListener(this);
        rmEnabledParam->addAsyncParameterListener(this);
    }
}

InterfaceUI::~InterfaceUI()
{
    if (RedundancyManager::getInstanceWithoutCreating() != nullptr)
    {
        if (rmRoleParam != nullptr) rmRoleParam->removeAsyncParameterListener(this);
        if (rmEnabledParam != nullptr) rmEnabledParam->removeAsyncParameterListener(this);
    }
}

void InterfaceUI::resizedHeader(Rectangle<int>& r)
{
    iconUI.setBounds(r.removeFromLeft(r.getHeight()).reduced(1));
	BaseItemUI::resizedHeader(r);
}

void InterfaceUI::paintOverChildren(Graphics& g)
{
    if (this->item == nullptr || !this->item->isRedundancyStandby()) return;

    // Amber overlay + badge: the interface is configured/ready but deliberately silenced because this
    // instance is a Backup in standby. Distinct from "disabled" (grey) and "active" (normal).
    const Rectangle<int> b = getLocalBounds();
    g.setColour(Colour(0xffffa000).withAlpha(0.10f));
    g.fillRoundedRectangle(b.toFloat(), 4.f);

    const int bw = 66, bh = 14;
    const Rectangle<int> badge(b.getRight() - bw - 6, b.getY() + 4, bw, bh);
    g.setColour(Colour(0xffffa000));
    g.fillRoundedRectangle(badge.toFloat(), 3.f);
    g.setColour(Colours::black);
    g.setFont(10.f);
    g.drawText("STANDBY", badge, Justification::centred, 1);
}

void InterfaceUI::newMessage(const Parameter::ParameterEvent& e)
{
    BaseItemUI<Interface>::newMessage(e);
    if (e.parameter == rmRoleParam || e.parameter == rmEnabledParam) repaint();
}
