/*
  ==============================================================================

    AudioOutputEditor.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class AudioOutput;

class AudioOutputEditor :
	public BaseItemEditor
{
public:
	AudioOutputEditor(AudioOutput* output, bool isRoot);
	~AudioOutputEditor();

	std::unique_ptr<TriggerButtonUI> testUI;

	AudioOutput* output;

	void resizedInternalHeaderItemInternal(Rectangle<int> &r) override;
};
