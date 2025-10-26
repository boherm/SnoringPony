/*
  ==============================================================================

    AudioInterfaceHardwareEditor.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioInterfaceHardwareEditor.h"

AudioInterfaceHardwareEditor::AudioInterfaceHardwareEditor(AudioModuleHardwareSettings* hs, bool isRoot) :
	GenericControllableContainerEditor(hs, isRoot),
	hs(hs),
	selector(*hs->am, 0, 0, 0, 64, false, false, false, false)
{
	selector.setSize(100, 300);
	selector.setVisible(!container->editorIsCollapsed);
	addAndMakeVisible(selector);
	if (selector.isVisible()) setSize(100, selector.getBottom() + headerHeight + headerGap);
}

AudioInterfaceHardwareEditor::~AudioInterfaceHardwareEditor()
{
}

void AudioInterfaceHardwareEditor::setCollapsed(bool value, bool force, bool animate, bool doNotRebuild)
{
	GenericControllableContainerEditor::setCollapsed(value, force, animate, doNotRebuild);
	selector.setVisible(!container->editorIsCollapsed);
}

void AudioInterfaceHardwareEditor::resizedInternalContent(Rectangle<int>& r)
{
	selector.setBounds(r.withHeight(selector.getHeight()));
	r.setY(selector.getHeight());
	r.setHeight(0);
}
