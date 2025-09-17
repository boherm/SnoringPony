/*
  ==============================================================================

	This file was auto-generated!

  ==============================================================================
*/
#include "MainIncludes.h"
#include "MainComponent.h"
#include "ui/panels/Clock.h"
#include "ui/panels/ShowControl.h"

using namespace std::placeholders;

String getAppVersion();

//==============================================================================
MainContentComponent::MainContentComponent()
{
}

MainContentComponent::~MainContentComponent()
{
}

void MainContentComponent::init()
{

	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Clock", &ClockUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Show Control", &ShowControlUI::create));

	// We set some Organic Tools in subfolder
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Outliner", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Parrots", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("The Detective", "Organic Tools");

	OrganicMainContentComponent::init();
}