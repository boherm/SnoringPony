/*
  ==============================================================================

    GoCue.cpp
    Created: 15 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "GoCue.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cuelist/CuelistManager.h"
#include "../CueManager.h"
#include "../../ui/SPAssetManager.h"

static void showGoTargetMenu(ControllableContainer* /*startFromCC*/,
                             std::function<void(ControllableContainer*)> returnFunc)
{
    juce::PopupMenu menu;
    CuelistManager* cm = CuelistManager::getInstance();

    for (int i = 1; i <= cm->items.size(); ++i)
    {
        Cuelist* cl = cm->items[i - 1];
        juce::PopupMenu sub;

        sub.addItem(i, "GO cuelist (play next cue)");
        sub.addSeparator();

        for (int y = 1; y <= cl->cues->items.size(); ++y)
        {
            Cue* c = cl->cues->items[y - 1];
            sub.addItem((i * 10000) + y,
                        c->niceName + " - " + c->getDescription(),
                        true, false,
                        SPAssetManager::getInstance()->getCueIcon(c->getCueType()));
        }
        menu.addSubMenu(cl->niceName, sub);
    }

    menu.showMenuAsync(juce::PopupMenu::Options(), [cm, returnFunc](int result)
    {
        if (result <= 0) return;
        if (result < 10000)
        {
            returnFunc(cm->items[result - 1]);
            return;
        }
        int cueIdx = (result % 10000) - 1;
        int cuelistIdx = ((result - cueIdx) / 10000) - 1;
        returnFunc(cm->items[cuelistIdx]->cues->items[cueIdx]);
    });
}

GoCue::GoCue(var params) :
    Cue(params)
{
    itemDataType = "Go Cue";
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;

    targetCue = addTargetParameter("Target", "Cue to GO, or cuelist to trigger its next cue",
                                   CuelistManager::getInstance());
    targetCue->targetType = TargetParameter::CONTAINER;
    targetCue->customGetTargetContainerFunc = &showGoTargetMenu;
}

GoCue::~GoCue()
{
}

void GoCue::playInternal()
{
    ControllableContainer* container = targetCue->getTargetContainer();
    if (auto* targetCueItem = dynamic_cast<Cue*>(container))
    {
        if (targetCueItem->parentCuelist != nullptr)
            targetCueItem->parentCuelist->currentCue->setValueFromTarget(targetCueItem);
        targetCueItem->play();
    }
    else if (auto* cl = dynamic_cast<Cuelist*>(container))
    {
        cl->goBtn->trigger();
    }
    endCue();
}

String GoCue::autoDescriptionInternal()
{
    ControllableContainer* container = targetCue->getTargetContainer();
    if (auto* target = dynamic_cast<Cue*>(container))
    {
        if (target == this)
            return "Go Cue (self-reference!)";
        String cuelistName = target->parentCuelist != nullptr ? target->parentCuelist->niceName : String("?");
        return "GO " + cuelistName + " / " + target->id->stringValue() + " - " + target->getDescription();
    }
    if (auto* cl = dynamic_cast<Cuelist*>(container))
    {
        return "GO " + cl->niceName;
    }
    return "Go Cue (no target!)";
}
