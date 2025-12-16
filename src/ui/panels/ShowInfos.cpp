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
}

ShowInfos::~ShowInfos()
{
    if (currentCue != nullptr) {
        currentCue->id->removeParameterListener(this);
        currentCue->description->removeParameterListener(this);
        currentCue = nullptr;
    }

    if (nextCue != nullptr) {
        nextCue->id->removeParameterListener(this);
        nextCue->description->removeParameterListener(this);
        nextCue->notes->removeParameterListener(this);
        nextCue = nullptr;
    }

    if (mainCuelist != nullptr) {
        mainCuelist->currentCue->removeParameterListener(this);
        mainCuelist->nextCue->removeParameterListener(this);
        mainCuelist = nullptr;
    }

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.mainCuelist->removeParameterListener(this);
}

void ShowInfos::paint(juce::Graphics &g)
{
    auto area = getLocalBounds().reduced(5, 10);

    g.fillAll(BG_COLOR);

    // Current cue line
    g.setFont(20.0f);
    g.setColour(Colours::white);
    g.drawText(">", area, Justification::topLeft);
    if (currentCue != nullptr) {
        g.drawText(currentCue->id->stringValue() + ": " + currentCue->getDescription(), area.withX(25), Justification::topLeft);
    } else {
        g.setColour(Colours::white.darker(0.6f));
        g.drawText("(no cue now)", area.withX(25), Justification::topLeft);
    }

    // Next cue with notes
    area.removeFromTop(26);
    g.setColour(Colours::white.darker(0.6f));
    g.drawText("|", area.withX(5), Justification::topLeft);
    if (nextCue != nullptr) {
        g.drawText(nextCue->id->stringValue() + ": " + nextCue->getDescription(), area.withX(25), Justification::topLeft);
    } else {
        g.drawText("(no cue next)", area.withX(25), Justification::topLeft);
    }

    if (nextCue != nullptr) {
        area.removeFromTop(40);
        g.setFont(16.0f);
        g.drawMultiLineText(nextCue->notes->stringValue(), area.withX(25).getX(), area.getY(), area.getWidth() - 30, Justification::topLeft, 1.2f);
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
        if (nextCue != nullptr) {
            nextCue->id->removeParameterListener(this);
            nextCue->description->removeParameterListener(this);
            nextCue->notes->removeParameterListener(this);
            nextCue = nullptr;
        }

        if (mainCuelist != nullptr) {
            mainCuelist->currentCue->removeParameterListener(this);
            mainCuelist->nextCue->removeParameterListener(this);
        }

        mainCuelist = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();

        if (mainCuelist != nullptr) {
            mainCuelist->currentCue->addParameterListener(this);
            mainCuelist->nextCue->addParameterListener(this);
            Cue* nc = mainCuelist->nextCue->getTargetContainerAs<Cue>();
            nextCue = nc;
            if (nc != nullptr) {
                nc->id->addParameterListener(this);
                nc->description->addParameterListener(this);
                nc->notes->addParameterListener(this);
            }
        }
    }

    if (mainCuelist && p == mainCuelist->currentCue)
    {
        if (currentCue != nullptr) {
            currentCue->id->removeParameterListener(this);
            currentCue->description->removeParameterListener(this);
        }
        Cue* cc = mainCuelist->currentCue->getTargetContainerAs<Cue>();
        currentCue = cc;
        if (cc != nullptr) {
            cc->id->addParameterListener(this);
            cc->description->addParameterListener(this);
        }
    }

    if (mainCuelist && p == mainCuelist->nextCue)
    {
        if (nextCue != nullptr) {
            nextCue->id->removeParameterListener(this);
            nextCue->description->removeParameterListener(this);
            nextCue->notes->removeParameterListener(this);
        }
        Cue* nc = mainCuelist->nextCue->getTargetContainerAs<Cue>();
        nextCue = nc;
        if (nc != nullptr) {
            nc->id->addParameterListener(this);
            nc->description->addParameterListener(this);
            nc->notes->addParameterListener(this);
        }
    }

    repaint();
}
