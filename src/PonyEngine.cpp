/*
  ==============================================================================

    PonyEngine.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Cue/audio/AudioCue.h"
#include "Cue/audio/AudioFile.h"
#include "Cue/playlist/PlaylistCue.h"
#include "Cue/CueManager.h"
#include "Cuelist/CuelistFactory.h"
#include "Cuelist/CuelistManager.h"
#include "Interface/action/MappingActionFactory.h"
#include "Interface/midi/MIDIFeedbackFactory.h"
#include "Interface/osc/OSCFeedbackFactory.h"
#include "Interface/midi/MIDIManager.h"
#include "PonyEngine.h"
#include "Brain.h"
#include "UserInputManager.h"
#include "ProjectSettings/ShowProperties.h"
#include "ProjectSettings/DecksSettings.h"
#include "ui/SPAssetManager.h"
#include "ui/panels/Clock.h"
#include "ui/panels/ShowControl.h"
#include "ui/panels/ShowInfos.h"
#include "ui/panels/MTCMonitor.h"
#include "ui/panels/ActiveCuesPanel.h"
#include "ui/panels/MeteringPanel.h"
#include "ui/panels/RFCoordinationPanel.h"
#include "RF/RFDeviceManager.h"
#include "RF/RFProfileManager.h"
#include "Interface/InterfaceManager.h"
#include "Redundancy/RedundancyManager.h"
#include "Timer/ShowTimerManager.h"

ControllableContainer* getAppSettings();
String getAppVersion();

PonyEngine::PonyEngine() :
	Engine("SnoringPony", ".indy"),
    showProperties(),
    audioSettings(),
    colorPresets(),
    volumePresets(),
    decksSettings(),
    rfSettings()
	//ossiaDevice(nullptr)
{
	//init here
	Engine::mainEngine = this;
	MIDIManager::getInstance();
	PluginScanner::getInstance(); // Init plugin scanner
	addChildControllableContainer(CuelistManager::getInstance());
    addChildControllableContainer(InterfaceManager::getInstance());
    addChildControllableContainer(ShowTimerManager::getInstance());
    addChildControllableContainer(ShowControl::getInstance());

	// Clean
	getAppSettings()->hideInEditor = true;
	GlobalSettings::getInstance()->getControllableContainerByName("editing")->hideInEditor = true;
	GlobalSettings::getInstance()->getControllableContainerByName("oscRemoteControl")
	->getControllableContainerByName("manualOSCSend")->editorIsCollapsed = true;
	GlobalSettings::getInstance()->getControllableContainerByName("launchArguments")->editorIsCollapsed = true;
    GlobalSettings::getInstance()->askBeforeRemovingItems->setValue(true);
	GlobalSettings::getInstance()->addChildControllableContainer(PluginScanner::getInstance());

	// Per-machine redundancy (Main/Backup) settings, persisted with the global settings, not the show.
	GlobalSettings::getInstance()->addChildControllableContainer(RedundancyManager::getInstance());
	RedundancyManager::getInstance()->applyStandbyToInterfaces();

	OSCRemoteControl::getInstance()->addRemoteControlListener(UserInputManager::getInstance());

    // Set projects settings
    ProjectSettings::getInstance()->addChildControllableContainer(&showProperties);
    ProjectSettings::getInstance()->addChildControllableContainer(&audioSettings);
    ProjectSettings::getInstance()->addChildControllableContainer(&colorPresets);
    ProjectSettings::getInstance()->addChildControllableContainer(&volumePresets);
    ProjectSettings::getInstance()->addChildControllableContainer(&decksSettings);
    ProjectSettings::getInstance()->addChildControllableContainer(&meteringSettings);
    ProjectSettings::getInstance()->addChildControllableContainer(&rfSettings);
    ProjectSettings::getInstance()->customValuesCC.hideInEditor = true;
    ProjectSettings::getInstance()->dashboardCC.editorIsCollapsed = true;

}

PonyEngine::~PonyEngine()
{
	//Application-end cleanup, nothing should be recreated after this
	//delete singletons here

	isClearing = true;
	OSCRemoteControl::getInstance()->removeRemoteControlListener(UserInputManager::getInstance());

    PluginScanner::deleteInstance();
	CuelistManager::deleteInstance();
    CuelistFactory::deleteInstance();
    MappingActionFactory::deleteInstance();
    MIDIFeedbackFactory::deleteInstance();
    OSCFeedbackFactory::deleteInstance();

    ShowTimerManager::deleteInstance();

    Clock::deleteInstance();
    MTCMonitor::deleteInstance();
    ActiveCuesPanel::deleteInstance();
    MeteringPanel::deleteInstance();
    RFCoordinationPanel::deleteInstance();
    RFDeviceManager::deleteInstance();
    RFProfileManager::deleteInstance();
    ShowControl::deleteInstance();
    ShowInfos::deleteInstance();

    SPAssetManager::deleteInstance();
    UserInputManager::deleteInstance();

    InterfaceManager::deleteInstance();
    MIDIManager::deleteInstance();
    RedundancyManager::deleteInstance();
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

    // Open/close the metering input to match the loaded "Active" state.
    meteringSettings.reconfigure();

    // Interfaces were (re)created during load; make sure they inherit the current standby state.
    RedundancyManager::getInstance()->applyStandbyToInterfaces();
}

void PonyEngine::clearInternal()
{
    // Stop any pending plugin loading before destroying cues
    pluginLoader.reset();

    // Clear deck settings first so DeckViewUIs release their cuelist references
    decksSettings.clear();

	//clear
	CuelistManager::getInstance()->clear();
    InterfaceManager::getInstance()->clear();
    ShowTimerManager::getInstance()->clear();

    showProperties.clear();
    audioSettings.clear();
    colorPresets.clear();
    volumePresets.clear();
    meteringSettings.clear();
    rfSettings.clear();
}

var PonyEngine::getJSONData(bool includeNonOverriden)
{
	var data = Engine::getJSONData(includeNonOverriden);

    var inData = InterfaceManager::getInstance()->getJSONData(includeNonOverriden);
    if (!inData.isVoid() && inData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(InterfaceManager::getInstance()->shortName, inData);

	var clData = CuelistManager::getInstance()->getJSONData(includeNonOverriden);
	if (!clData.isVoid() && clData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(CuelistManager::getInstance()->shortName, clData);

	var tData = ShowTimerManager::getInstance()->getJSONData(includeNonOverriden);
	if (!tData.isVoid() && tData.getDynamicObject()->getProperties().size() > 0) data.getDynamicObject()->setProperty(ShowTimerManager::getInstance()->shortName, tData);

    var metaData = data.getDynamicObject()->getProperty("metaData");
    metaData.getDynamicObject()->setProperty("hashVersion", data.toString().hashCode64());

	return data;
}

void PonyEngine::createNewGraphInternal()
{
	loadedShowHash = 0; // new/unsaved show

	// Every new show starts with one timer.
	ShowTimerManager::getInstance()->addItem();
}

void PonyEngine::loadJSONDataInternalEngine(var data, ProgressTask* loadingTask)
{
	// Remember the show's stored hash (for the redundancy same-show check).
	var md = data.getProperty("metaData", var());
	loadedShowHash = md.isObject() ? (juce::int64)md.getProperty("hashVersion", 0) : 0;


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

	ProgressTask* tTask = loadingTask->addTask("Timers");
	tTask->start();
	var tData = data.getProperty(ShowTimerManager::getInstance()->shortName, var());
	ShowTimerManager::getInstance()->loadJSONData(tData);
	tTask->setProgress(1);
	tTask->end();
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
		FileChooser* fc(new FileChooser("Load a LilPony", File::getCurrentWorkingDirectory(), "*.lilpony"));
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

    InterfaceManager::getInstance()->addItemsFromData(data.getProperty(InterfaceManager::getInstance()->shortName, var()));
	RFProfileManager::getInstance()->addItemsFromData(data.getProperty(RFProfileManager::getInstance()->shortName, var()));
    RFDeviceManager::getInstance()->addItemsFromData(data.getProperty(RFDeviceManager::getInstance()->shortName, var()));
    ShowTimerManager::getInstance()->addItemsFromData(data.getProperty(ShowTimerManager::getInstance()->shortName, var()));
	CuelistManager::getInstance()->addItemsFromData(data.getProperty(CuelistManager::getInstance()->shortName, var()));
}

void PonyEngine::exportSelection()
{
	var data(new DynamicObject());

    data.getDynamicObject()->setProperty(InterfaceManager::getInstance()->shortName, InterfaceManager::getInstance()->getExportSelectionData());
	data.getDynamicObject()->setProperty(RFProfileManager::getInstance()->shortName, RFProfileManager::getInstance()->getExportSelectionData());
    data.getDynamicObject()->setProperty(RFDeviceManager::getInstance()->shortName, RFDeviceManager::getInstance()->getExportSelectionData());
    data.getDynamicObject()->setProperty(ShowTimerManager::getInstance()->shortName, ShowTimerManager::getInstance()->getExportSelectionData());
	data.getDynamicObject()->setProperty(CuelistManager::getInstance()->shortName, CuelistManager::getInstance()->getExportSelectionData());

	String s = JSON::toString(data);

	FileChooser* fc(new FileChooser("Save a LilPony", File::getCurrentWorkingDirectory(), "*.lilpony"));
	fc->launchAsync(FileBrowserComponent::FileChooserFlags::saveMode | FileBrowserComponent::FileChooserFlags::canSelectFiles, [s](const FileChooser& fc)
		{
			File f = fc.getResult();
			delete& fc;
			if (f == File()) return;
			f.replaceWithText(s);
		}
	);
}

// Runs the slow part of packaging (copying audio files, then compressing) on a background
// thread with a progress window, so the UI stays responsive and shows what is happening.
// The .indy is already written and the live parameters restored before this starts.
class PackageProjectTask : public ThreadWithProgressWindow
{
public:
	PackageProjectTask(Array<std::pair<File, File>> jobs, bool compress, File packageRoot,
		File finalOutput, File indyFile, bool openAfter, StringArray missing, PonyEngine* engine) :
		ThreadWithProgressWindow("Packaging project...", true, true),
		copyJobs(std::move(jobs)), compress(compress), packageRoot(packageRoot),
		finalOutput(finalOutput), indyFile(indyFile), openAfter(openAfter),
		missingFiles(std::move(missing)), engine(engine)
	{
	}

	void run() override
	{
		int total = copyJobs.size() + (compress ? 1 : 0);
		if (total == 0) total = 1;
		int step = 0;

		for (auto& job : copyJobs)
		{
			if (threadShouldExit()) return;
			setStatusMessage("Copying " + job.second.getFileName() + "...");
			if (job.first.copyFileTo(job.second)) copied++;
			setProgress(++step / (double)total);
		}

		if (compress)
		{
			if (threadShouldExit()) return;
			setStatusMessage("Compressing archive...");

			ZipFile::Builder builder;
			Array<File> files;
			packageRoot.findChildFiles(files, File::findFiles, true);
			for (auto& f : files)
				builder.addFile(f, 9, packageRoot.getFileName() + "/" + f.getRelativePathFrom(packageRoot).replace("\\", "/"));

			if (std::unique_ptr<FileOutputStream> os = std::unique_ptr<FileOutputStream>(finalOutput.createOutputStream()))
			{
				zipOk = builder.writeToStream(*os, nullptr);
				os->flush();
			}

			packageRoot.deleteRecursively(); // keep only the archive
			setProgress(1.0);
		}
	}

	void threadComplete(bool userPressedCancel) override
	{
		if (userPressedCancel)
		{
			// Remove any partial output so nothing residual is left behind.
			packageRoot.deleteRecursively();
			if (compress) finalOutput.deleteFile();
			LOG("Package Project: cancelled.");
		}
		else if (compress && !zipOk)
		{
			LOGERROR("Package Project: failed to write the archive " << finalOutput.getFullPathName());
			AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Package Project",
				"Could not write the .zip archive.", "OK");
		}
		else
		{
			String msg = "Package created:\n" + finalOutput.getFullPathName()
				+ "\n\nAudio files copied: " + String(copied);
			if (!missingFiles.isEmpty())
			{
				msg += "\n\nMissing files (left unchanged): " + String(missingFiles.size());
				for (auto& m : missingFiles) msg += "\n  - " + m;
			}
			LOG(msg);
			AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Package Project", msg, "OK");

			// Opening a .zip makes no sense, so only switch to the package when it stays a folder.
			if (openAfter && !compress && engine != nullptr)
				engine->loadDocumentAsync(indyFile);
		}

		delete this;
	}

private:
	Array<std::pair<File, File>> copyJobs; // source -> destination inside sounds/
	bool compress;
	File packageRoot;
	File finalOutput;
	File indyFile;
	bool openAfter;
	StringArray missingFiles;
	PonyEngine* engine;
	int copied = 0;
	bool zipOk = false;
};

Array<FileParameter*> PonyEngine::collectAudioFileParameters()
{
	Array<FileParameter*> result;

	for (auto* cuelist : CuelistManager::getInstance()->items)
	{
		if (cuelist->cues == nullptr) continue;

		for (auto* cue : cuelist->cues->items)
		{
			if (auto* audioCue = dynamic_cast<AudioCue*>(cue))
			{
				if (audioCue->filesManager != nullptr)
					for (auto* f : audioCue->filesManager->items)
						if (f->audioFile != nullptr) result.add(f->audioFile);
			}
			else if (auto* playlistCue = dynamic_cast<PlaylistCue*>(cue))
			{
				if (playlistCue->filesManager != nullptr)
					for (auto* f : playlistCue->filesManager->items)
						if (f->audioFile != nullptr) result.add(f->audioFile);
			}
		}
	}

	return result;
}

void PonyEngine::packageProject()
{
	// The whole flow chains async dialogs: ask the packaging options, then a destination.
	auto askOptions = [this]()
	{
		auto* aw = new AlertWindow("Package Project",
			"All audio files used by Audio and Playlist cues will be copied into a \"sounds\" "
			"subfolder and their paths made relative to the project.",
			AlertWindow::QuestionIcon);

		StringArray yesNo;
		yesNo.add("No");
		yesNo.add("Yes");

		aw->addComboBox("compress", yesNo, "Compress to a .zip archive");
		aw->addComboBox("open", yesNo, "Open the packaged project afterwards");
		aw->getComboBoxComponent("compress")->setSelectedItemIndex(0);
		aw->getComboBoxComponent("open")->setSelectedItemIndex(0);

		aw->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));
		aw->addButton("Let's pack!", 1, KeyPress(KeyPress::returnKey));

		aw->enterModalState(true, ModalCallbackFunction::create([this, aw](int result)
			{
				bool compress = aw->getComboBoxComponent("compress")->getSelectedItemIndex() == 1;
				bool openAfter = aw->getComboBoxComponent("open")->getSelectedItemIndex() == 1;
				delete aw;

				if (result == 0) return;
				choosePackageDestination(compress, openAfter);
			}), false);
	};

	// Packaging relies on a saved, clean project: audio paths resolve against the .indy folder
	// and we reuse its file name. Offer to save first if the project is new or dirty.
	if (!getFile().existsAsFile() || hasChangedSinceSaved())
	{
		saveIfNeededAndUserAgreesAsync([this, askOptions](FileBasedDocument::SaveResult r)
			{
				if (r == FileBasedDocument::savedOk && getFile().existsAsFile()) askOptions();
			});
		return;
	}

	askOptions();
}

void PonyEngine::choosePackageDestination(bool compress, bool openAfter)
{
	FileChooser* fc(new FileChooser("Choose where to create the package", getFile().getParentDirectory()));
	fc->launchAsync(FileBrowserComponent::FileChooserFlags::openMode | FileBrowserComponent::FileChooserFlags::canSelectDirectories,
		[this, compress, openAfter](const FileChooser& fc)
		{
			File dir = fc.getResult();
			delete& fc;
			if (dir == File() || !dir.isDirectory()) return;
			runPackageExport(dir, compress, openAfter);
		}
	);
}

void PonyEngine::runPackageExport(const File& destinationDir, bool compress, bool openAfter)
{
	// Base name used for both the package folder and the .zip: "company - project - version".
	String base = File::createLegalFileName(
		showProperties.companyName->stringValue() + " - "
		+ showProperties.projectName->stringValue() + " - "
		+ showProperties.showFileVersion->stringValue());
	if (base.isEmpty()) base = "Package";

	// What ends up at the chosen location: a .zip archive, or the package folder itself.
	File finalOutput = compress ? destinationDir.getChildFile(base + ".zip")
		: destinationDir.getChildFile(base);

	// The message-thread part: create the folder, copy list + relative paths, write the .indy,
	// then hand the slow copy/compress work to a background task with a progress window.
	auto performBuild = [this, compress, openAfter, base, finalOutput]()
	{
		// When compressing we only keep the .zip, so build in a temp folder removed afterwards.
		File packageRoot = compress
			? File::getSpecialLocation(File::tempDirectory).getChildFile("SnoringPonyPackage").getChildFile(base)
			: finalOutput;

		if (packageRoot.exists()) packageRoot.deleteRecursively();
		Result created = packageRoot.createDirectory();
		if (created.failed())
		{
			LOGERROR("Package Project: could not create folder " << packageRoot.getFullPathName());
			AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Package Project",
				"Could not create the package folder. Check that you have write access to the chosen location.", "OK");
			return;
		}

		File soundsDir = packageRoot.getChildFile("sounds");
		soundsDir.createDirectory();

		Array<FileParameter*> params = collectAudioFileParameters();

		// Snapshot so the currently open project is left untouched once packaging is done.
		struct Snapshot { FileParameter* p; String value; bool forceRel; bool forceAbs; String base; };
		Array<Snapshot> snapshots;

		Array<std::pair<File, File>> copyJobs; // deferred to the background task
		HashMap<String, String> sourceToName;  // absolute source path -> unique file name inside sounds/
		StringArray usedNames;
		StringArray missingFiles;

		for (auto* p : params)
		{
			snapshots.add({ p, p->stringValue(), p->forceRelativePath, p->forceAbsolutePath, p->customBasePath });

			File src = p->getFile();
			if (!src.existsAsFile())
			{
				// Report by the missing path, or by the owning file's name when the path is empty.
				String label = src.getFullPathName();
				if (label.isEmpty()) label = p->parentContainer != nullptr ? p->parentContainer->niceName : p->niceName;
				missingFiles.addIfNotAlreadyThere(label);
				continue;
			}

			String srcKey = src.getFullPathName();
			String name;

			if (sourceToName.contains(srcKey))
			{
				name = sourceToName[srcKey];
			}
			else
			{
				// Resolve name collisions between files coming from different folders.
				name = src.getFileName();
				int idx = 2;
				while (usedNames.contains(name.toLowerCase()))
					name = src.getFileNameWithoutExtension() + " (" + String(idx++) + ")" + src.getFileExtension();

				usedNames.add(name.toLowerCase());
				copyJobs.add({ src, soundsDir.getChildFile(name) });
				sourceToName.set(srcKey, name);
			}

			// Repoint the parameter to the packaged copy, relative to the package root, silently
			// (no listener notifications, so players are not reloaded during packaging). The
			// relative value is set directly, so it holds even though the copy happens later.
			p->customBasePath = packageRoot.getFullPathName();
			p->forceRelativePath = true;
			p->forceAbsolutePath = false;
			p->setValue("sounds/" + name, true, true);
		}

		// Write the packaged .indy from the now-relative live state.
		File indyFile = packageRoot.getChildFile(getFile().getFileName());
		var data = getJSONData();
		if (indyFile.exists()) indyFile.deleteFile();
		if (std::unique_ptr<OutputStream> os = indyFile.createOutputStream())
		{
			JSON::writeToStream(*os, data);
			os->flush();
		}

		// Restore every parameter to its original value so the open project is unchanged.
		for (auto& s : snapshots)
		{
			s.p->customBasePath = s.base;
			s.p->forceRelativePath = s.forceRel;
			s.p->forceAbsolutePath = s.forceAbs;
			s.p->setValue(s.value, true, true);
		}

		// Copy the audio files (and compress) on a background thread with a progress window.
		(new PackageProjectTask(std::move(copyJobs), compress, packageRoot, finalOutput,
			indyFile, openAfter, std::move(missingFiles), this))->launchThread();
	};

	// Ask before overwriting an existing package: if the user agrees, delete it first (folder or
	// archive) so no residual files remain; otherwise cancel.
	if (finalOutput.exists())
	{
		String what = compress ? "A .zip archive" : "A folder";
		AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, "Package Project",
			what + " named \"" + finalOutput.getFileName() + "\" already exists at this location.\n\nReplace it?",
			"Cancel", "Replace", nullptr,
			ModalCallbackFunction::create([performBuild, finalOutput](int result)
				{
					if (result == 1) return; // Cancel
					finalOutput.deleteRecursively();
					performBuild();
				}));
		return;
	}

	performBuild();
}

String PonyEngine::getMinimumRequiredFileVersion()
{
	return "1.0.0b1";
}
