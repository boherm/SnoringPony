/*
  ==============================================================================

    AHCQMixerSettings.cpp
    Created: 22 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "AHCQMixerSettings.h"

AHCQMixerSettings::AHCQMixerSettings() : MixerSettings("Mixer Settings")
{
    auto* info = addStringParameter("Info", "", "Partial support: only channel mute/unmute is available.", false);
    info->multiline = true;
    info->setControllableFeedbackOnly(true);

    remoteHost = addStringParameter("Remote Host", "IP address of the CQ console", "");
    remoteHost->autoTrim = true;

    remotePort = addIntParameter("Remote Port", "TCP port for MIDI-over-TCP",
                                 AHCQProtocol::DEFAULT_TCP_PORT, 1, 65535);

    numDCAs = addIntParameter("Num DCAs", "Number of DCAs to expose for mixing cues",
                              AHCQProtocol::MAX_DCAS, 1, AHCQProtocol::MAX_DCAS);

    connectedParam = addBoolParameter("Connected", "Connection status", false);
    connectedParam->setControllableFeedbackOnly(true);
}

AHCQMixerSettings::~AHCQMixerSettings()
{
    if (connectThread != nullptr)
    {
        connectThread->signalThreadShouldExit();
        if (!connectThread->waitForThreadToExit(1000))
            connectThread->stopThread(500);
        connectThread.reset();
    }
    disconnect();
}

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------

void AHCQMixerSettings::connect()
{
    if (connecting.load()) return;

    disconnect();

    if (connectThread != nullptr)
        connectThread->stopThread(5000);

    connectThread = std::make_unique<ConnectThread>(*this);
    connectThread->startThread();
}

void AHCQMixerSettings::ConnectThread::run()
{
    owner.connecting.store(true);

    auto newSocket = std::make_unique<juce::StreamingSocket>();
    bool success = newSocket->connect(owner.remoteHost->stringValue(),
                                       owner.remotePort->intValue(),
                                       3000); // 3 second timeout

    if (threadShouldExit())
    {
        owner.connecting.store(false);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(owner.socketMutex);
        if (success)
            owner.socket = std::move(newSocket);
        owner.connected = success;
    }

    owner.connecting.store(false);

    MessageManager::callAsync([&o = this->owner, success]()
    {
        if (Engine::mainEngine == nullptr || Engine::mainEngine->isClearing)
            return;

        o.connectedParam->setValue(success);

        if (o.onConnectionResult)
            o.onConnectionResult(success);
    });
}

void AHCQMixerSettings::disconnect()
{
    if (connectThread != nullptr)
    {
        connectThread->signalThreadShouldExit();
        connectThread->stopThread(2000);
        connectThread.reset();
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex);
        if (socket != nullptr)
            socket->close();
        socket.reset();
        connected = false;
    }
    connectedParam->setValue(false);
}

bool AHCQMixerSettings::isConnected() const
{
    return connected;
}

bool AHCQMixerSettings::shouldReconnectOnChange(Controllable* c) const
{
    return c == remoteHost || c == remotePort;
}

// ---------------------------------------------------------------------------
// Sending
// ---------------------------------------------------------------------------

void AHCQMixerSettings::sendMidi(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(socketMutex);
    if (!connected || socket == nullptr) return;

    if (logOutgoing)
    {
        juce::String hex;
        for (auto b : data)
            hex += juce::String::toHexString((int) b).paddedLeft('0', 2).toUpperCase() + " ";
        logOutgoing("MIDI: " + hex.trimEnd());
    }

    int written = socket->write(data.data(), (int) data.size());
    if (written < 0)
    {
        // Connection lost
        connected = false;
        socket->close();
        socket.reset();

        MessageManager::callAsync([this]()
        {
            if (Engine::mainEngine == nullptr || Engine::mainEngine->isClearing)
                return;
            connectedParam->setValue(false);
            NLOGWARNING("Allen & Heath CQ", "Connection lost");
        });
    }
}

// ---------------------------------------------------------------------------
// Channel operations (not supported on CQ MIDI protocol)
// ---------------------------------------------------------------------------

void AHCQMixerSettings::sendChannelName(int /*channelNum*/, const juce::String& /*name*/)
{
    // Not available via CQ MIDI protocol — channel names can only be set
    // through the CQ-MixPad app or the mixer's web UI.
}

void AHCQMixerSettings::sendChannelIcon(int /*channelNum*/, int /*icon*/)
{
    // Not available via CQ MIDI protocol.
}

// ---------------------------------------------------------------------------
// DCA Membership
// ---------------------------------------------------------------------------

void AHCQMixerSettings::applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                                            const juce::StringArray& /*dcaNames*/,
                                            const juce::Array<int>& definedChannels,
                                            const std::map<int, juce::String>& /*activeChannelNames*/,
                                            const std::map<int, std::set<int>>& /*channelFXBuses*/,
                                            const juce::Array<int>& /*definedBuses*/,
                                            const juce::Array<bool>& /*dcaHasFX*/,
                                            const std::map<int, float>& /*dcaForcedFaders*/)
{
    if (!connected) return;

    // Partial support: only channel mute/unmute is available via the CQ MIDI protocol.
    // DCA management and FX send routing are not exposed by the protocol.

    // Mute channels not assigned to any DCA, unmute those that are.
    for (int ch : definedChannels)
    {
        bool isActive = false;
        for (int d = 0; d < membership.size(); ++d)
        {
            if (membership[d].contains(ch)) { isActive = true; break; }
        }

        sendMidi(AHCQProtocol::inputMuteMessage(ch, !isActive));
    }
}
