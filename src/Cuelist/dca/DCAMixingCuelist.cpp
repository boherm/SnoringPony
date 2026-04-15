/*
  ==============================================================================

    DCAMixingCuelist.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCAMixingCuelist.h"
#include "../../Cue/CueManager.h"
#include "../../Cue/dca/DCACue.h"
#include "../../ui/SPAssetManager.h"
#include "ui/DCAMixingCuelistEditor.h"

DCAMixingCuelist::DCAMixingCuelist(var params) :
    Cuelist(params)
{
    // Re-bind the cue factory now that our vtable is fully set up,
    // so the virtual registerCueTypes override is used.
    if (cues != nullptr) cues->bindFactoryFromCuelist();

    if (!Engine::mainEngine->isLoadingFile) ensureLineCheckCue();
}

DCAMixingCuelist::~DCAMixingCuelist()
{
}

void DCAMixingCuelist::registerCueTypes(Factory<Cue>& f)
{
    f.defs.add(Factory<Cue>::Definition::createDef("Mixer", "DCA Cue", &DCACue::create)
        ->addIcon(SPAssetManager::getInstance()->getCueIcon("DCA")));
}

InspectableEditor* DCAMixingCuelist::getEditorInternal(bool isRoot, Array<Inspectable*> /*inspectables*/)
{
    return new DCAMixingCuelistEditor(this, isRoot);
}

void DCAMixingCuelist::loadJSONDataInternal(var data)
{
    Cuelist::loadJSONDataInternal(data);
    ensureLineCheckCue();
}

void DCAMixingCuelist::ensureLineCheckCue()
{
    Cue* existing = nullptr;
    for (Cue* c : cues->items)
    {
        if (c->id->floatValue() == 0.0f) { existing = c; break; }
    }

    if (existing != nullptr)
    {
        existing->userCanRemove = false;
        existing->canBeReorderedInEditor = false;
        if (cues->items.indexOf(existing) != 0)
            cues->setItemIndex(existing, 0, false);
        return;
    }

    DCACue* lineCheck = new DCACue();
    lineCheck->id->setValue(0);
    lineCheck->description->setValue("Line check");
    lineCheck->userCanRemove = false;
    lineCheck->canBeReorderedInEditor = false;
    var data(new DynamicObject());
    data.getDynamicObject()->setProperty("index", 0);
    cues->addItem(lineCheck, data, false);
}
