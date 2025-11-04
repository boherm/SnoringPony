/*
  ==============================================================================

    ShowInfos.cpp
    Created: 4 Nov 2025 10:50:23am
    Author:  boherm

  ==============================================================================
*/

#include "ShowInfos.h"
#include "juce_organicui/ui/Style.h"

//==============================================================================

ShowInfosUI::ShowInfosUI(const String &contentName):
    ShapeShifterContent(ShowInfos::getInstance(), contentName) { }

ShowInfosUI::~ShowInfosUI() { }

//==============================================================================

juce_ImplementSingleton(ShowInfos);

ShowInfos::ShowInfos():
    ControllableContainer("Show Infos") { }

ShowInfos::~ShowInfos() { }

void ShowInfos::paint(juce::Graphics &g)
{
    g.fillAll(BG_COLOR);

    g.setFont(18.0f);
    g.setColour(Colours::white);
    g.drawText(">", getLocalBounds().reduced(5), Justification::topLeft);

    g.drawText("1.000: Lorem ipsum dolor sit amet, consectetur adipiscing elit, praesent tincidunt agueu mi", getLocalBounds().reduced(25, 8), Justification::topLeft);

    g.setColour(Colours::white.darker(0.3f));
    g.drawText("|", getLocalBounds().reduced(5).withY(30).withX(7), Justification::topLeft);
    g.drawText("2.000: Lorem ipsum dolor sit amet, consectetur adipiscing elit, praesent tincidunt agueu mi", getLocalBounds().reduced(25, 8).withY(30), Justification::topLeft);
}

void ShowInfos::resized()
{
}
