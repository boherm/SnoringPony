/*
  ==============================================================================

    RedundancyLink.cpp
    Created: 19 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "RedundancyLink.h"

namespace
{
    constexpr int BEACON_INTERVAL_MS = 1000;
    constexpr int PEER_TIMEOUT_MS = 4000;
}

RedundancyLink::RedundancyLink() :
    juce::Thread("RedundancyLink UDP"),
    selfId(juce::Uuid().toString())
{
}

RedundancyLink::~RedundancyLink()
{
    stop();
}

void RedundancyLink::start(int p)
{
    stop();
    port = p;

    socket.reset(new juce::DatagramSocket(true));
    socket->setEnablePortReuse(true);
    if (!socket->bindToPort(port))
    {
        NLOGWARNING("Redundancy", "Could not bind UDP port " << port << " for redundancy link.");
        socket.reset();
        return;
    }

    running = true;
    startThread();
    startTimer(BEACON_INTERVAL_MS);
    sendBeacon(); // announce immediately
}

void RedundancyLink::stop()
{
    stopTimer();

    if (socket != nullptr) sendBye(); // tell peers we're leaving so they drop us at once

    running = false;

    signalThreadShouldExit();
    if (socket != nullptr) socket->shutdown(); // unblock the receive loop
    stopThread(2000);
    socket.reset();

    {
        const juce::ScopedLock sl(peersLock);
        peers.clear();
    }
}

juce::Array<RedundancyLink::Peer> RedundancyLink::getPeers() const
{
    const juce::ScopedLock sl(peersLock);
    return peers;
}

int RedundancyLink::getPeerCount() const
{
    const juce::ScopedLock sl(peersLock);
    return peers.size();
}

bool RedundancyLink::hasAnyPeer() const
{
    const juce::ScopedLock sl(peersLock);
    return peers.size() > 0;
}

void RedundancyLink::sendBeacon()
{
    if (socket == nullptr) return;

    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty("t", "b");
    o->setProperty("id", selfId);
    o->setProperty("name", getInstanceName ? getInstanceName() : juce::SystemStats::getComputerName());
    o->setProperty("role", getRole ? getRole() : 0);
    o->setProperty("hash", getShowHash ? getShowHash() : juce::String());
    o->setProperty("gen", getPromotionGen ? getPromotionGen() : 0);
    o->setProperty("prio", getPriority ? getPriority() : 99);

    const juce::String s = juce::JSON::toString(juce::var(o.get()), true);
    socket->write(broadcastAddress, port, s.toRawUTF8(), (int)s.getNumBytesAsUTF8());
}

void RedundancyLink::sendBye()
{
    if (socket == nullptr) return;

    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty("t", "bye");
    o->setProperty("id", selfId);

    const juce::String s = juce::JSON::toString(juce::var(o.get()), true);
    socket->write(broadcastAddress, port, s.toRawUTF8(), (int)s.getNumBytesAsUTF8());
}

void RedundancyLink::sendToPeers(const juce::String& message)
{
    if (socket == nullptr) return;

    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty("t", "m");
    o->setProperty("id", selfId);
    o->setProperty("d", message);

    const juce::String s = juce::JSON::toString(juce::var(o.get()), true);
    socket->write(broadcastAddress, port, s.toRawUTF8(), (int)s.getNumBytesAsUTF8());
}

void RedundancyLink::prunePeers()
{
    const juce::uint32 now = juce::Time::getMillisecondCounter();
    bool removed = false;
    {
        const juce::ScopedLock sl(peersLock);
        for (int i = peers.size() - 1; i >= 0; --i)
        {
            if (now - peers.getReference(i).lastSeenMs > (juce::uint32)PEER_TIMEOUT_MS)
            {
                peers.remove(i);
                removed = true;
            }
        }
    }
    if (removed) notifyPeersChanged();
}

void RedundancyLink::notifyPeersChanged()
{
    if (!onPeersChanged) return;
    auto cb = onPeersChanged;
    juce::MessageManager::callAsync([cb]() { cb(); });
}

void RedundancyLink::timerCallback()
{
    sendBeacon();
    prunePeers();
}

void RedundancyLink::run()
{
    juce::HeapBlock<char> buffer(2048);

    while (!threadShouldExit())
    {
        if (socket == nullptr) break;

        const int ready = socket->waitUntilReady(true, 300);
        if (ready < 0) break;      // socket error / shutdown
        if (ready == 0) continue;  // timeout, re-check exit flag

        juce::String senderIp;
        int senderPort = 0;
        const int n = socket->read(buffer, 2047, false, senderIp, senderPort);
        if (n <= 0) continue;

        const juce::String payload = juce::String::fromUTF8(buffer, n);
        juce::var data = juce::JSON::parse(payload);
        if (!data.isObject()) continue;

        const juce::String id = data.getProperty("id", "").toString();
        if (id.isEmpty() || id == selfId) continue; // ignore malformed and our own beacons

        const juce::String type = data.getProperty("t", "").toString();

        if (type == "b")
        {
            const juce::String name = data.getProperty("name", "").toString();
            const int roleVal = (int)data.getProperty("role", 0);
            const juce::String hash = data.getProperty("hash", "").toString();
            const int gen = (int)data.getProperty("gen", 0);
            const int prio = (int)data.getProperty("prio", 99);

            bool changed = false;
            {
                const juce::ScopedLock sl(peersLock);
                Peer* existing = nullptr;
                for (auto& pr : peers) if (pr.id == id) { existing = &pr; break; }

                if (existing == nullptr)
                {
                    Peer np;
                    np.id = id;
                    peers.add(np);
                    existing = &peers.getReference(peers.size() - 1);
                    changed = true; // new peer
                }
                else if (existing->name != name || existing->role != roleVal
                         || existing->ip != senderIp || existing->hash != hash
                         || existing->promotionGen != gen || existing->priority != prio)
                {
                    changed = true; // an already-known peer changed identity/role/show/generation/priority
                }

                existing->name = name;
                existing->ip = senderIp;
                existing->role = roleVal;
                existing->hash = hash;
                existing->promotionGen = gen;
                existing->priority = prio;
                existing->lastSeenMs = juce::Time::getMillisecondCounter();
            }
            if (changed) notifyPeersChanged();
        }
        else if (type == "bye")
        {
            bool removed = false;
            {
                const juce::ScopedLock sl(peersLock);
                for (int i = peers.size() - 1; i >= 0; --i)
                    if (peers.getReference(i).id == id) { peers.remove(i); removed = true; }
            }
            if (removed) notifyPeersChanged();
        }
        else if (type == "m")
        {
            if (onMessageReceived)
            {
                auto cb = onMessageReceived;
                const juce::String d = data.getProperty("d", "").toString();
                const juce::String fromIp = senderIp;
                juce::MessageManager::callAsync([cb, d, fromIp]() { cb(d, fromIp); });
            }
        }
    }
}
