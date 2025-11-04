/*
  ==============================================================================

    ShowControl.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "ShowControl.h"
#include "juce_organicui/settings/ProjectSettings.h"
#include "../../PonyEngine.h"

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
    paramGo->setEnabled(false);

    paramPanic = addTrigger("Panic", "Push in case of emergency!");
    btnPanic = paramPanic->createButtonUI();
    addAndMakeVisible(btnPanic);
    btnPanic->customBGColor = Colours::darkred;
    btnPanic->useCustomBGColor = true;
    paramPanic->setEnabled(false);

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.nextCueToGo->addParameterListener(this);
}

ShowControl::~ShowControl() {
    delete btnGo;
    delete btnPanic;

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.nextCueToGo->removeParameterListener(this);
}

void ShowControl::paint(juce::Graphics &g) { }

void ShowControl::resized() {
    btnGo->setBounds(0, 0, getLocalBounds().getWidth() / 2, getLocalBounds().getHeight());
    btnPanic->setBounds(getLocalBounds().getWidth() / 2, 0, getLocalBounds().getWidth() / 2, getLocalBounds().getHeight());
}

void ShowControl::triggerTriggered(Trigger *t) {
    if (t == paramGo) {
        // TODO: move this logic in engine directly?
        TargetParameter* tp = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.nextCueToGo;
        Cue* nextCue = tp->getTargetContainerAs<Cue>();
        if (nextCue != nullptr) {
            nextCue->play();
            tp->resetValue();
            tp->notifyValueChanged();
        }
    }
}

void ShowControl::parameterValueChanged(Parameter *p) {
    if (p == dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.nextCueToGo) {
        TargetParameter* tp = dynamic_cast<TargetParameter*>(p);
        Cue* nextCue = tp->getTargetContainerAs<Cue>();

        if (nextCue != nullptr) {
            paramGo->setEnabled(true);
        } else {
            paramGo->setEnabled(false);
        }
    }
}
