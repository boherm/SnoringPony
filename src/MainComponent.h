/*
  ==============================================================================

	This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "ui/AboutWindow.h"

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

class MainContentComponent :
	public OrganicMainContentComponent
{
public:
	//==============================================================================
	MainContentComponent();
	~MainContentComponent() override;

	std::unique_ptr<AboutWindow> aboutWindow;
	
	void init() override;
	
	void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
	void getAllCommands(Array<CommandID>& commands) override;
	void fillFileMenuInternal(PopupMenu& menu) override;
	PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName) override;
	StringArray getMenuBarNames() override;
	bool perform(const InvocationInfo& info) override;
};
