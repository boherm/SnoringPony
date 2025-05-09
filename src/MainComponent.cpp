/*
  ==============================================================================

	This file was auto-generated!

  ==============================================================================
*/
#include "MainIncludes.h"
#include "MainComponent.h"
#include "PonyEngine.h"
#include "ui/AboutWindow.h"

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
	OrganicMainContentComponent::init();

	// We set some Organic Tools in subfolder
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Outliner", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Parrots", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("The Detective", "Organic Tools");
}
