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


DeckViewHeaderUI::DeckViewHeaderUI(Cuelist* cl) :
    currentCuelist(cl)
{
    currentCuelist->addAsyncContainerListener(this);

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
    addAndMakeVisible(panicBtnUI.get());
}

DeckViewHeaderUI::~DeckViewHeaderUI()
{
    currentCuelist->removeAsyncContainerListener(this);
}

void DeckViewHeaderUI::paint(Graphics& g)
{
    g.setColour(currentCuelist->itemColor->getColor());
    g.fillRect(0, 0, getWidth(), 30);
    g.setColour(Colours::white);
    g.setFont (Font (15, Font::bold));
    g.drawText(currentCuelist->niceName, 0, 0, getWidth(), 30, Justification::centred, true);
}

void DeckViewHeaderUI::resized()
{
    addItemBT->setBounds(getWidth() - 24, 4, 20, 20);
    goBtnUI->setBounds(4, 4, 60, 22);
    panicBtnUI->setBounds(68, 4, 70, 22);
}

void DeckViewHeaderUI::newMessage(const ContainerAsyncEvent& e)
{
    if (e.targetControllable == currentCuelist->itemColor) {
        repaint();
    }
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
}

void DeckViewHeaderUI::mouseDown(const MouseEvent& event)
{
    parent->mouseDown(event);
}
