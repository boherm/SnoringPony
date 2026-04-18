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

	std::unique_ptr<PluginLoader> pluginLoader;

	void clearInternal() override;
	void afterLoadFileInternal() override;

	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternalEngine(var data, ProgressTask * loadingTask) override;

	void childStructureChanged(ControllableContainer * cc) override;
	void controllableFeedbackUpdate(ControllableContainer * cc, Controllable * c) override;

	void handleAsyncUpdate() override;

	void importSelection(File f = File());
	void exportSelection();

	String getMinimumRequiredFileVersion() override;
};
