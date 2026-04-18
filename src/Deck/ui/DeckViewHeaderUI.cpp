/*
  ==============================================================================

    DeckViewHeaderUI.cpp
    Created: 05 Oct 2025 12:56:20am
    Author:  boherm

  ==============================================================================
*/

#include "DeckViewHeaderUI.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cue/CueManager.h"
#include "../../PonyEngine.h"
#include "../../ProjectSettings/ShowProperties.h"


DeckViewHeaderUI::DeckViewHeaderUI(Cuelist* cl) :
    currentCuelist(cl)
{
    currentCuelist->addAsyncContainerListener(this);

    if (auto* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine))
        engine->showProperties.addAsyncContainerListener(this);

    addItemBT.reset(AssetManager::getInstance()->getAddBT());
    addItemBT->setWantsKeyboardFocus(false);
    addItemBT->setTooltip("Add a new Cue");
    addAndMakeVisible(addItemBT.get());
    addItemBT->addListener(this);

    goBtnUI = std::make_unique<TextButton>("GO");
    goBtnUI->setColour(TextButton::buttonColourId, Colours::green.darker(.3f));
    goBtnUI->setColour(TextButton::textColourOffId, Colours::white);
    goBtnUI->setTooltip("Trigger next cue");
    goBtnUI->addListener(this);
    addAndMakeVisible(goBtnUI.get());

    panicBtnUI = std::make_unique<TextButton>("PANIC");
    panicBtnUI->setColour(TextButton::buttonColourId, Colours::red.darker(.3f));
    panicBtnUI->setColour(TextButton::textColourOffId, Colours::white);
    panicBtnUI->setTooltip("Stop all playing cues immediately");
    panicBtnUI->addListener(this);
    panicBtnUI->setEnabled(currentCuelist->isPlaying->boolValue());
    addAndMakeVisible(panicBtnUI.get());

    mainToggleBT = std::make_unique<TextButton>("Main");
    mainToggleBT->setTooltip("Set as main cuelist (controlled by Show Control panel)");
    mainToggleBT->setClickingTogglesState(false);
    mainToggleBT->setColour(TextButton::buttonColourId, BG_COLOR.brighter(.1f));
    mainToggleBT->setColour(TextButton::buttonOnColourId, Colours::orange.darker(.3f));
    mainToggleBT->setColour(TextButton::textColourOffId, Colours::white);
    mainToggleBT->setColour(TextButton::textColourOnId, Colours::white);
    mainToggleBT->addListener(this);
    addAndMakeVisible(mainToggleBT.get());

    updateMainToggleState();
}

DeckViewHeaderUI::~DeckViewHeaderUI()
{
    if (currentCuelist != nullptr)
        currentCuelist->removeAsyncContainerListener(this);
    if (auto* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine))
        engine->showProperties.removeAsyncContainerListener(this);
}

void DeckViewHeaderUI::paint(Graphics& g)
{
    if (currentCuelist == nullptr) return;

    g.setColour(currentCuelist->itemColor->getColor());
    g.fillRect(0, 0, getWidth(), 30);
    g.setColour(Colours::white);
    g.setFont (Font (15, Font::bold));
    g.drawText(currentCuelist->niceName, 0, 0, getWidth(), 30, Justification::centred, true);
}

void DeckViewHeaderUI::resized()
{
    addItemBT->setBounds(getWidth() - 24, 4, 20, 20);
    mainToggleBT->setBounds(getWidth() - 24 - 60, 4, 56, 22);
    goBtnUI->setBounds(4, 4, 60, 22);
    panicBtnUI->setBounds(68, 4, 70, 22);
}

void DeckViewHeaderUI::newMessage(const ContainerAsyncEvent& e)
{
    if (e.targetControllable == currentCuelist->itemColor) {
        repaint();
    }

    if (e.targetControllable == currentCuelist->isPanicking) {
        updatePanicAnimation();
    }

    if (e.targetControllable == currentCuelist->isPlaying) {
        if (panicBtnUI != nullptr)
            panicBtnUI->setEnabled(currentCuelist->isPlaying->boolValue());
    }

    if (auto* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine))
    {
        if (e.targetControllable == engine->showProperties.mainCuelist)
            updateMainToggleState();
    }
}

void DeckViewHeaderUI::updatePanicAnimation()
{
    if (panicBtnUI == nullptr) return;
    if (currentCuelist->isPanicking->boolValue())
    {
        startTimer(150);
    }
    else
    {
        stopTimer();
        panicFlashState = false;
        panicBtnUI->setColour(TextButton::buttonColourId, Colours::red.darker(.3f));
        panicBtnUI->repaint();
    }
}

void DeckViewHeaderUI::timerCallback()
{
    if (!currentCuelist->isPanicking->boolValue())
    {
        stopTimer();
        return;
    }
    panicFlashState = !panicFlashState;
    panicBtnUI->setColour(TextButton::buttonColourId,
        panicFlashState ? Colours::red.brighter(.2f) : Colours::red.darker(.3f));
    panicBtnUI->repaint();
}

void DeckViewHeaderUI::updateMainToggleState()
{
    auto* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    if (engine == nullptr || mainToggleBT == nullptr) return;
    bool isMain = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>() == currentCuelist;
    mainToggleBT->setToggleState(isMain, dontSendNotification);
}

void DeckViewHeaderUI::buttonClicked(Button* button)
{
    if (button == addItemBT.get() && currentCuelist)
    {
		currentCuelist->cues->factory.showCreateMenu([this](Cue* item)
			{
				if (item != nullptr)
				{
					this->currentCuelist->cues->addItem(item);
				}
			}
		);
    }
    else if (button == goBtnUI.get() && currentCuelist)
    {
        currentCuelist->goBtn->trigger();
    }
    else if (button == panicBtnUI.get() && currentCuelist)
    {
        currentCuelist->panic();
    }
    else if (button == mainToggleBT.get() && currentCuelist)
    {
        auto* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
        if (engine == nullptr) return;

        bool isCurrentlyMain = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>() == currentCuelist;
        if (isCurrentlyMain)
        {
            engine->showProperties.mainCuelist->resetValue();
        }
        else
        {
            engine->showProperties.mainCuelist->setValueFromTarget(currentCuelist);
        }
        engine->showProperties.mainCuelist->notifyValueChanged();
    }
}

void DeckViewHeaderUI::mouseDown(const MouseEvent& event)
{
    parent->mouseDown(event);
}
