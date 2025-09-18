/*
  ==============================================================================

    MainComponentCommands.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "MainComponent.h"
#include "PonyEngine.h"
#include "ui/AboutWindow.h"

namespace PonyCommandIDs
{
	static const int showAbout = 0x60000;
	static const int gotoWebsite = 0x60001;
	static const int gotoDiscord = 0x60002;
	static const int gotoDocs = 0x60003;
	static const int postGithubIssue = 0x60004;
	static const int donate = 0x60005;
	static const int sponsor = 0x60055;
	static const int showWelcome = 0x60006;
	static const int gotoChangelog = 0x60007;

	static const int exportSelection = 0x800;
	static const int importSelection = 0x801;
}

void MainContentComponent::getCommandInfo(CommandID commandID, ApplicationCommandInfo& result)
{
	switch (commandID)
	{
	case PonyCommandIDs::showAbout:
		result.setInfo("About...", "", "General", result.readOnlyInKeyEditor);
		break;

	case PonyCommandIDs::showWelcome:
		result.setInfo("Show Welcome Screen...", "", "General", result.readOnlyInKeyEditor);
		break;

	case PonyCommandIDs::donate:
		result.setInfo("Be cool and donate", "", "General", result.readOnlyInKeyEditor);
		break;

	case PonyCommandIDs::sponsor:
		result.setInfo("Be even cooler and sponsor !", "", "General", result.readOnlyInKeyEditor);
		break;

	case PonyCommandIDs::gotoWebsite:
		result.setInfo("Go to website", "", "Help", result.readOnlyInKeyEditor);
		break;
	case PonyCommandIDs::gotoDiscord:
		result.setInfo("Go to Discord", "", "Help", result.readOnlyInKeyEditor);
		break;

	case PonyCommandIDs::gotoDocs:
		result.setInfo("Go to the Documentation", "", "Help", result.readOnlyInKeyEditor);
		break;

	case PonyCommandIDs::gotoChangelog:
		result.setInfo("See the changelog", "", "Help", result.readOnlyInKeyEditor);
		break;


	case PonyCommandIDs::postGithubIssue:
		result.setInfo("Post an issue on github", "", "Help", result.readOnlyInKeyEditor);
		break;

	case PonyCommandIDs::exportSelection:
		result.setInfo("Export Selection", "This will export the current selection as *.lilpony file that can be later imported", "File", result.readOnlyInKeyEditor);
		result.addDefaultKeypress(KeyPress::createFromDescription("s").getKeyCode(), ModifierKeys::altModifier);
		result.setActive(InspectableSelectionManager::mainSelectionManager->currentInspectables.size() > 0);
		break;

	case PonyCommandIDs::importSelection:
		result.setInfo("Import...", "This will import a *.lilpony file and add it to the current noisette", "File", result.readOnlyInKeyEditor);
		result.addDefaultKeypress(KeyPress::createFromDescription("o").getKeyCode(), ModifierKeys::altModifier);
		break;

	default:
		OrganicMainContentComponent::getCommandInfo(commandID, result);
		break;
	}
}

void MainContentComponent::getAllCommands(Array<CommandID>& commands) {
	OrganicMainContentComponent::getAllCommands(commands);
	
	const CommandID ids[] = {
		PonyCommandIDs::showAbout,
		PonyCommandIDs::showWelcome,
		PonyCommandIDs::donate,
		PonyCommandIDs::sponsor,
		PonyCommandIDs::gotoWebsite,
		PonyCommandIDs::gotoDiscord,
		PonyCommandIDs::gotoDocs,
		PonyCommandIDs::gotoChangelog,
		PonyCommandIDs::postGithubIssue,
		PonyCommandIDs::importSelection,
		PonyCommandIDs::exportSelection,
	};
	
	commands.addArray(ids, numElementsInArray(ids));
}

void MainContentComponent::fillFileMenuInternal(PopupMenu& menu)
{
	menu.addCommandItem(&getCommandManager(), PonyCommandIDs::importSelection);
	menu.addCommandItem(&getCommandManager(), PonyCommandIDs::exportSelection);
}

PopupMenu MainContentComponent::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
{
	PopupMenu menu = OrganicMainContentComponent::getMenuForIndex(topLevelMenuIndex, menuName);

	if (menuName == "Help")
	{
		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::showAbout);
//		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::showWelcome);
		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::donate);
		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::sponsor);
		menu.addSeparator();
		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::gotoWebsite);
//		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::gotoDiscord);
		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::gotoDocs);
		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::gotoChangelog);
		menu.addCommandItem(&getCommandManager(), PonyCommandIDs::postGithubIssue);
	}
	
	return menu;
}

bool MainContentComponent::perform(const InvocationInfo& info)
{
	switch (info.commandID)
	{


	case PonyCommandIDs::showAbout:
	{
		aboutWindow.reset(new AboutWindow());
		DialogWindow::showDialog("About", aboutWindow.get(), getTopLevelComponent(), Colours::transparentBlack, true);
	}
	break;

	case PonyCommandIDs::showWelcome:
	{
//		welcomeScreen.reset(new WelcomeScreen());
//		DialogWindow::showDialog("Welcome", welcomeScreen.get(), getTopLevelComponent(), Colours::black, true);
	}
	break;

	case PonyCommandIDs::donate:
		URL("https://www.paypal.me/boerms").launchInDefaultBrowser();
		break;

	case PonyCommandIDs::sponsor:
		URL("https://github.com/sponsors/boherm").launchInDefaultBrowser();
		break;

	case PonyCommandIDs::gotoWebsite:
		URL("https://www.snoringpony.app/").launchInDefaultBrowser();
		break;

	case PonyCommandIDs::gotoDiscord:
		// Go to Discord
		break;

	case PonyCommandIDs::gotoDocs:
		URL("https://docs.snoringpony.app").launchInDefaultBrowser();
		break;

	case PonyCommandIDs::gotoChangelog:
		URL("https://www.snoringpony.app/changelog").launchInDefaultBrowser();
		break;

	case PonyCommandIDs::postGithubIssue:
		URL("http://github.com/boherm/SnoringPony/issues").launchInDefaultBrowser();
		break;


	case PonyCommandIDs::exportSelection:
	{
		((PonyEngine*)Engine::mainEngine)->exportSelection();
	}
	break;

	case PonyCommandIDs::importSelection:
	{
		((PonyEngine*)Engine::mainEngine)->importSelection();
	}
	break;

	default:
		return OrganicMainContentComponent::perform(info);
	}

	return true;
}

StringArray MainContentComponent::getMenuBarNames()
{
	StringArray names = OrganicMainContentComponent::getMenuBarNames();
	names.add("Help");
	return names;
}
