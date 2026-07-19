/*
  ==============================================================================

    RedundancyStateSync.cpp
    Created: 19 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "RedundancyStateSync.h"
#include "RedundancyManager.h"
#include "RedundancyLink.h"
#include "../PonyEngine.h"
#include "../Cuelist/CuelistManager.h"
#include "../Cuelist/Cuelist.h"
#include "../Cue/CueManager.h"
#include "../Cue/Cue.h"

namespace
{
    constexpr int TICK_INTERVAL_MS = 25;  // progress cadence (~40 Hz)
    constexpr int SNAPSHOT_EVERY = 8;     // full snapshot every 8 ticks (~5 Hz)
}

RedundancyStateSync::RedundancyStateSync(RedundancyManager* owner) :
    rm(owner)
{
}

RedundancyStateSync::~RedundancyStateSync()
{
    stop();
}

void RedundancyStateSync::start()
{
    snapshotCounter = 0; // send a full snapshot on the first tick
    startTimer(TICK_INTERVAL_MS);
}

void RedundancyStateSync::stop()
{
    stopTimer();
}

Cue* RedundancyStateSync::resolveCue(const juce::String& address)
{
    if (address.isEmpty() || Engine::mainEngine == nullptr) return nullptr;
    return dynamic_cast<Cue*>(Engine::mainEngine->getControllableContainerForAddress(address, true));
}

// ---------------------------------------------------------------- Main: broadcast

void RedundancyStateSync::timerCallback()
{
    if (rm == nullptr || !rm->isEnabled() || rm->getRole() != RedundancyManager::MAIN) return;
    if (rm->link == nullptr) return;

    // Full snapshot at ~5 Hz (self-healing, resets stopped cues); progress at ~40 Hz (smooth bars).
    if (snapshotCounter <= 0)
    {
        rm->link->sendToPeers(serializeSnapshot());
        snapshotCounter = SNAPSHOT_EVERY;
    }
    --snapshotCounter;

    rm->link->sendToPeers(serializeProgress());
}

juce::String RedundancyStateSync::serializeProgress() const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("k", "p");
    juce::Array<juce::var> act;

    for (Cuelist* cl : CuelistManager::getInstance()->items)
    {
        if (cl == nullptr || cl->cues == nullptr) continue;
        for (Cue* c : cl->cues->items)
        {
            if (c == nullptr) continue;
            if (!c->isPlaying->boolValue() && !c->preWaitActive->boolValue() && !c->postWaitActive->boolValue())
                continue;

            juce::DynamicObject::Ptr a = new juce::DynamicObject();
            a->setProperty("addr", c->getControlAddress());
            a->setProperty("t", c->currentTime->doubleValue());
            a->setProperty("pt", c->preWaitCurrentTime->doubleValue());
            a->setProperty("qt", c->postWaitCurrentTime->doubleValue());
            a->setProperty("d", c->duration->doubleValue());
            act.add(juce::var(a.get()));
        }
    }

    root->setProperty("act", act);
    return juce::JSON::toString(juce::var(root.get()), true);
}

juce::String RedundancyStateSync::serializeSnapshot() const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("k", "s");
    juce::Array<juce::var> cls;

    for (Cuelist* cl : CuelistManager::getInstance()->items)
    {
        if (cl == nullptr) continue;

        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty("addr", cl->getControlAddress());

        if (Cue* c = cl->currentCue->getTargetContainerAs<Cue>()) o->setProperty("cur", c->getControlAddress());
        if (Cue* c = cl->nextCue->getTargetContainerAs<Cue>())    o->setProperty("nxt", c->getControlAddress());

        juce::Array<juce::var> act;
        if (cl->cues != nullptr)
        {
            for (Cue* c : cl->cues->items)
            {
                if (c == nullptr) continue;
                const bool playing = c->isPlaying->boolValue();
                const bool pre = c->preWaitActive->boolValue();
                const bool post = c->postWaitActive->boolValue();
                if (!playing && !pre && !post) continue;

                juce::DynamicObject::Ptr a = new juce::DynamicObject();
                a->setProperty("addr", c->getControlAddress());
                a->setProperty("p", playing);
                a->setProperty("pre", pre);
                a->setProperty("post", post);
                a->setProperty("t", c->currentTime->doubleValue());
                a->setProperty("pt", c->preWaitCurrentTime->doubleValue());
                a->setProperty("qt", c->postWaitCurrentTime->doubleValue());
                a->setProperty("d", c->duration->doubleValue()); // may be overridden by a Stop-on-retrigger fade
                act.add(juce::var(a.get()));
            }
        }
        o->setProperty("act", act);
        cls.add(juce::var(o.get()));
    }

    root->setProperty("cls", cls);

    // The main cuelist selection (which cuelist GO/panic target) is runtime state to mirror too.
    if (auto* pe = dynamic_cast<PonyEngine*>(Engine::mainEngine))
        if (Cuelist* mcl = pe->showProperties.mainCuelist->getTargetContainerAs<Cuelist>())
            root->setProperty("main", mcl->getControlAddress());

    return juce::JSON::toString(juce::var(root.get()), true);
}

// ---------------------------------------------------------------- Backup: apply

void RedundancyStateSync::handleIncoming(const juce::String& payload)
{
    if (rm == nullptr || !rm->isEnabled() || rm->getRole() != RedundancyManager::BACKUP) return;

    // Session protection: with a different show open than the Main, don't plaster its positions onto
    // ours (would point at the wrong cues).
    if (rm->enforceSameShow->boolValue() && rm->showMismatch->boolValue()) return;

    juce::var data = juce::JSON::parse(payload);
    if (!data.isObject()) return;

    if (data.getProperty("k", "s").toString() == "p") applyProgress(data);
    else                                              applyState(data);
}

void RedundancyStateSync::applyProgress(const juce::var& data)
{
    const juce::var act = data.getProperty("act", juce::var());
    if (!act.isArray()) return;

    rm->isApplyingRemote = true;
    for (const juce::var& a : *act.getArray())
    {
        if (Cue* c = resolveCue(a.getProperty("addr", "").toString()))
        {
            c->currentTime->setValue((double)a.getProperty("t", 0.0));
            c->preWaitCurrentTime->setValue((double)a.getProperty("pt", 0.0));
            c->postWaitCurrentTime->setValue((double)a.getProperty("qt", 0.0));
            c->duration->setValue((double)a.getProperty("d", c->duration->doubleValue()));
        }
    }
    rm->isApplyingRemote = false;
}

void RedundancyStateSync::applyState(const juce::var& data)
{
    juce::var cls = data.getProperty("cls", juce::var());
    if (!cls.isArray()) return;

    rm->isApplyingRemote = true;

    // Mirror the main cuelist selection.
    if (auto* pe = dynamic_cast<PonyEngine*>(Engine::mainEngine))
    {
        const juce::String mainAddr = data.getProperty("main", "").toString();
        if (Cuelist* mcl = dynamic_cast<Cuelist*>(Engine::mainEngine->getControllableContainerForAddress(mainAddr, true)))
            pe->showProperties.mainCuelist->setValueFromTarget(mcl);
    }

    for (const juce::var& clv : *cls.getArray())
    {
        const juce::String claddr = clv.getProperty("addr", "").toString();
        Cuelist* cl = dynamic_cast<Cuelist*>(Engine::mainEngine != nullptr
            ? Engine::mainEngine->getControllableContainerForAddress(claddr, true) : nullptr);
        if (cl == nullptr) continue;

        // Current / next cue (setValueFromTarget is non-undoable by default; clear helpers too).
        const juce::String cur = clv.getProperty("cur", "").toString();
        if (Cue* c = resolveCue(cur)) cl->currentCue->setValueFromTarget(c);
        else                          cl->clearCurrentCue();

        const juce::String nxt = clv.getProperty("nxt", "").toString();
        if (Cue* c = resolveCue(nxt)) cl->nextCue->setValueFromTarget(c);
        else                          cl->clearNextCue();

        // Active set: apply flags for listed cues, clear everything else. Never call play() — this
        // only mirrors state for display; audio/interfaces stay silent (Backup is in standby).
        juce::StringArray activeAddrs;
        const juce::var act = clv.getProperty("act", juce::var());
        if (act.isArray())
        {
            for (const juce::var& a : *act.getArray())
            {
                const juce::String addr = a.getProperty("addr", "").toString();
                activeAddrs.add(addr);
                if (Cue* c = resolveCue(addr))
                {
                    c->isPlaying->setValue((bool)a.getProperty("p", false));
                    c->preWaitActive->setValue((bool)a.getProperty("pre", false));
                    c->postWaitActive->setValue((bool)a.getProperty("post", false));
                    c->currentTime->setValue((double)a.getProperty("t", 0.0));
                    c->preWaitCurrentTime->setValue((double)a.getProperty("pt", 0.0));
                    c->postWaitCurrentTime->setValue((double)a.getProperty("qt", 0.0));
                    c->duration->setValue((double)a.getProperty("d", c->duration->doubleValue()));
                }
            }
        }

        if (cl->cues != nullptr)
        {
            for (Cue* c : cl->cues->items)
            {
                if (c == nullptr || activeAddrs.contains(c->getControlAddress())) continue;

                const bool wasActive = c->isPlaying->boolValue() || c->preWaitActive->boolValue()
                                       || c->postWaitActive->boolValue();

                if (c->isPlaying->boolValue())      c->isPlaying->setValue(false);
                if (c->preWaitActive->boolValue())  c->preWaitActive->setValue(false);
                if (c->postWaitActive->boolValue()) c->postWaitActive->setValue(false);
                // A cue that is no longer active must not keep a stale progress on the Backup.
                if (c->currentTime->doubleValue() != 0.0)         c->currentTime->setValue(0.0);
                if (c->preWaitCurrentTime->doubleValue() != 0.0)  c->preWaitCurrentTime->setValue(0.0);
                if (c->postWaitCurrentTime->doubleValue() != 0.0) c->postWaitCurrentTime->setValue(0.0);

                // On going inactive, restore the natural duration (a Stop-on-retrigger fade overrides it).
                if (wasActive) c->refreshDuration();
            }
        }
    }

    rm->isApplyingRemote = false;
}
