/*
  ==============================================================================

    MainComponent.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/
#include "MainIncludes.h"
#include "MainComponent.h"
#include "ui/panels/Clock.h"
#include "ui/panels/ShowControl.h"
#include "CueList/ui/CueListManagerUI.h"

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
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Cue Lists", &CueListManagerUI::create));

	// We set some Organic Tools in subfolder
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Outliner", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Parrots", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("The Detective", "Organic Tools");

	OrganicMainContentComponent::init();
}