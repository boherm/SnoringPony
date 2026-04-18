/*
  ==============================================================================

    PonyEngine.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Cue/audio/AudioCue.h"
#include "Cue/playlist/PlaylistCue.h"
#include "Cue/CueManager.h"
#include "Cuelist/CuelistFactory.h"
#include "Cuelist/CuelistManager.h"
#include "Interface/action/MappingActionFactory.h"
#include "Interface/midi/MIDIManager.h"
#include "MainIncludes.h"
#include "PonyEngine.h"
#include "Brain.h"
#include "UserInputManager.h"
#include "ProjectSettings/ShowProperties.h"
#include "ProjectSettings/DecksSettings.h"
#include "ui/SPAssetManager.h"
#include "ui/panels/Clock.h"
#include "ui/panels/ShowControl.h"
#include "ui/panels/ShowInfos.h"
#include "Interface/InterfaceManager.h"

ControllableContainer* getAppSettings();
String getAppVersion();

PonyEngine::PonyEngine() :
	Engine("SnoringPony", ".indy"),
    showProperties(),
    audioSettings(),
    colorPresets(),
    volumePresets(),
    decksSettings()
	//ossiaDevice(nullptr)
{
	//init here
	Engine::mainEngine = this;
	MIDIManager::getInstance(); // Init MIDI device tracking early
	PluginScanner::getInstance(); // Init plugin scanner
	addChildControllableContainer(CuelistManager::getInstance());
    addChildControllableContainer(InterfaceManager::getInstance());
    addChildControllableContainer(ShowControl::getInstance());

	// Clean
	getAppSettings()->hideInEditor = true;
	GlobalSettings::getInstance()->getControllableContainerByName("editing")->hideInEditor = true;
	GlobalSettings::getInstance()->getControllableContainerByName("oscRemoteControl")
	->getControllableContainerByName("manualOSCSend")->editorIsCollapsed = true;
	GlobalSettings::getInstance()->getControllableContainerByName("launchArguments")->editorIsCollapsed = true;
    GlobalSettings::getInstance()->askBeforeRemovingItems->setValue(true);
	GlobalSettings::getInstance()->addChildControllableContainer(PluginScanner::getInstance());

	OSCRemoteControl::getInstance()->addRemoteControlListener(UserInputManager::getInstance());

    // Set projects settings
    ProjectSettings::getInstance()->addChildControllableContainer(&showProperties);
    ProjectSettings::getInstance()->addChildControllableContainer(&audioSettings);
    ProjectSettings::getInstance()->addChildControllableContainer(&colorPresets);
    ProjectSettings::getInstance()->addChildControllableContainer(&volumePresets);
    ProjectSettings::getInstance()->addChildControllableContainer(&decksSettings);
    ProjectSettings::getInstance()->customValuesCC.hideInEditor = true;
    ProjectSettings::getInstance()->dashboardCC.editorIsCollapsed = true;

    // Warm up audio HAL so that creating an Audio interface later is instant.
    // On macOS, AudioDeviceManager can be constructed on a background thread,
    // so we do it without blocking startup.
    #if JUCE_MAC
        std::thread([]() { AudioDeviceManager warmup; }).detach();
    #endif
}

PonyEngine::~PonyEngine()
{
	//Application-end cleanup, nothing should be recreated after this
	//delete singletons here

	isClearing = true;
	OSCRemoteControl::getInstance()->removeRemoteControlListener(UserInputManager::getInstance());

    PluginScanner::deleteInstance();
    InterfaceManager::deleteInstance();
    MIDIManager::deleteInstance();
    MappingActionFactory::deleteInstance();
	CuelistManager::deleteInstance();
    CuelistFactory::deleteInstance();

    Clock::deleteInstance();
    ShowControl::deleteInstance();
    ShowInfos::deleteInstance();

    SPAssetManager::deleteInstance();
    UserInputManager::deleteInstance();

    Brain::deleteInstance();
}

void PonyEngine::afterLoadFileInternal()
{
    Array<PluginSlot*> pendingSlots;

    for (auto* cuelist : CuelistManager::getInstance()->items)
    {
        if (cuelist->cues == nullptr) continue;

        for (auto* cue : cuelist->cues->items)
        {
            PluginChainManager* chain = nullptr;

            if (auto* audioCue = dynamic_cast<AudioCue*>(cue))
                chain = audioCue->pluginChainManager;
            else if (auto* playlistCue = dynamic_cast<PlaylistCue*>(cue))
                chain = playlistCue->pluginChainManager;

            if (chain != nullptr)
            {
                for (auto* slot : chain->items)
                {
                    if (slot->hasPendingLoad())
                        pendingSlots.add(slot);
                }
            }
        }
    }

    if (!pendingSlots.isEmpty())
    {
        pluginLoader = std::make_unique<PluginLoader>(pendingSlots);
    }
}

void PonyEngine::clearInternal()
{
    // Stop any pending plugin loading before destroying cues
    pluginLoader.reset();

	//clear
    InterfaceManager::getInstance()->clear();
	CuelistManager::getInstance()->clear();

    showProperties.clear();
    audioSettings.clear();
    colorPresets.clear();
    volumePresets.clear();
    decksSettings.clear();
}

var PonyEngine::getJSONData(bool includeNonOverriden)
{
	var data = Engine::getJSONData(includeNonOverriden);

    var inData = InterfaceManager::getInstance()->getJSONData(includeNonOverriden);
    if (!inData.isVoid() && inData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(InterfaceManager::getInstance()->shortName, inData);

	var clData = CuelistManager::getInstance()->getJSONData(includeNonOverriden);
	if (!clData.isVoid() && clData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(CuelistManager::getInstance()->shortName, clData);

	return data;
}

void PonyEngine::loadJSONDataInternalEngine(var data, ProgressTask* loadingTask)
{
	ProgressTask* inTask = loadingTask->addTask("Interfaces");
	inTask->start();
	var inData = data.getProperty(InterfaceManager::getInstance()->shortName, var());
	InterfaceManager::getInstance()->loadJSONData(inData);
	inTask->setProgress(1);
	inTask->end();

	ProgressTask* clTask = loadingTask->addTask("CueLists");
	clTask->start();
	var clData = data.getProperty(CuelistManager::getInstance()->shortName, var());
	CuelistManager::getInstance()->loadJSONData(clData);
	clTask->setProgress(1);
	clTask->end();
}

void PonyEngine::childStructureChanged(ControllableContainer* cc)
{
	Engine::childStructureChanged(cc);
}

void PonyEngine::controllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	if (isClearing || isLoadingFile) return;
}

void PonyEngine::handleAsyncUpdate()
{
	Engine::handleAsyncUpdate();
}

void PonyEngine::importSelection(File f)
{
	if (!f.existsAsFile())
	{
		FileChooser* fc(new FileChooser("Load a LilNut", File::getCurrentWorkingDirectory(), "*.lilnut"));
		fc->launchAsync(FileBrowserComponent::FileChooserFlags::openMode | FileBrowserComponent::FileChooserFlags::canSelectFiles, [this](const FileChooser& fc)
			{
				File f = fc.getResult();
				delete& fc;
				if (f == File()) return;
				importSelection(f);
			}
		);
		return;
	}

	var data = JSON::parse(f);
	if (!data.isObject()) return;

	CuelistManager::getInstance()->addItemsFromData(data.getProperty(CuelistManager::getInstance()->shortName, var()));
}

void PonyEngine::exportSelection()
{
	var data(new DynamicObject());

	data.getDynamicObject()->setProperty(CuelistManager::getInstance()->shortName, CuelistManager::getInstance()->getExportSelectionData());

	String s = JSON::toString(data);

	FileChooser* fc(new FileChooser("Save a LilNut", File::getCurrentWorkingDirectory(), "*.lilnut"));
	fc->launchAsync(FileBrowserComponent::FileChooserFlags::saveMode | FileBrowserComponent::FileChooserFlags::canSelectFiles, [s](const FileChooser& fc)
		{
			File f = fc.getResult();
			delete& fc;
			if (f == File()) return;
			f.replaceWithText(s);
		}
	);
}

String PonyEngine::getMinimumRequiredFileVersion()
{
	return "1.0.0b1";
}
