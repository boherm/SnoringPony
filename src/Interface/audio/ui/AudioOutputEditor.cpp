/*
  ==============================================================================

    AudioOutputEditor.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioOutputEditor.h"
#include "../AudioOutput.h"

AudioOutputEditor::AudioOutputEditor(AudioOutput* output, bool isRoot) :
    BaseItemEditor(output, isRoot),
	output(output)
{
	testUI.reset(output->testTrigger->createButtonUI());

	testUI->useCustomBGColor = true;
	testUI->customBGColor = PANEL_COLOR;
	addAndMakeVisible(testUI.get());
}

AudioOutputEditor::~AudioOutputEditor()
{
}

void AudioOutputEditor::resizedInternalHeaderItemInternal(Rectangle<int>& r)
{
	testUI->setBounds(r.removeFromRight(60).reduced(2));
	r.removeFromRight(2);
}
