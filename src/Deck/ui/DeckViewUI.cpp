/*
  ==============================================================================

    DeckViewUI.cpp
    Created: 27 Sep 2025 05:23:00am
    Author:  boherm

  ==============================================================================
*/

#include "../../MainIncludes.h"
#include "DeckViewUI.h"
#include "DeckViewHeaderUI.h"
#include "CuesTableUI.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../PonyEngine.h"

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
        if (headerUI)
            headerUI.reset();

        headerUI = std::make_unique<DeckViewHeaderUI>(cl);
        headerUI->setParent(this);
        addAndMakeVisible(headerUI.get());
        cuesTable = std::make_unique<CuesTableUI>(cl);
        addAndMakeVisible(cuesTable.get());
    } else {
        if (cuesTable)
            cuesTable.reset();
        if (headerUI)
            headerUI.reset();
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
    g.setColour(Colours::white.withAlpha(0.4f));
    g.drawFittedText("You can manage and operate your cuelists in decks.\nPress the right mouse button to assign a cuelist to this desk.", 2, 0, getWidth(), getHeight(), Justification::centred, true);
}

void DeckViewUI::resized()
{
    if (cuesTable)
        cuesTable->setBounds(0, 30, getWidth(), getHeight() - 30);

    if (headerUI)
        headerUI->setBounds(0, 0, getWidth(), 30);
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
    } else if (currentCuelist) {
        auto inspect = ShapeShifterManager::getInstance()->getContentForName("Inspector");
        if (inspect == nullptr)
            return;
        InspectorUI *inspectorUI = dynamic_cast<InspectorUI *>(inspect->contentComponent);

        inspectorUI->inspector->setCurrentInspectables(currentCuelist);
        inspectorUI->repaint();
    }
}
