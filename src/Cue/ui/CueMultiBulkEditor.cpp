/*
  ==============================================================================

    CueMultiBulkEditor.cpp
    Created: 11 Sep 2026
    Author:  boherm

  ==============================================================================
*/

#include "CueMultiBulkEditor.h"
#include "../Cue.h"

CueMultiBulkEditor::CueMultiBulkEditor(Inspectable* anchor, const Array<Cue*>& scopeCues,
                                       const String& proxyName, bool renderAsContainer) :
    InspectableEditor(anchor, false),
    renderAsContainer(renderAsContainer)
{
    for (auto* c : scopeCues)
        if (c != nullptr) cues.add(c);

    proxy.reset(new ControllableContainer(proxyName));
}

CueMultiBulkEditor::~CueMultiBulkEditor()
{
    if (proxy != nullptr) proxy->removeAsyncContainerListener(this);
    paramEditors.clear();
    proxyEditor.reset();
    proxy.reset();
}

void CueMultiBulkEditor::buildProxyEditor()
{
    if (renderAsContainer)
    {
        proxyEditor.reset(new GenericControllableContainerEditor(Array<ControllableContainer*>({ proxy.get() }), false));
        addAndMakeVisible(proxyEditor.get());
    }
    else
    {
        for (auto& c : proxy->controllables)
        {
            if (c == nullptr) continue;
            InspectableEditor* ed = c->getEditor(false);
            if (ed == nullptr) continue;
            paramEditors.add(ed);
            addAndMakeVisible(ed);
        }
    }

    // Only listen once the controls are seeded, so seeding never counts as a user edit.
    proxy->addAsyncContainerListener(this);

    setSize(100, 10);
}

void CueMultiBulkEditor::resized()
{
    if (isLayingOut || getWidth() == 0) return;

    isLayingOut = true;

    int y = 0;

    if (proxyEditor != nullptr)
    {
        proxyEditor->setSize(getWidth(), proxyEditor->getHeight());
        proxyEditor->setTopLeftPosition(0, 0);
        y = proxyEditor->getHeight();
    }
    else
    {
        for (auto* ed : paramEditors)
        {
            if (ed == nullptr) continue;
            ed->setSize(getWidth(), ed->getHeight());
            ed->setTopLeftPosition(0, y);
            y += ed->getHeight() + 4;
        }
        if (!paramEditors.isEmpty()) y -= 4;
    }

    isLayingOut = false;

    int h = jmax(y, 10);
    if (getHeight() != h) setSize(getWidth(), h);
}

void CueMultiBulkEditor::childBoundsChanged(juce::Component* c)
{
    if (isLayingOut || getWidth() == 0) return;
    resized();
}

void CueMultiBulkEditor::newMessage(const ContainerAsyncEvent& e)
{
    if (isApplying) return;
    if (e.type != ContainerAsyncEvent::ControllableFeedbackUpdate) return;

    Controllable* c = e.targetControllable;
    if (c == nullptr || e.targetControllable.wasObjectDeleted()) return;

    Parameter* p = dynamic_cast<Parameter*>(c);
    if (p == nullptr || p->parentContainer != proxy.get()) return;

    applyProxyChange(p);
}

void CueMultiBulkEditor::applyToParameters(const Array<Parameter*>& targets, const var& newVal,
                                           const String& undoLabel)
{
    Array<UndoableAction*> actions;

    for (auto* target : targets)
    {
        if (target == nullptr) continue;
        // A parameter driven by a reference (e.g. a volume linked to a volume preset) must
        // keep that link: writing a raw value would silently break it.
        if (target->controlMode != Parameter::ControlMode::MANUAL) continue;
        if (target->getValue() == newVal) continue;

        if (UndoableAction* a = target->setUndoableValue(target->getValue(), newVal, true))
            actions.add(a);
    }

    if (actions.isEmpty()) return;

    isApplying = true;
    UndoMaster::getInstance()->performActions(undoLabel, actions);
    isApplying = false;
}
