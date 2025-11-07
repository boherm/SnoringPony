/*
  ==============================================================================

    ShowInfos.cpp
    Created: 4 Nov 2025 10:50:23am
    Author:  boherm

  ==============================================================================
*/

#include "ShowInfos.h"
#include "../../PonyEngine.h"
#include "../../Cue/Cue.h"
#include "../../Cuelist/Cuelist.h"

//==============================================================================

ShowInfosUI::ShowInfosUI(const String &contentName):
    ShapeShifterContent(ShowInfos::getInstance(), contentName) { }

ShowInfosUI::~ShowInfosUI() { }

//==============================================================================

juce_ImplementSingleton(ShowInfos);

ShowInfos::ShowInfos():
    ControllableContainer("Show Infos")
{
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.mainCuelist->addParameterListener(this);

    mainCuelist = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();

    if (mainCuelist != nullptr)
        mainCuelist->nextCue->addParameterListener(this);
}

ShowInfos::~ShowInfos()
{
    if (mainCuelist != nullptr)
        mainCuelist->nextCue->removeParameterListener(this);

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.mainCuelist->removeParameterListener(this);
}

void ShowInfos::paint(juce::Graphics &g)
{
    g.fillAll(BG_COLOR);

    g.setFont(20.0f);
    g.setColour(Colours::white);
    g.drawText(">", getLocalBounds().reduced(5), Justification::topLeft);

    // g.drawText("1.000: Lorem ipsum dolor sit amet, consectetur adipiscing elit, praesent tincidunt agueu mi", getLocalBounds().reduced(25, 8), Justification::topLeft);

    g.setColour(Colours::white.darker(0.6f));
    g.drawText("|", getLocalBounds().reduced(5).withY(32).withX(7), Justification::topLeft);

    if (mainCuelist && mainCuelist->nextCue->getTargetContainerAs<Cue>()) {
        Cue* c = mainCuelist->nextCue->getTargetContainerAs<Cue>();
        g.drawText(c->id->stringValue() + ": " + c->description->stringValue(), getLocalBounds().reduced(25, 8).withY(32), Justification::topLeft);
    } else {
        g.drawText("(no cue next)", getLocalBounds().reduced(25, 8).withY(32), Justification::topLeft);
    }
}

void ShowInfos::resized()
{
}

void ShowInfos::parameterValueChanged(Parameter* p)
{
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);

    if (p == engine->showProperties.mainCuelist)
    {
        if (mainCuelist != nullptr)
            mainCuelist->nextCue->removeParameterListener(this);

        mainCuelist = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();

        if (mainCuelist != nullptr)
            mainCuelist->nextCue->addParameterListener(this);

        repaint();
    }

    if (mainCuelist && p == mainCuelist->nextCue) {
        repaint();
    }
}
