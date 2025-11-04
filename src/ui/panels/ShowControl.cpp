/*
  ==============================================================================

    ShowControl.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "ShowControl.h"

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
    btnGo->customBGColor = Colour(0, 139, 0);
    btnGo->useCustomBGColor = true;
    paramGo->setEnabled(false);

    paramPanic = addTrigger("Panic", "Push in case of emergency!");
    btnPanic = paramPanic->createButtonUI();
    addAndMakeVisible(btnPanic);
    btnPanic->customBGColor = Colour(159, 0, 0);
    btnPanic->useCustomBGColor = true;
    paramPanic->setEnabled(false);
}

ShowControl::~ShowControl() {
    delete btnGo;
    delete btnPanic;
}

void ShowControl::paint(juce::Graphics &g) { }

void ShowControl::resized() {
    btnGo->setBounds(0, 0, getLocalBounds().getWidth() / 2, getLocalBounds().getHeight());
    btnPanic->setBounds(getLocalBounds().getWidth() / 2, 0, getLocalBounds().getWidth() / 2, getLocalBounds().getHeight());
}

void ShowControl::triggerTriggered(Trigger *t) {
    Logger::getCurrentLogger()->writeToLog("Push: " + t->niceName);
    // UserInputManager::getInstance()->processInput(t->niceName);
}
