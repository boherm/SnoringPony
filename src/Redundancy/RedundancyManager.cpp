/*
  ==============================================================================

    RedundancyManager.cpp
    Created: 19 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "RedundancyManager.h"
#include "RedundancyLink.h"
#include "RedundancyStateSync.h"
#include "../PonyEngine.h"
#include "../Interface/InterfaceManager.h"

juce_ImplementSingleton(RedundancyManager)

RedundancyManager::RedundancyManager() :
    EnablingControllableContainer("Redundancy"),
    peersCC("Connected peers")
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;

    // Redundancy always starts OFF on launch (never restored), but the config below IS saved.
    enabled->setValue(false);
    enabled->isSavable = false;

    role = addEnumParameter("Role", "Redundancy role of this instance. Main is the playback authority; Backup mirrors the Main and stays silent until it takes over.");
    role->addOption("Main", MAIN)->addOption("Backup", BACKUP);
    role->defaultValue = "Main";

    priority = addIntParameter("Priority", "Failover priority: 1 is highest, 99 is lowest. When the Main is lost, the highest-priority Backup auto-promotes; on any two-Main conflict, the higher priority wins.", 1, 1, 99);

    instanceName = addStringParameter("Instance name", "Name used to identify this instance on the network (defaults to the computer name).", SystemStats::getComputerName());

    syncPort = addIntParameter("Sync port", "Shared UDP port all instances use to discover each other and exchange state.", 44911, 1024, 65535);

    enforceSameShow = addBoolParameter("Enforce same show", "When enabled, the Backup refuses to mirror state if the open show differs from the Main's (hash mismatch).", true);

    peerPresent = addBoolParameter("Peer present", "Whether at least one peer instance is currently reachable on the network.", false);
    peerPresent->isSavable = false;
    peerPresent->setControllableFeedbackOnly(true);
    peerPresent->hideInEditor = true;

    peerCount = addIntParameter("Peer count", "Number of peer instances currently connected.", 0, 0, 128);
    peerCount->isSavable = false;
    peerCount->setControllableFeedbackOnly(true);
    peerCount->hideInEditor = true;

    mainConflict = addBoolParameter("Main conflict", "True when another Main instance is present on the network (should never persist — one of them auto-demotes to Backup).", false);
    mainConflict->isSavable = false;
    mainConflict->setControllableFeedbackOnly(true);
    mainConflict->hideInEditor = true;

    showMismatch = addBoolParameter("Show mismatch", "True when this Backup has a different show open than the Main (hash mismatch).", false);
    showMismatch->isSavable = false;
    showMismatch->setControllableFeedbackOnly(true);
    showMismatch->hideInEditor = true;

    takeOverBtn = addTrigger("Take over", "Promote this Backup to Main now. Any current Main auto-demotes to Backup.");
    takeOverBtn->isSavable = false;

    peersCC.saveAndLoadRecursiveData = false;
    peersCC.hideInRemoteControl = true;
    addChildControllableContainer(&peersCC);

    // Standalone UDP mesh link. Callbacks bridge network events back onto the message thread.
    link.reset(new RedundancyLink());
    link->getInstanceName = [this]() { return instanceName->stringValue(); };
    link->getRole = [this]() { return (int)getRole(); };
    link->getPromotionGen = [this]() { return promotionGen; };
    link->getPriority = [this]() { return priority->intValue(); };
    link->getShowHash = []()
    {
        if (auto* pe = dynamic_cast<PonyEngine*>(Engine::mainEngine))
            return juce::String(pe->getCurrentShowHash());
        return juce::String();
    };
    link->onPeersChanged = [this]()
    {
        if (link == nullptr) return;
        peerPresent->setValue(link->hasAnyPeer());
        peerCount->setValue(link->getPeerCount());
        rebuildPeersList();

        // Remember if a Main is (or was) present — required before a Backup may auto-promote.
        for (const auto& p : link->getPeers()) if (p.role == MAIN) { hasSeenMain = true; break; }

        checkMainConflict();
        checkAutoPromote();
        checkShowMismatch();
    };

    // Playback-state replication over the same link.
    stateSync.reset(new RedundancyStateSync(this));
    link->onMessageReceived = [this](const juce::String& message, const juce::String&)
    {
        if (stateSync != nullptr) stateSync->handleIncoming(message);
    };

    // All members are wired: the parameter-change handler may now run its redundancy logic.
    redundancyInitialized = true;
}

RedundancyManager::~RedundancyManager()
{
    if (stateSync != nullptr) stateSync->stop();
    stateSync.reset();
    if (link != nullptr) link->stop();
    link.reset();
}

RedundancyManager::Role RedundancyManager::getRole() const
{
    return role->getValueDataAsEnum<Role>();
}

bool RedundancyManager::isEnabled() const
{
    return enabled->boolValue();
}

bool RedundancyManager::isStandby() const
{
    return isEnabled() && getRole() == BACKUP && !hasTakenOver;
}

void RedundancyManager::applyStandbyToInterfaces()
{
    if (auto* im = InterfaceManager::getInstanceWithoutCreating())
        im->setRedundancyStandby(isStandby());
}

void RedundancyManager::updateLink()
{
    if (link == nullptr) return;
    if (isEnabled())
    {
        link->start(syncPort->intValue());
        if (stateSync != nullptr) stateSync->start();
    }
    else
    {
        link->stop();
        if (stateSync != nullptr) stateSync->stop();
    }
}

void RedundancyManager::rebuildPeersList()
{
    // Remove existing entries via removeControllable (which notifies editors — unlike clear()),
    // so the on-screen list updates live when a peer joins, leaves or changes name/role.
    juce::Array<juce::WeakReference<Controllable>> toRemove;
    for (auto* c : peersCC.controllables) toRemove.add(c);
    for (auto& c : toRemove) peersCC.removeControllable(c);

    if (link == nullptr) return;

    for (const auto& p : link->getPeers())
    {
        const juce::String roleStr = (p.role == MAIN) ? "Main" : "Backup";
        const juce::String label = p.name.isNotEmpty() ? p.name : (p.ip.isNotEmpty() ? p.ip : p.id);

        StringParameter* sp = peersCC.addStringParameter(label, "Connected peer", roleStr + " @ " + p.ip);
        sp->isSavable = false;
        sp->setControllableFeedbackOnly(true);
    }
}

void RedundancyManager::checkMainConflict()
{
    if (!isEnabled() || getRole() != MAIN || link == nullptr)
    {
        mainConflict->setValue(false);
        return;
    }

    const juce::String myId = link->getSelfId();
    const int myPrio = priority->intValue();
    bool conflict = false;
    bool iAmLoser = false;
    juce::String otherName;

    for (const auto& p : link->getPeers())
    {
        if (p.role != MAIN) continue;
        conflict = true;
        otherName = p.name;
        // Winner precedence: higher promotion generation (manual takeover) first, then better
        // priority (lower number), then lower id. The loser auto-demotes to Backup.
        const bool peerWins = (p.promotionGen > promotionGen)
            || (p.promotionGen == promotionGen && p.priority < myPrio)
            || (p.promotionGen == promotionGen && p.priority == myPrio && p.id < myId);
        if (peerWins) iAmLoser = true;
    }

    if (conflict && iAmLoser)
    {
        NLOGWARNING(niceName, "Another Main outranks this one (" << otherName << "); auto-demoting to Backup.");
        mainConflict->setValue(false);
        hasTakenOver = false;
        role->setValueWithData(BACKUP); // triggers standby via onContainerParameterChanged
        return;
    }

    mainConflict->setValue(conflict); // winner (or none): flag the transient conflict for display
}

int RedundancyManager::maxPeerGen() const
{
    int m = 0;
    if (link != nullptr)
        for (const auto& p : link->getPeers()) m = juce::jmax(m, p.promotionGen);
    return m;
}

void RedundancyManager::promoteToMain(bool manual)
{
    // A manual takeover outranks priority (bump generation above every peer); a priority-driven
    // auto-promotion leaves the generation as is, so a higher-priority Main can still reclaim.
    if (manual) promotionGen = maxPeerGen() + 1;
    hasTakenOver = true;
    role->setValueWithData(MAIN);    // onContainerParameterChanged keeps hasTakenOver (role==MAIN)
    if (link != nullptr) link->announceNow(); // let the old Main demote at once (no ~1s double-drive)
}

void RedundancyManager::takeOver()
{
    if (!isEnabled() || getRole() != BACKUP) return;
    NLOGWARNING(niceName, "Manual takeover: promoting this Backup to Main.");
    promoteToMain(true);
}

void RedundancyManager::checkAutoPromote()
{
    if (!isEnabled() || getRole() != BACKUP || link == nullptr) return;
    if (!hasSeenMain) return; // never promote a Backup that never had a Main

    // Only when no Main is present anymore.
    for (const auto& p : link->getPeers())
        if (p.role == MAIN) return;

    // Promote only if this instance is the best remaining candidate by priority (1 = highest),
    // ties broken by the lower id — so exactly one Backup (the highest priority) takes over.
    const int myPrio = priority->intValue();
    const juce::String myId = link->getSelfId();
    for (const auto& p : link->getPeers())
    {
        if (p.role != BACKUP) continue;
        if (p.priority < myPrio) return;                 // a higher-priority Backup exists
        if (p.priority == myPrio && p.id < myId) return; // tie -> lower id wins
    }

    NLOGWARNING(niceName, "Main lost; auto-promoting this Backup to Main (highest priority).");
    promoteToMain(false);
}

void RedundancyManager::checkShowMismatch()
{
    if (!isEnabled() || getRole() != BACKUP || link == nullptr)
    {
        showMismatch->setValue(false);
        return;
    }

    juce::String myHash;
    if (auto* pe = dynamic_cast<PonyEngine*>(Engine::mainEngine))
        myHash = juce::String(pe->getCurrentShowHash());

    bool mismatch = false;
    for (const auto& p : link->getPeers())
    {
        if (p.role != MAIN) continue;
        // Both hashes known and different -> the Main opened a different show.
        if (p.hash.isNotEmpty() && myHash.isNotEmpty() && p.hash != myHash) mismatch = true;
    }
    showMismatch->setValue(mismatch);
}

void RedundancyManager::onContainerParameterChanged(Parameter* p)
{
    EnablingControllableContainer::onContainerParameterChanged(p);

    // Params set during construction fire this before role/link/mainConflict exist.
    if (!redundancyInitialized) return;

    if (p == enabled)
    {
        hasTakenOver = false;
        hasSeenMain = false; // fresh session each time redundancy is (re)enabled
        applyStandbyToInterfaces();
        updateLink();
        checkMainConflict();
        checkShowMismatch();
    }
    else if (p == role)
    {
        // Role change re-evaluates standby (the beacon picks up the new role on its next tick).
        // Only a demotion back to Backup clears the taken-over flag (a promotion sets it beforehand).
        if (getRole() == BACKUP) hasTakenOver = false;
        applyStandbyToInterfaces();
        checkMainConflict();
        checkShowMismatch();
    }
    else if (p == syncPort)
    {
        updateLink();
    }
}

void RedundancyManager::onContainerTriggerTriggered(Trigger* t)
{
    if (t == takeOverBtn) takeOver();
}
