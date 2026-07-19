/*
  ==============================================================================

    RedundancyManager.h
    Created: 19 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include <memory>

class RedundancyLink;
class RedundancyStateSync;

// Owns the Main/Backup redundancy state for THIS instance. It is an EnablingControllableContainer:
// its `enabled` flag turns the whole redundancy system on/off (like a cue's post-wait). It is a
// per-machine settings holder (added as a child of GlobalSettings in PonyEngine, like PluginScanner)
// — NOT saved into the show file. While enabled and acting as a Backup that has not taken over, all
// output interfaces are held silent and local playback triggers are suppressed. Discovery/presence
// runs over a standalone UDP mesh (RedundancyLink): every enabled instance broadcasts a beacon on a
// shared port and maintains a live peer list.
class RedundancyManager :
    public EnablingControllableContainer
{
public:
    juce_DeclareSingleton(RedundancyManager, true);

    RedundancyManager();
    ~RedundancyManager();

    enum Role { MAIN, BACKUP };

    EnumParameter* role = nullptr;
    IntParameter* priority = nullptr;       // failover priority: 1 = highest, 99 = lowest
    StringParameter* instanceName = nullptr;
    IntParameter* syncPort = nullptr;
    BoolParameter* enforceSameShow = nullptr;
    BoolParameter* peerPresent = nullptr;   // read-only: at least one peer is currently reachable
    IntParameter* peerCount = nullptr;      // read-only: number of connected peers
    BoolParameter* mainConflict = nullptr;  // read-only: another Main is present on the network
    BoolParameter* showMismatch = nullptr;  // read-only: the Main has a different show open
    Trigger* takeOverBtn = nullptr;         // manually promote this Backup to Main (last promotion wins)

    // One read-only child per connected peer ("<name>" -> "<role> @ <ip>"), rebuilt on changes.
    ControllableContainer peersCC;

    // Set true when this instance took over the Main role (manual or auto failover). Reset on demotion.
    bool hasTakenOver = false;

    // Promotion generation: bumped above every peer's on each promotion so the most recently promoted
    // Main wins any two-Main conflict ("last promotion wins").
    int promotionGen = 0;

    // True while the Backup is applying remote state received from the Main (so applied flag changes
    // are not mistaken for local actions / re-broadcast).
    bool isApplyingRemote = false;

    // Guards the parameter-change handler while the constructor is still wiring members up (setting
    // `enabled`/params during construction fires onContainerParameterChanged before role/link exist).
    bool redundancyInitialized = false;

    // True once a Main has been seen on the network this session. A Backup only auto-promotes to
    // Main if a Main previously existed (so a Backup started before its Main doesn't promote itself).
    bool hasSeenMain = false;

    // Standalone UDP mesh link. Started when enabled, stopped otherwise.
    std::unique_ptr<RedundancyLink> link;

    // Playback-state replication (Main broadcasts, Backup mirrors). Runs while enabled.
    std::unique_ptr<RedundancyStateSync> stateSync;

    Role getRole() const;
    bool isEnabled() const;
    // Standby = redundancy on, this instance is a Backup that has not taken over: interfaces silent,
    // triggers ignored.
    bool isStandby() const;
    bool isTriggeringSuppressed() const { return isStandby(); }

    // Push the current standby state to all interfaces. Safe to call anytime.
    void applyStandbyToInterfaces();

    // Start/stop the network link to match the enabled state.
    void updateLink();

    // Rebuild the read-only peer list from the link.
    void rebuildPeersList();

    // Enforce a single Main: if this instance is Main and another Main is on the network, the one
    // with the greater id auto-demotes to Backup (deterministic, clock-independent). The other flags
    // the (transient) conflict for display.
    void checkMainConflict();

    // Auto-failover: if this is a Backup, a Main was previously present, and no peers remain (this is
    // the last instance standing), promote it to Main.
    void checkAutoPromote();

    // Session protection: flag when a Backup's open show differs from the Main's (hash mismatch).
    void checkShowMismatch();

    // Manual takeover: promote this Backup to Main now (old Main auto-demotes via the conflict rule).
    void takeOver();

    void onContainerParameterChanged(Parameter* p) override;
    void onContainerTriggerTriggered(Trigger* t) override;

private:
    // Become Main. A manual takeover also bumps the promotion generation above every peer's so it
    // overrides priority; an automatic (priority-driven) promotion leaves the generation as is.
    void promoteToMain(bool manual);
    int maxPeerGen() const;
};
