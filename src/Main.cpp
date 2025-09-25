/*
  ==============================================================================

    Main.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Main.h"
#include "MainMenuBarComponent.h"
#include "PonyEngine.h"
#include "MainComponent.h"

#if JUCE_MAC //for chmod
#include <sys/types.h>
#include <sys/stat.h>
#endif

//==============================================================================

SnoringPonyApplication::SnoringPonyApplication() :
	OrganicApplication("SnoringPony", true, ImageCache::getFromMemory(BinaryData::tray_icon_png, BinaryData::tray_icon_pngSize)),
	crashSent(false)
{
}

void SnoringPonyApplication::initialiseInternal(const String &)
{
	// Engine and MainContntComponent first
	engine.reset(new PonyEngine());
	if(useWindow) mainComponent.reset(new MainContentComponent());

	// Then, ...
	// Layout
	ShapeShifterManager::getInstance()->setDefaultFileData(BinaryData::default_ponylayout);
	ShapeShifterManager::getInstance()->setLayoutInformations("ponylayout", "SnoringPony/layouts");
	// App Updater (url update.json - base donwload)
	AppUpdater::getInstance()->setURLs("https://webhook.site/7fb6eddc-aebf-4783-bdc3-e9e3e0b80da2", "https://webhook.site/7fb6eddc-aebf-4783-bdc3-e9e3e0b80da2", "SnoringPony");
	// Help URL (http://benjamin.kuperberg.fr/chataigne/help/help_en.json)
	HelpBox::getInstance()->helpURL = URL("http://benjamin.kuperberg.fr/chataigne/help/");
	// Crashdumper
	CrashDumpUploader::getInstance()->init("https://webhook.site/7fb6eddc-aebf-4783-bdc3-e9e3e0b80da2", ImageCache::getFromMemory(BinaryData::crash_png, BinaryData::crash_pngSize));
}


void SnoringPonyApplication::afterInit()
{
	// Download dashboard server
	// DashboardManager::getInstance()->setupDownloadURL("https://webhook.site/7fb6eddc-aebf-4783-bdc3-e9e3e0b80da2");
	DashboardManager::getInstance()->setupDownloadURL("http://benjamin.kuperberg.fr/download/dashboard/dashboard.php?folder=dashboard");
	
	if (mainWindow != nullptr)
	{
		mainWindow->setMenuBarComponent(new MainMenuBarComponent((MainContentComponent*)mainComponent.get(), (PonyEngine*)engine.get()));
	}
}

void SnoringPonyApplication::shutdown()
{
	OrganicApplication::shutdown();
	AppUpdater::deleteInstance();
}

void SnoringPonyApplication::handleCrashed()
{
	OrganicApplication::handleCrashed();
}
