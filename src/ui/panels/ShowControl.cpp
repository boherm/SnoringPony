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
#include "../../Cuelist/CuelistManager.h"

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

    // GO still targets the main cuelist's next cue.
    if (mainCuelist != nullptr)
        mainCuelist->nextCue->addParameterListener(this);

    // Panic is global: watch every cuelist's play / panic state.
    CuelistManager::getInstance()->addBaseManagerListener(this);
    for (auto* cl : CuelistManager::getInstance()->items)
        addCuelistPanicListeners(cl);

    refreshPanicState();
}

ShowControl::~ShowControl()
{
    stopTimer();
    delete btnGo;
    delete btnPanic;

    if (mainCuelist != nullptr)
        mainCuelist->nextCue->removeParameterListener(this);

    if (auto* clm = CuelistManager::getInstanceWithoutCreating())
    {
        clm->removeBaseManagerListener(this);
        for (auto* cl : clm->items)
            removeCuelistPanicListeners(cl);
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
    // Don't react while the engine tears down (cuelists clearing fire play/panic changes);
    // touching singletons here would recreate ones already deleted (freeze + assert on quit).
    if (Engine::mainEngine == nullptr || Engine::mainEngine->isClearing) return;

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);

    // Main cuelist retargeted: GO follows it (Panic stays global, handled below).
    if (p == engine->showProperties.mainCuelist) {
        if (mainCuelist != nullptr)
            mainCuelist->nextCue->removeParameterListener(this);

        mainCuelist = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();

        if (mainCuelist != nullptr) {
            mainCuelist->nextCue->addParameterListener(this);
            paramGo->setEnabled(mainCuelist->nextCue->getTargetContainerAs<Cue>() != nullptr);
        } else {
            paramGo->setEnabled(false);
        }
        repaint();
        return;
    }

    if (mainCuelist && p == mainCuelist->nextCue) {
        paramGo->setEnabled(mainCuelist->nextCue->getTargetContainerAs<Cue>() != nullptr);
        repaint();
        return;
    }

    // Any cuelist's play / panic state changed -> refresh the global Panic button.
    if (auto* clm = CuelistManager::getInstanceWithoutCreating())
        for (auto* cl : clm->items) {
            if (cl == nullptr) continue;
            if (p == cl->isPlaying || p == cl->isPanicking) {
                refreshPanicState();
                return;
            }
        }
}

bool ShowControl::anyCuelistPlaying() const
{
    auto* clm = CuelistManager::getInstanceWithoutCreating();
    if (clm == nullptr) return false;
    for (auto* cl : clm->items)
        if (cl != nullptr && cl->isPlaying->boolValue()) return true;
    return false;
}

bool ShowControl::anyCuelistPanicking() const
{
    auto* clm = CuelistManager::getInstanceWithoutCreating();
    if (clm == nullptr) return false;
    for (auto* cl : clm->items)
        if (cl != nullptr && cl->isPanicking->boolValue()) return true;
    return false;
}

void ShowControl::addCuelistPanicListeners(Cuelist* cl)
{
    if (cl == nullptr) return;
    cl->isPlaying->addParameterListener(this);
    cl->isPanicking->addParameterListener(this);
}

void ShowControl::removeCuelistPanicListeners(Cuelist* cl)
{
    if (cl == nullptr) return;
    cl->isPlaying->removeParameterListener(this);
    cl->isPanicking->removeParameterListener(this);
}

void ShowControl::refreshPanicState()
{
    if (Engine::mainEngine == nullptr || Engine::mainEngine->isClearing) return;

    const bool panicking = anyCuelistPanicking();
    // Keep the button available while anything plays or is panicking.
    paramPanic->setEnabled(anyCuelistPlaying() || panicking);

    if (panicking) startPanicking();
    else stopPanicking();

    repaint();
}

void ShowControl::itemAdded(Cuelist* c) { addCuelistPanicListeners(c); refreshPanicState(); }
void ShowControl::itemsAdded(Array<Cuelist*> items) { for (auto* c : items) addCuelistPanicListeners(c); refreshPanicState(); }
void ShowControl::itemRemoved(Cuelist* c) { removeCuelistPanicListeners(c); refreshPanicState(); }
void ShowControl::itemsRemoved(Array<Cuelist*> items) { for (auto* c : items) removeCuelistPanicListeners(c); refreshPanicState(); }

void ShowControl::startPanicking()
{
    if (!btnPanic->isEnabled())
        return;

    startTimer(150);
    isPanicking->setValue(true);
    btnPanic->customLabel = "Don't\nPanic!";
    btnPanic->customTextSize = 22;
}

void ShowControl::stopPanicking()
{
    stopTimer();
    isPanicking->setValue(false);
    btnPanic->customBGColor = Colours::darkred;
    btnPanic->customLabel = "";
    btnPanic->customTextSize = 30;
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
