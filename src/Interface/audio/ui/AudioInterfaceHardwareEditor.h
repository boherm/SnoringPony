/*
  ==============================================================================

    AudioInterfaceHardwareEditor.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class AudioModuleHardwareSettings;

class AudioInterfaceHardwareEditor :
	public GenericControllableContainerEditor
{
public:
	AudioInterfaceHardwareEditor(AudioModuleHardwareSettings* hs, bool isRoot);
	~AudioInterfaceHardwareEditor();

	AudioModuleHardwareSettings* hs;
	AudioDeviceSelectorComponent selector;

	void setCollapsed(bool value, bool force = false, bool animate = true, bool doNotRebuild = false) override;
	void resizedInternalContent(Rectangle<int> &r) override;
};
