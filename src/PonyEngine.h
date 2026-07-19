/*
  ==============================================================================

    PonyEngine.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MainIncludes.h"
#include "ProjectSettings/ShowProperties.h"
#include "ProjectSettings/AudioSettings.h"
#include "ProjectSettings/ColorPresets.h"
#include "ProjectSettings/VolumePresets.h"
#include "ProjectSettings/DecksSettings.h"
#include "ProjectSettings/MeteringSettings.h"
#include "RF/RFCoordinationSettings.h"
#include "Audio/PluginScanner.h"
#include "Audio/PluginSlot.h"

class PonyEngine : public Engine
{
public:
	PonyEngine();
	~PonyEngine();

    ShowProperties showProperties;
    AudioSettings audioSettings;
    ColorPresets colorPresets;
    VolumePresets volumePresets;
    DecksSettings decksSettings;
    MeteringSettings meteringSettings;
    RFCoordinationSettings rfSettings;

	std::unique_ptr<PluginLoader> pluginLoader;

	void clearInternal() override;
	void createNewGraphInternal() override;
	void afterLoadFileInternal() override;

	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternalEngine(var data, ProgressTask * loadingTask) override;

	void childStructureChanged(ControllableContainer * cc) override;
	void controllableFeedbackUpdate(ControllableContainer * cc, Controllable * c) override;

	void handleAsyncUpdate() override;

	void importSelection(File f = File());
	void exportSelection();

	void packageProject();

	String getMinimumRequiredFileVersion() override;

	// Hash of the currently-open show (the `hashVersion` stamped in the file at save time). Two
	// instances that opened the same file share this value — used by the redundancy feature to
	// detect when the Main and Backup have different shows open. 0 for a new/unsaved show.
	juce::int64 getCurrentShowHash() const { return loadedShowHash; }

private:
	juce::int64 loadedShowHash = 0;
	juce::Array<FileParameter*> collectAudioFileParameters();
	void choosePackageDestination(bool compress, bool openAfter);
	void runPackageExport(const juce::File& destinationDir, bool compress, bool openAfter);
};
