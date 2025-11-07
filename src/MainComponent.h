/*
  ==============================================================================

    MainComponent.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MainIncludes.h"

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

class MainContentComponent :
	public OrganicMainContentComponent
{
public:
	//==============================================================================
	MainContentComponent();
	~MainContentComponent() override;

	std::unique_ptr<Component> aboutWindow;

	void init() override;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
	void getAllCommands(Array<CommandID>& commands) override;
	void fillFileMenuInternal(PopupMenu& menu) override;
	PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName) override;
	StringArray getMenuBarNames() override;
	bool perform(const InvocationInfo& info) override;
};
