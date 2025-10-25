/*
  ==============================================================================

    DeckViewHeaderUI.cpp
    Created: 05 Oct 2025 12:56:20am
    Author:  boherm

  ==============================================================================
*/

#include "DeckViewHeaderUI.h"
#include "../../Cue/music/MusicCue.h"

DeckViewHeaderUI::DeckViewHeaderUI(Cuelist* cl) :
    currentCuelist(cl)
{
    currentCuelist->addAsyncContainerListener(this);

    addItemBT.reset(AssetManager::getInstance()->getAddBT());
    addItemBT->setWantsKeyboardFocus(false);
    addItemBT->setTooltip("Add a new Cue");
    addAndMakeVisible(addItemBT.get());
    addItemBT->addListener(this);
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
        currentCuelist->cues.addItem(new MusicCue(), var());
    }
}

void DeckViewHeaderUI::mouseDown(const MouseEvent& event)
{
    parent->mouseDown(event);
}
