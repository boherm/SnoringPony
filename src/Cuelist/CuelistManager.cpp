/*
  ==============================================================================

    CuelistManager.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistManager.h"
#include "../Cue//CueManager.h"
#include "../ui/SPAssetManager.h"
#include "../PonyEngine.h"

CuelistManager::CuelistManager() :
    BaseManager<Cuelist>("Cuelists")
{
    itemDataType = "Cuelist";
    // selectItemWhenCreated = true;
    // saveAndLoadRecursiveData = true;
    // managerFactory = CuelistFactory::getInstance();
}

CuelistManager::~CuelistManager()
{
}

PopupMenu CuelistManager::getItemsMenuWithTickedItem(int startID, Cuelist* currentCuelist)
{
    PopupMenu m;
    int i = 0;
    for (auto item : items) {
        if (item == currentCuelist)
            m.addItem(i + startID, item->niceName, true, true);
        else
            m.addItem(i + startID, item->niceName);
        i++;
    }
    return m;
}

void CuelistManager::showMenuForTargetCue(ControllableContainer* startFromCC, std::function<void(Cue*)> returnFunc)
{
    PopupMenu menu;

    CuelistManager* cm = CuelistManager::getInstance();

    if (startFromCC != nullptr && startFromCC->niceName == "Cues") {
        CueManager* cueManager = dynamic_cast<CueManager*>(startFromCC);
        menu.addSectionHeader(cueManager->parentCuelist->niceName);
        for (int i = 1 ; i <= cueManager->items.size(); ++i)
        {
            Cue* c = dynamic_cast<Cue*>(cueManager->items[i-1]);
            menu.addItem(i, c->niceName + " - " + c->description->stringValue(), true, false, SPAssetManager::getInstance()->getCueIcon(c->getCueType()));
        }

        menu.showMenuAsync(PopupMenu::Options(), [cueManager, returnFunc](int result)
            {
                if (result <= 0) return;
                returnFunc(cueManager->items[result-1]);
            }
        );
    } else {
        for (int i = 1; i <= cm->items.size(); ++i)
        {
            Cuelist* cl = dynamic_cast<Cuelist*>(cm->items[i-1]);
            PopupMenu sub;

            for (int y = 1 ; y <= cl->cues->items.size(); ++y)
            {
                Cue* c = dynamic_cast<Cue*>(cl->cues->items[y-1]);
                sub.addItem((i * 10000) + y, c->niceName + " - " + c->description->stringValue(), true, false, SPAssetManager::getInstance()->getCueIcon(c->getCueType()));
            }

            menu.addSubMenu(cl->niceName, sub);
        }

        menu.showMenuAsync(PopupMenu::Options(), [cm, returnFunc](int result)
            {
                if (result <= 0) return;

                int cueIdx = (result % 10000) - 1;
                int cuelistIdx = ((result - cueIdx) / 10000) - 1;

                Cuelist* cl = dynamic_cast<Cuelist*>(cm->items[cuelistIdx]);
                returnFunc(cl->cues->items[cueIdx]);
            }
        );
    }
}

void CuelistManager::addItemInternal(Cuelist* cl, var data)
{
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    // if not loading file, we set the first cuelist automatically to main cuelist to be controlled
    if (!engine->isLoadingFile) {
        if (items.size() == 1) {
            engine->showProperties.mainCuelist->setTarget(cl);
            engine->showProperties.mainCuelist->notifyValueChanged();
        }
    }
}

void CuelistManager::askForRemoveBaseItem(BaseItem* item)
{
    Cuelist* cl = static_cast<Cuelist*>(item);
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    if (engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>() == cl) {
        engine->showProperties.mainCuelist->resetValue();
        engine->showProperties.mainCuelist->notifyValueChanged();
    }
    BaseManager<Cuelist>::askForRemoveBaseItem(item);
}

bool CuelistManager::haveOnePlaying()
{
    bool result = false;

    for (Cuelist* cl : items) {
        if (cl->isPlaying->boolValue()) {
            result = true;
            break;
        }
    }

    return result;
}

juce_ImplementSingleton(CuelistManager);
