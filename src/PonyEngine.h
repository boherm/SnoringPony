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
#include "ProjectSettings/ColorPresets.h"
#include "ProjectSettings/VolumePresets.h"
#include "ProjectSettings/DecksSettings.h"

class PonyEngine : public Engine
{
public:
	PonyEngine();
	~PonyEngine();

    ShowProperties showProperties;
    ColorPresets colorPresets;
    VolumePresets volumePresets;
    DecksSettings decksSettings;

	void clearInternal() override;

	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternalEngine(var data, ProgressTask * loadingTask) override;

	void childStructureChanged(ControllableContainer * cc) override;
	void controllableFeedbackUpdate(ControllableContainer * cc, Controllable * c) override;

	void handleAsyncUpdate() override;

	void importSelection(File f = File());
	void exportSelection();

	String getMinimumRequiredFileVersion() override;
};
