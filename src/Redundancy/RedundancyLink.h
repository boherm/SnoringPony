/*
  ==============================================================================

    RedundancyLink.h
    Created: 19 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

// Standalone network link between all SnoringPony instances. It is a symmetric UDP mesh: every
// instance (with a role other than Disabled) periodically broadcasts a small "beacon" (identity +
// role + show hash) on a single shared port, and listens for the others. From those beacons it
// maintains a live list of connected peers (with presence timeouts). No Zeroconf/mDNS, no per-peer
// connection setup — everyone talks on one port. Opaque messages (state sync, later) travel on the
// same socket. This has NO dependency on OBSInterface or the Interface hierarchy.
class RedundancyLink :
    public juce::Thread,   // blocking UDP receive loop
    public juce::Timer     // periodic beacon + presence pruning
{
public:
    RedundancyLink();
    ~RedundancyLink() override;

    struct Peer
    {
        juce::String id;      // per-process unique id (to ignore our own beacons)
        juce::String name;    // display name (defaults to the computer name)
        juce::String ip;      // source IP of the last beacon
        int role = 0;         // RedundancyManager::Role
        juce::String hash;    // open-show hash (session protection)
        int promotionGen = 0; // promotion generation (higher = more recently promoted to Main)
        int priority = 99;    // failover priority (1 = highest, 99 = lowest)
        juce::uint32 lastSeenMs = 0;
    };

    // Owner callbacks (set by RedundancyManager). Invoked on the message thread.
    std::function<juce::String()> getInstanceName; // identity (defaults to computer name)
    std::function<juce::String()> getShowHash;     // open-show hash to advertise
    std::function<int()> getRole;                  // this instance's role (broadcast in the beacon)
    std::function<int()> getPromotionGen;          // promotion generation (broadcast in the beacon)
    std::function<int()> getPriority;              // failover priority (broadcast in the beacon)
    std::function<void()> onPeersChanged;          // peer list / presence changed
    std::function<void(const juce::String& message, const juce::String& fromIp)> onMessageReceived;

    // Start/stop broadcasting+listening on the shared port. Safe to call repeatedly.
    void start(int port);
    void stop();

    bool isRunning() const { return running.load(); }

    // This process' unique id (broadcast in every beacon; used to disambiguate/resolve conflicts).
    juce::String getSelfId() const { return selfId; }

    // Snapshot of the currently-known peers (thread-safe copy).
    juce::Array<Peer> getPeers() const;
    int getPeerCount() const;
    bool hasAnyPeer() const;

    // Broadcast an opaque message to all peers on the shared port.
    void sendToPeers(const juce::String& message);

    // Send a beacon right now (e.g. on promotion) so peers react without waiting for the next tick.
    void announceNow() { sendBeacon(); }

private:
    int port = 44911;
    std::atomic<bool> running { false };
    const juce::String selfId; // unique per process, to discard our own broadcasts

    std::unique_ptr<juce::DatagramSocket> socket; // recreated on each start()
    juce::String broadcastAddress = "255.255.255.255";

    juce::CriticalSection peersLock;
    juce::Array<Peer> peers;

    void sendBeacon();
    void sendBye();       // graceful "leaving" notice so peers drop us immediately
    void prunePeers();
    void notifyPeersChanged();

    void run() override;          // receive loop
    void timerCallback() override; // beacon + prune
};
