/*
  ==============================================================================

    DCACueMultiSync.cpp
    Created: 04 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCACueMultiSync.h"
#include "DCACue.h"
#include "DCAAssignment.h"
#include "CharacterRef.h"
#include "../../Interface/mixer/Character.h"
#include "../../Interface/mixer/MixerFX.h"
#include <set>

DCACueMultiSync::DCACueMultiSync(const juce::Array<Cue*>& dcaCues)
{
    for (auto* c : dcaCues)
        if (c != nullptr) cues.add(c);

    // Editing any selected DCA cue (Inspector or cues table) must propagate to the others.
    // Value changes bubble to the cue; assignment add/remove fire on its dcaAssignments
    // manager; character add/remove fire on each assignment's Characters manager. Listen
    // to all three levels.
    for (auto& w : cues)
        if (DCACue* c = asDca(w))
        {
            c->addAsyncContainerListener(this);
            c->dcaAssignments->addAsyncContainerListener(this);
        }

    refreshCharManagers();
    snapshotAll();
}

DCACueMultiSync::~DCACueMultiSync()
{
    detachAll();
}

void DCACueMultiSync::detachAll()
{
    for (auto& w : cues)
        if (DCACue* c = asDca(w))
        {
            c->removeAsyncContainerListener(this);
            c->dcaAssignments->removeAsyncContainerListener(this);
        }

    for (auto& w : listenedCharManagers)
        if (!w.wasObjectDeleted() && w != nullptr)
            w->removeAsyncContainerListener(this);
    listenedCharManagers.clear();
}

bool DCACueMultiSync::isListeningTo(ControllableContainer* cm) const
{
    for (auto& w : listenedCharManagers)
        if (!w.wasObjectDeleted() && w.get() == cm) return true;
    return false;
}

void DCACueMultiSync::refreshCharManagers()
{
    for (auto& w : cues)
    {
        DCACue* c = asDca(w);
        if (c == nullptr) continue;
        for (auto* a : c->dcaAssignments->items)
        {
            if (a == nullptr) continue;
            ControllableContainer* cm = a->characters.get();
            if (cm != nullptr && !isListeningTo(cm))
            {
                cm->addAsyncContainerListener(this);
                listenedCharManagers.add(cm);
            }
        }
    }
}

DCACue* DCACueMultiSync::asDca(const juce::WeakReference<Inspectable>& w) const
{
    if (w.wasObjectDeleted() || w == nullptr) return nullptr;
    return dynamic_cast<DCACue*>(w.get());
}

DCACueMultiSync::DcaState DCACueMultiSync::stateOf(DCACue* c) const
{
    DcaState s;
    if (c == nullptr) return s;
    for (auto* a : c->dcaAssignments->items)
    {
        if (a == nullptr) continue;
        DcaInfo info;
        info.name = a->displayName->stringValue();
        info.force = a->forceFader->boolValue();
        info.fader = a->faderPosition->floatValue();
        for (auto* r : a->characters->items)
        {
            if (r == nullptr) continue;
            if (Character* ch = r->getCharacter())
                info.chars[ch] = r->getFX();
        }
        s[a->dcaNumber->intValue()] = info;
    }
    return s;
}

void DCACueMultiSync::snapshotAll()
{
    lastState.clear();
    for (auto& w : cues)
        if (DCACue* c = asDca(w))
            lastState[c] = stateOf(c);
}

DCACue* DCACueMultiSync::cueAncestor(ControllableContainer* cc) const
{
    for (ControllableContainer* p = cc; p != nullptr; p = p->parentContainer.get())
        if (auto* d = dynamic_cast<DCACue*>(p)) return d;
    return nullptr;
}

bool DCACueMultiSync::isUnderDcaAssignments(DCACue* owner, ControllableContainer* node) const
{
    if (owner == nullptr || node == nullptr) return false;
    ControllableContainer* mgr = owner->dcaAssignments.get();
    for (ControllableContainer* p = node; p != nullptr; p = p->parentContainer.get())
        if (p == mgr) return true;
    return false;
}

void DCACueMultiSync::newMessage(const ContainerAsyncEvent& e)
{
    if (isSyncing) return;

    // Resolve the container the event relates to. On removals the target is already
    // detached/deleted, so fall back to the dispatching container (e.source), which is the
    // manager the item was removed from (still valid, still under dcaAssignments).
    ControllableContainer* node = nullptr;
    switch (e.type)
    {
    case ContainerAsyncEvent::ControllableFeedbackUpdate:
    case ContainerAsyncEvent::ControllableAdded:
    case ContainerAsyncEvent::ControllableRemoved:
        node = (e.targetControllable != nullptr && !e.targetControllable.wasObjectDeleted())
             ? e.targetControllable->parentContainer.get() : e.source.get();
        break;

    case ContainerAsyncEvent::ControllableContainerAdded:
    case ContainerAsyncEvent::ControllableContainerRemoved:
        node = (e.targetContainer != nullptr && !e.targetContainer.wasObjectDeleted())
             ? e.targetContainer.get() : e.source.get();
        break;

    default:
        return;
    }

    DCACue* source = cueAncestor(node);
    if (source == nullptr) return;
    if (!isUnderDcaAssignments(source, node)) return;

    // New assignments may have appeared: make sure their Characters managers are listened to.
    refreshCharManagers();

    // Ignore echoes from our own sync writes (state unchanged since last snapshot).
    if (stateOf(source) == lastState[source]) return;

    propagateFrom(source);
}

void DCACueMultiSync::propagateFrom(DCACue* source)
{
    if (source == nullptr) return;

    const DcaState prev = lastState[source];
    const DcaState cur = stateOf(source);

    // Every DCA number whose snapshot changed (per-character or property level).
    std::set<int> changed;
    for (auto& kv : cur)
        if (prev.find(kv.first) == prev.end() || prev.at(kv.first) != kv.second)
            changed.insert(kv.first);
    for (auto& kv : prev)
        if (cur.find(kv.first) == cur.end()) changed.insert(kv.first);

    if (changed.empty()) return;

    static const DcaInfo empty;

    isSyncing = true;
    for (auto& w : cues)
    {
        DCACue* peer = asDca(w);
        if (peer == nullptr || peer == source) continue;

        for (int k : changed)
        {
            const bool inCur = cur.find(k) != cur.end();
            const DcaInfo& p = prev.count(k) ? prev.at(k) : empty;
            const DcaInfo& c = inCur ? cur.at(k) : empty;
            applyDcaDelta(peer, k, p, c, inCur);
        }

        peer->refreshCharacterRefRoots();
        peer->checkBrokenRefs();
    }
    isSyncing = false;

    // Peers may have gained assignments: listen to their new Characters managers.
    refreshCharManagers();

    // Record the post-sync state of every cue so the resulting async echoes are ignored.
    snapshotAll();
}

void DCACueMultiSync::applyDcaDelta(DCACue* peer, int dcaNumber, const DcaInfo& prev, const DcaInfo& cur, bool inCur)
{
    DCAAssignment* pa = peer->findAssignment(dcaNumber);

    // Characters removed on the source: drop them from this peer's DCA (if present).
    for (auto& kv : prev.chars)
        if (cur.chars.find(kv.first) == cur.chars.end())
            if (pa != nullptr && pa->hasCharacter(kv.first))
                pa->removeCharacter(kv.first);

    // Characters added on the source: add them to this peer's DCA (keeping its own others).
    for (auto& kv : cur.chars)
    {
        Character* ch = kv.first;
        MixerFX* fx = kv.second;
        const bool wasThere = prev.chars.find(ch) != prev.chars.end();

        if (!wasThere)
        {
            if (pa == nullptr) pa = peer->createAssignment(dcaNumber);
            if (pa != nullptr && !pa->hasCharacter(ch))
            {
                pa->addCharacter(ch);
                setCharacterFx(pa, ch, fx);
            }
        }
        else if (prev.chars.at(ch) != fx) // FX changed for a character present in both
        {
            if (pa != nullptr && pa->hasCharacter(ch)) setCharacterFx(pa, ch, fx);
        }
    }

    // Assignment-level properties: mirror only the ones that actually changed on the source
    // (so e.g. removing a character doesn't overwrite each peer's own name/fader).
    if (inCur && pa != nullptr)
    {
        if (cur.name != prev.name && pa->displayName->stringValue() != cur.name) pa->displayName->setValue(cur.name);
        if (cur.force != prev.force && pa->forceFader->boolValue() != cur.force) pa->forceFader->setValue(cur.force);
        if (cur.fader != prev.fader && (float)pa->faderPosition->floatValue() != cur.fader) pa->faderPosition->setValue(cur.fader);
    }

    // A DCA emptied by the delta is dropped (an empty DCA means "no DCA").
    if (pa != nullptr && pa->characters->items.isEmpty())
        peer->dcaAssignments->removeItem(pa);
}

void DCACueMultiSync::setCharacterFx(DCAAssignment* pa, Character* ch, MixerFX* fx)
{
    for (auto* r : pa->characters->items)
    {
        if (r != nullptr && r->getCharacter() == ch)
        {
            if (fx != nullptr) r->fxRef->setValueFromTarget(fx);
            else r->fxRef->resetValue();
            return;
        }
    }
}
