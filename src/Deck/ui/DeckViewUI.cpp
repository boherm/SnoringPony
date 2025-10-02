/*
  ==============================================================================

    DeckViewUI.cpp
    Created: 27 Sep 2025 05:23:00am
    Author:  boherm

  ==============================================================================
*/

#include "../../MainIncludes.h"
#include "DeckViewUI.h"
#include "../../PonyEngine.h"
#include "CuesTableUI.h"
#include "juce_core/juce_core.h"
#include "../../Cuelist/CuelistManager.h"
#include "CuesTableUI.h"
#include "../../Cue/Cue.h"

DeckViewUI::DeckViewUI(const String &deckName)
{
    this->deckName = deckName;

    TargetParameter* tp = getAssociatedTargetParameter();

    if (tp) {
        tp->addParameterListener(this);
    }

}

DeckViewUI::~DeckViewUI()
{
    TargetParameter* tp = getAssociatedTargetParameter();

    if (tp)
        tp->removeParameterListener(this);
}

void DeckViewUI::setCurrentCuelist(Cuelist* cl)
{
    currentCuelist = cl;

    if (cl != nullptr) {
        if (cuesTable)
            cuesTable.reset();
        if (addItemBT)
            addItemBT.reset();

        cuesTable = std::make_unique<CuesTableUI>(cl);
        addAndMakeVisible(cuesTable.get());

        addItemBT.reset(AssetManager::getInstance()->getAddBT());
        addItemBT->setWantsKeyboardFocus(false);
        addItemBT->setTooltip("Add a new Cue");
        addAndMakeVisible(addItemBT.get());
        addItemBT->addListener(this);
    } else {
        if (cuesTable)
            cuesTable.reset();
        if (addItemBT)
            addItemBT.reset();
    }

    resized();
    repaint();
}

void DeckViewUI::parameterValueChanged(Parameter* parameter)
{
    TargetParameter* tp = dynamic_cast<TargetParameter*>(parameter);
    if (!tp) return;

    Cuelist* cl = tp->getTargetContainerAs<Cuelist>();
    setCurrentCuelist(cl);
}

void DeckViewUI::paint(Graphics& g)
{
    g.fillAll(BG_COLOR);
    if (currentCuelist != nullptr) {
        // g.setColour(BG_COLOR.darker(0.5f));
        g.setColour(currentCuelist->itemColor->getColor());
        g.fillRect(0, 0, getWidth(), 30);
        g.setColour(Colours::white);
        g.setFont (Font (15, Font::bold));
        g.drawText(currentCuelist->niceName, 0, 0, getWidth(), 30, Justification::centred, true);
    } else {
        g.setColour(Colours::white.withAlpha(0.4f));
        g.drawFittedText("You can manage and operate your cuelists in decks.\nPress the right mouse button to assign a cuelist to this desk.", 2, 0, getWidth(), getHeight(), Justification::centred, true);
    }
}

void DeckViewUI::resized()
{
    if (cuesTable)
        cuesTable->setBounds(0, 30, getWidth(), getHeight() - 30);

    if (addItemBT)
        addItemBT->setBounds(getWidth() - 24, 4, 20, 20);
}

TargetParameter* DeckViewUI::getAssociatedTargetParameter()
{
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    DecksSettings* decksSettings = &engine->decksSettings;

    if (deckName == "Deck A")
        return decksSettings->deckA;
    else if (deckName == "Deck B")
        return decksSettings->deckB;
    else if (deckName == "Deck C")
        return decksSettings->deckC;
    else if (deckName == "Deck D")
        return decksSettings->deckD;

    return nullptr;
}

void DeckViewUI::mouseDown(const MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        PopupMenu p;
        p.addSubMenu("Assign a Cuelist...", CuelistManager::getInstance()->getItemsMenuWithTickedItem(100, currentCuelist));

        p.showMenuAsync(PopupMenu::Options(), [this](int result)
            {
                if (result <= 0) return;
                if (result >= 100) {
                    Cuelist* cl = CuelistManager::getInstance()->getItemForMenuResultID(result, 100);

                    if (cl == currentCuelist) {
                        cl = nullptr;
                    }

                    TargetParameter* tp = getAssociatedTargetParameter();

                    if (tp) {
                        if (cl == nullptr) {
                            tp->resetValue();
                        } else {
                            tp->setValueFromTarget(cl);
                        }
                    }
                }
            }
        );
    }
}

void DeckViewUI::buttonClicked(Button* button)
{
    if (button == addItemBT.get())
    {
        if (currentCuelist == nullptr) return;

        Cue* c = new Cue();
        var data = var();
        // data.getDynamicObject()->setProperty("name", "New Cue");
        currentCuelist->cues.addItem(c, data);

        // Logger::writeToLog("DeckViewUI::buttonClicked: addItemBT");
        // Logger::writeToLog("size: " + (String)(currentCuelist->cues.items.size()));
    }
}
