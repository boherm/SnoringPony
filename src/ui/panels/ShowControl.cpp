/*
  ==============================================================================

    ShowControl.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "ShowControl.h"
#include "../../PonyEngine.h"
#include "../../Brain.h"
#include "../../Cue/Cue.h"
#include "../../Cuelist/Cuelist.h"

//==============================================================================

ShowControlUI::ShowControlUI(const String &contentName):
    ShapeShifterContent(ShowControl::getInstance(), contentName) { }

ShowControlUI::~ShowControlUI() { }

//==============================================================================

juce_ImplementSingleton(ShowControl);

ShowControl::ShowControl():
    ControllableContainer("Show Control")
{
    paramGo = addTrigger("GO", "GO the next cue");
    btnGo = paramGo->createButtonUI();
    addAndMakeVisible(btnGo);
    btnGo->customBGColor = Colours::darkgreen;
    btnGo->useCustomBGColor = true;
    btnGo->customTextSize = 30;
    paramGo->setEnabled(false);

    paramPanic = addTrigger("Panic", "Push in case of emergency!");
    btnPanic = paramPanic->createButtonUI();
    addAndMakeVisible(btnPanic);
    btnPanic->customBGColor = Colours::darkred;
    btnPanic->useCustomBGColor = true;
    btnPanic->customTextSize = 30;
    paramPanic->setEnabled(false);

    isPanicking = addBoolParameter("Is Panicking", "Is the show currently panicking?", false);
    isPanicking->hideInRemoteControl = true;
    isPanicking->setEnabled(false);

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.mainCuelist->addParameterListener(this);

    mainCuelist = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();

    if (mainCuelist != nullptr) {
        mainCuelist->nextCue->addParameterListener(this);
        mainCuelist->isPlaying->addParameterListener(this);
        mainCuelist->isPanicking->addParameterListener(this);
    }
}

ShowControl::~ShowControl()
{
    stopTimer();
    delete btnGo;
    delete btnPanic;

    if (mainCuelist != nullptr) {
        mainCuelist->nextCue->removeParameterListener(this);
        mainCuelist->isPlaying->removeParameterListener(this);
        mainCuelist->isPanicking->removeParameterListener(this);
    }

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.mainCuelist->removeParameterListener(this);
}

void ShowControl::paint(juce::Graphics &g) { }

void ShowControl::resized()
{
    btnGo->setBounds(0, 0, getLocalBounds().getWidth() / 2, getLocalBounds().getHeight());
    btnPanic->setBounds(getLocalBounds().getWidth() / 2, 0, getLocalBounds().getWidth() / 2, getLocalBounds().getHeight());
}

void ShowControl::triggerTriggered(Trigger *t)
{
    if (t == paramGo) {
        Brain::getInstance()->go();
    }

    if (t == paramPanic) {
        Brain::getInstance()->panic();
    }
}

void ShowControl::parameterValueChanged(Parameter *p)
{
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);

    if (p == engine->showProperties.mainCuelist) {
        if (mainCuelist != nullptr) {
            mainCuelist->nextCue->removeParameterListener(this);
            mainCuelist->isPlaying->removeParameterListener(this);
            mainCuelist->isPanicking->removeParameterListener(this);
        }

        mainCuelist = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();

        if (mainCuelist != nullptr) {
            mainCuelist->nextCue->addParameterListener(this);
            mainCuelist->isPlaying->addParameterListener(this);
            mainCuelist->isPanicking->addParameterListener(this);

            TargetParameter* tp = dynamic_cast<TargetParameter*>(mainCuelist->nextCue);
            Cue* nextCue = tp->getTargetContainerAs<Cue>();

            if (nextCue != nullptr) {
                paramGo->setEnabled(true);
            } else {
                paramGo->setEnabled(false);
            }
        }
        repaint();
    }

    if (mainCuelist && p == mainCuelist->nextCue) {
        TargetParameter* tp = dynamic_cast<TargetParameter*>(p);
        Cue* nextCue = tp->getTargetContainerAs<Cue>();

        if (nextCue != nullptr) {
            paramGo->setEnabled(true);
        } else {
            paramGo->setEnabled(false);
        }
        repaint();
    }

    if (mainCuelist && p == mainCuelist->isPlaying) {
        if (mainCuelist->isPlaying->boolValue()) {
            paramPanic->setEnabled(true);
        } else {
            paramPanic->setEnabled(false);
        }
        repaint();
    }

    if (mainCuelist && p == mainCuelist->isPanicking) {
        if (mainCuelist->isPanicking->boolValue()) {
            startPanicking();
        } else {
            stopPanicking();
        }
        repaint();
    }
}

void ShowControl::startPanicking()
{
    if (!btnPanic->isEnabled())
        return;

    startTimer(150);
    isPanicking->setValue(true);
    btnPanic->customLabel = "Don't\nPanic!";
}

void ShowControl::stopPanicking()
{
    stopTimer();
    isPanicking->setValue(false);
    btnPanic->customBGColor = Colours::darkred;
    btnPanic->customLabel = "";
}

void ShowControl::timerCallback()
{
    if (isPanicking->boolValue()) {

        if (btnPanic->customBGColor == Colours::darkred)
            btnPanic->customBGColor = Colours::red.darker(0.3f);
        else
            btnPanic->customBGColor = Colours::darkred;

        repaint();
    }
}
