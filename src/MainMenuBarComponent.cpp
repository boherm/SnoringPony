/*
  ==============================================================================

    MainMenuBarComponent.cpp
    Created: 25 Sep 2025 01:43:00pm
    Author:  boherm

  ==============================================================================
*/

#include "MainMenuBarComponent.h"
#include "Show/ShowProperties.h"
#include "juce_graphics/juce_graphics.h"

MainMenuBarComponent::MainMenuBarComponent(MainContentComponent* mainComp, PonyEngine* engine) :
	Component("Menu Bar")
#if !JUCE_MAC
	, menuBarComp(mainComp)
#endif
{
#if !JUCE_MAC
	addAndMakeVisible(menuBarComp);
#endif

    ShowProperties::getInstance()->projectName->addParameterListener(this);
    ShowProperties::getInstance()->companyName->addParameterListener(this);
    ShowProperties::getInstance()->showFileVersion->addParameterListener(this);
}

MainMenuBarComponent::~MainMenuBarComponent()
{
    ShowProperties::getInstance()->projectName->removeParameterListener(this);
}

void MainMenuBarComponent::paint(Graphics& g)
{
	g.fillAll(BG_COLOR);
    g.setColour(TEXTNAME_COLOR);
    

    Rectangle<int> *r = new Rectangle<int>(getLocalBounds().getX() + 5, getLocalBounds().getY(), getLocalBounds().getWidth() - 10, getLocalBounds().getHeight());

    String title = ShowProperties::getInstance()->companyName->stringValue() + " - " + 
        ShowProperties::getInstance()->projectName->stringValue() + " - " + 
        ShowProperties::getInstance()->showFileVersion->stringValue();
    
    g.drawText(title, *r, Justification::centred, 1);
    
    // Main badge
    // g.setColour(Colours::darkred);
    // g.fillRoundedRectangle(getLocalBounds().getWidth() - 55, getLocalBounds().getY() + 4, 50, 16, 5);
    // g.setColour(Colours::white);
    // g.drawText("Main", getLocalBounds().withX(getLocalBounds().getWidth() - 55).withY(4).withWidth(50).withHeight(16), Justification::centred, 1);
}

void MainMenuBarComponent::resized()
{
#if !JUCE_MAC	
	Rectangle<int> r = getLocalBounds();
	r.removeFromRight(300);
	menuBarComp.setBounds(r);
#endif
}

void MainMenuBarComponent::parameterValueChanged(Parameter* parameter)
{
    repaint();
}