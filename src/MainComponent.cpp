/*
  ==============================================================================

    MainComponent.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "MainIncludes.h"
#include "MainComponent.h"
#include "juce_organicui/ui/shapeshifter/ShapeShifterFactory.h"
#include "ui/panels/Clock.h"
#include "ui/panels/ShowControl.h"
#include "Cuelist/ui/CuelistManagerUI.h"
#include "Deck/ui/DeckUI.h"
#include "Interface/ui/InterfaceManagerUI.h"

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
    // ShapeShifters registration --
    ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Interfaces", &InterfaceManagerUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Clock", &ClockUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Show Control", &ShowControlUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Cuelists", &CuelistManagerUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Deck A", &DeckUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Deck B", &DeckUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Deck C", &DeckUI::create));
	ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Deck D", &DeckUI::create));

	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Deck A", "Decks");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Deck B", "Decks");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Deck C", "Decks");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Deck D", "Decks");

	// We set some Organic Tools in subfolder
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Outliner", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("Parrots", "Organic Tools");
	ShapeShifterManager::getInstance()->isInViewSubMenu.set("The Detective", "Organic Tools");

    // Init Organic Main Content
	OrganicMainContentComponent::init();
}
