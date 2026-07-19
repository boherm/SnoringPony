/*
  ==============================================================================

    MainMenuBarComponent.cpp
    Created: 25 Sep 2025 01:43:00pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MainIncludes.h"

class MainContentComponent;
class PonyEngine;
class ShowProperties;

class MainMenuBarComponent :
	public Component,
	public ParameterListener
{
public:
	MainMenuBarComponent(MainContentComponent* mainComp, PonyEngine* engine);
	~MainMenuBarComponent();

#if !JUCE_MAC
	MenuBarComponent menuBarComp;
#endif

    ShowProperties* showProperties;

    // Clickable bounds of the redundancy badge (updated in paintOverChildren); clicking it opens
    // the redundancy system in the inspector.
    juce::Rectangle<int> redundancyBadgeBounds;

	void parameterValueChanged(Parameter* parameter) override;
	void paint(Graphics& g) override;
	void paintOverChildren(Graphics& g) override;
	void resized() override;
    void mouseDown(const MouseEvent& event) override;
};
