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

	void parameterValueChanged(Parameter* parameter) override;
	void paint(Graphics& g) override;
	void paintOverChildren(Graphics& g) override;
	void resized() override;
};
