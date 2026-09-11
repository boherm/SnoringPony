/*
  ==============================================================================

    CueUnsetValueEditor.cpp
    Created: 11 Sep 2026
    Author:  boherm

  ==============================================================================
*/

#include "CueUnsetValueEditor.h"
#include "../Cue.h"

bool CueUnsetValueEditor::canSubstitute(Controllable* c)
{
    return dynamic_cast<FloatParameter*>(c) != nullptr;
}

CueUnsetValueEditor::CueUnsetValueEditor(Parameter* anchorParam, const Array<Cue*>& scopeCues) :
    CueMultiBulkEditor(anchorParam, scopeCues, "Unset Proxy", false),
    targetName(anchorParam != nullptr ? anchorParam->shortName : String())
{
    if (auto* f = dynamic_cast<FloatParameter*>(anchorParam))
    {
        // Same look as the parameter it replaces, but parked at the minimum: as long as it
        // is not moved, each cue keeps its own value.
        FloatParameter* p = proxy->addFloatParameter(f->niceName,
            f->description + " (applied to every selected cue as soon as you move it; left at minimum, each cue keeps its own value)",
            (double)f->minimumValue, (double)f->minimumValue, (double)f->maximumValue);
        p->defaultUI = f->defaultUI;
        p->customUI = f->customUI;
        proxyParam = p;
    }

    buildProxyEditor();
}

CueUnsetValueEditor::~CueUnsetValueEditor()
{
}

void CueUnsetValueEditor::applyProxyChange(Parameter* changedProxyParam)
{
    if (changedProxyParam != proxyParam || targetName.isEmpty()) return;

    Array<Parameter*> targets;
    for (auto& w : cues)
    {
        auto* cue = dynamic_cast<Cue*>(w.get());
        if (cue == nullptr) continue;

        for (auto& c : cue->controllables)
            if (c != nullptr && c->shortName == targetName)
                if (auto* p = dynamic_cast<Parameter*>(c)) targets.add(p);
    }

    applyToParameters(targets, changedProxyParam->getValue(),
                      "Edit " + proxyParam->niceName.toLowerCase() + " of " + String(cues.size()) + " cues");
}
