/*
  ==============================================================================

    CuelistManager.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistManager.h"
#include "CuelistFactory.h"

CuelistManager::CuelistManager() :
    BaseManager<Cuelist>("Cuelists")
{
    selectItemWhenCreated = true;
    managerFactory = CuelistFactory::getInstance();
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

juce_ImplementSingleton(CuelistManager);
