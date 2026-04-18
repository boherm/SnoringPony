/*
  ==============================================================================

    WingTcpMixerSettings.cpp
    Created: 17 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "WingTcpMixerSettings.h"
#include <vector>
#include <cmath>
#include <cstring>

WingTcpMixerSettings::WingTcpMixerSettings() : MixerSettings("Mixer Settings")
{
    remoteHost = addStringParameter("Remote Host", "IP address of the WING console", "");
    remoteHost->autoTrim = true;

    remotePort = addIntParameter("Remote Port", "TCP port (wapi)", DEFAULT_TCP_PORT, 1, 65535);

    numDCAs = addIntParameter("Num DCAs", "Number of DCAs to expose for mixing cues", 8, 1, MAX_DCAS);

    monitoringEnabled = addBoolParameter("Silence Monitoring",
        "When enabled, declared channels that stop receiving signal blink red. "
        "Disable to keep channels at their base DCA colors only.", true);

    silenceTimeout = addFloatParameter("Silence Timeout",
        "Number of seconds a channel must stay silent before it starts blinking red.",
        DEFAULT_SILENCE_TIMEOUT_SEC, 0.5f, 30.0f);
    silenceTimeout->defaultUI = FloatParameter::TIME;

    silentThresholdDb = addFloatParameter("Silence Threshold (dBFS)",
        "Level below which a channel is considered silent. Raise if silent channels don't blink "
        "(noise floor too high). Lower if channels with signal still blink.",
        DEFAULT_SILENT_THRESHOLD_DB, -80.0f, 10.0f);

    clipThresholdDb = addFloatParameter("Clip Threshold (dBFS)",
        "Level above which a channel is considered clipping. 0 dBFS is digital clip. "
        "When a clip is detected, the channel is held solid red for 'Clip Hold Time' seconds.",
        DEFAULT_CLIP_THRESHOLD_DB, -80.0f, 10.0f);

    clipHoldTime = addFloatParameter("Clip Hold Time",
        "Number of seconds a channel stays solid red after a clip is detected before "
        "returning to its normal state.",
        DEFAULT_CLIP_HOLD_SEC, 0.5f, 30.0f);
    clipHoldTime->defaultUI = FloatParameter::TIME;

    reconnectBtn = addTrigger("Reconnect", "Reconnect to the WING console");

    connectedParam = addBoolParameter("Connected", "Connection status", false);
    connectedParam->setControllableFeedbackOnly(true);
}

WingTcpMixerSettings::~WingTcpMixerSettings()
{
    disconnect();
    if (connectThread != nullptr)
        connectThread->stopThread(5000);
}

void WingTcpMixerSettings::connect()
{
    if (connecting.load())
        return;

    disconnectInternal();
    autoReconnect = true;

    if (connectThread != nullptr)
        connectThread->stopThread(5000);

    connectThread = std::make_unique<ConnectThread>(*this);
    connectThread->startThread();
}

void WingTcpMixerSettings::ConnectThread::run()
{
    owner.connecting.store(true);

    char ip[64];
    owner.remoteHost->stringValue().copyToUTF8(ip, sizeof(ip));

    bool success = (wOpen(ip) == WSUCCESS);

    if (threadShouldExit())
    {
        if (success) wClose();
        owner.connecting.store(false);
        return;
    }

    owner.connected = success;
    owner.connecting.store(false);

    MessageManager::callAsync([&owner = this->owner, success]()
    {
        owner.connectedParam->setValue(success);

        if (success)
        {
            owner.setupMeterSubscription();
            owner.startTimer(TICK_INTERVAL_MS);
        }
        else
        {
            owner.startTimer(RECONNECT_INTERVAL_MS);
        }

        if (owner.onConnectionResult)
            owner.onConnectionResult(success);
    });
}

void WingTcpMixerSettings::disconnect()
{
    autoReconnect = false;
    if (connectThread != nullptr)
        connectThread->stopThread(5000);
    disconnectInternal();
}

void WingTcpMixerSettings::disconnectInternal()
{
    stopTimer();
    teardownMeterSubscription();
    if (connected)
    {
        wClose();
        connected = false;
    }
    connectedParam->setValue(false);

    std::lock_guard<std::mutex> lock(stateMutex);
    lastSignalMs.clear();
    baseColors.clear();
    clipUntilMs.clear();
    blinkRedPhase = false;
    tickCount = 0;
}

void WingTcpMixerSettings::timerCallback()
{
    if (!connected)
    {
        if (!autoReconnect || connecting.load()) return;

        if (logOutgoing) logOutgoing("attempting reconnect...");

        if (connectThread != nullptr)
            connectThread->stopThread(5000);

        connectThread = std::make_unique<ConnectThread>(*this);
        connectThread->startThread();
        return;
    }

    tickCount++;

    // Check silence / update blinking (every tick = 250ms)
    updateBlinking();

    // Renew meter subscription every ~4s (Wing times out after 5s)
    if (tickCount % METER_RENEW_EVERY_N_TICKS == 0)
    {
        if (wRenewMeters(METER_REQ_ID) != WSUCCESS)
        {
            // Non-fatal, just log
            if (logOutgoing) logOutgoing("wRenewMeters failed");
        }
    }

    // Keepalive every ~5s (Wing closes TCP after 10s idle)
    if (tickCount % KEEPALIVE_EVERY_N_TICKS == 0)
    {
        if (wKeepAlive() != WSUCCESS) { handleWapiError(); return; }
    }
}

void WingTcpMixerSettings::onContainerTriggerTriggered(Trigger* t)
{
    if (t == reconnectBtn)
        connect();
}

void WingTcpMixerSettings::onContainerParameterChanged(Parameter* p)
{
    if (p == monitoringEnabled)
    {
        const bool enabled = monitoringEnabled->boolValue();
        silenceTimeout->setEnabled(enabled);
        silentThresholdDb->setEnabled(enabled);
        clipThresholdDb->setEnabled(enabled);
        clipHoldTime->setEnabled(enabled);

        // If monitoring was just turned off, clear clip-hold state and restore all channels
        // to their base colors so no channel stays stuck on red.
        if (!enabled)
        {
            std::map<int, int> restore;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                clipUntilMs.clear();
                if (connected) restore = baseColors;
            }
            for (auto& kv : restore) sendChannelColor(kv.first, kv.second);
        }
    }
}

void WingTcpMixerSettings::handleWapiError()
{
    if (logOutgoing) logOutgoing("wapi error — disconnecting, will auto-reconnect");

    teardownMeterSubscription();
    if (connected)
    {
        wClose();
        connected = false;
    }
    connectedParam->setValue(false);
    startTimer(RECONNECT_INTERVAL_MS);
}

bool WingTcpMixerSettings::isConnected() const
{
    return connected;
}

bool WingTcpMixerSettings::shouldReconnectOnChange(Controllable* c) const
{
    return c == remoteHost || c == remotePort;
}

// ---------------------------------------------------------------------------
// Meter subscription & thread
// ---------------------------------------------------------------------------

void WingTcpMixerSettings::setupMeterSubscription()
{
    int r1 = wMeterUDPPort(METER_LOCAL_UDP_PORT);
    if (logOutgoing) logOutgoing("wMeterUDPPort(" + juce::String(METER_LOCAL_UDP_PORT) + ") = " + juce::String(r1));

    // 30-byte meter subscription bitfield. Layout (from external/wapi/wmeters.c):
    //   byte 0-4  : V1 channels 1-40 (8 channels per byte, MSB = lowest-numbered)
    //   byte 5    : V1 aux 1-8
    //   byte 6-7  : V1 bus 1-16
    //   ... (we ignore all other groups)
    // Setting bytes 0-4 to 0xFF subscribes to all 40 V1 channels.
    unsigned char wMid[30] = { 0 };
    wMid[0] = 0xFF;
    wMid[1] = 0xFF;
    wMid[2] = 0xFF;
    wMid[3] = 0xFF;
    wMid[4] = 0xFF;

    int r2 = wSetMetersRequest(METER_REQ_ID, wMid);
    if (logOutgoing) logOutgoing("wSetMetersRequest() = " + juce::String(r2));

    if (r2 < 0)
    {
        if (logOutgoing) logOutgoing("wSetMetersRequest failed, meter thread not started");
        return;
    }

    meterThread.reset(new MeterThread(*this));
    meterThread->startThread();
    if (logOutgoing) logOutgoing("meter thread started");
}

void WingTcpMixerSettings::teardownMeterSubscription()
{
    if (meterThread != nullptr)
    {
        meterThread->stopThread(500);
        meterThread.reset();
    }
}

void WingTcpMixerSettings::MeterThread::run()
{
    std::vector<unsigned char> buf(4096);
    while (!threadShouldExit())
    {
        int received = wGetMeters(buf.data(), (int) buf.size(), 100000); // 100ms timeout
        if (received > 0) owner.processMeterPacket(buf.data(), received);
    }
}

void WingTcpMixerSettings::processMeterPacket(const unsigned char* data, int size)
{
    // Packet format (from external/wapi/wmeters.c):
    //   bytes 0..3    : 4-byte header (skipped)
    //   per channel   : 16 bytes = 8 int16 big-endian values:
    //                   [IN L, IN R, OUT L, OUT R, gate key, gate gain, dyn key, dyn gain]
    //   For 40 channels: packet size = 4 + 40*16 = 644 bytes.
    //
    // Each int16 is scaled: value = (int16 + 32768) / 256 → range 0..256,
    // where 0 = fully silent and ≈110+ = audible signal.
    // We only read IN L + IN R (first 4 bytes of each channel chunk) to detect
    // input signal activity. Gate/dyn fields default to 0x0000 (= v=128, no reduction).
    if (size < 4 + 16) return;

    const int headerBytes = 4;
    const int bytesPerChannel = 16;
    const int dataBytes = size - headerBytes;
    const int maxChannel = juce::jmin(dataBytes / bytesPerChannel, 40);

    const double now = juce::Time::getMillisecondCounterHiRes();
    std::lock_guard<std::mutex> lock(stateMutex);

    auto readBE16 = [](const unsigned char* p) -> int {
        juce::int16 raw = (juce::int16)(((juce::uint16)p[0] << 8) | (juce::uint16)p[1]);
        return (int) raw;
    };

    const int clipT = dbToMeterValue(clipThresholdDb->floatValue());
    const double clipHoldMs = clipHoldTime->floatValue() * 1000.0;
    const int silentT = dbToMeterValue(silentThresholdDb->floatValue());

    for (int ch = 1; ch <= maxChannel; ++ch)
    {
        // Only track channels that are declared in MixerInterface.
        if (baseColors.find(ch) == baseColors.end()) continue;

        const unsigned char* p = data + headerBytes + (ch - 1) * bytesPerChannel;
        // IN L + IN R only (first pair of 4 bytes).
        const int raw_l = readBE16(p);
        const int raw_r = readBE16(p + 2);

        const int val_l = (raw_l + 32768) / 256;
        const int val_r = (raw_r + 32768) / 256;
        const int value = juce::jmax(val_l, val_r);

        if (value > silentT) lastSignalMs[ch] = now;

        if (value >= clipT)
        {
            auto existing = clipUntilMs.find(ch);
            const bool wasHolding = (existing != clipUntilMs.end()) && (now < existing->second);
            clipUntilMs[ch] = now + clipHoldMs;
            if (!wasHolding && logOutgoing)
                logOutgoing("CLIP DETECTED ch=" + juce::String(ch) + " value=" + juce::String(value)
                            + " hold=" + juce::String(clipHoldMs / 1000.0, 2) + "s");
        }
    }
}

void WingTcpMixerSettings::sendChannelColor(int ch, int color)
{
    sendNode("/ch." + juce::String(ch) + ".col=" + juce::String(color));
}

void WingTcpMixerSettings::updateBlinking()
{
    // Monitoring disabled → no blink logic. Base colors were already restored when
    // the flag was toggled off, so nothing to do here.
    if (!monitoringEnabled->boolValue()) return;

    const double now = juce::Time::getMillisecondCounterHiRes();
    const double silenceTimeoutMs = silenceTimeout->floatValue() * 1000.0;
    blinkRedPhase = !blinkRedPhase;

    std::map<int, int> snapshot; // ch -> color to send
    {
        std::lock_guard<std::mutex> lock(stateMutex);

        for (auto& kv : baseColors)
        {
            const int ch = kv.first;
            const int base = kv.second;

            // Priority 1: clip hold → solid red until clip hold time expires.
            auto clipIt = clipUntilMs.find(ch);
            const bool clipping = (clipIt != clipUntilMs.end()) && (now < clipIt->second);
            if (clipping)
            {
                snapshot[ch] = COLOR_RED;
                continue;
            }

            // Priority 2: silence blink → alternate red / base color.
            auto sigIt = lastSignalMs.find(ch);
            const bool silent = (sigIt == lastSignalMs.end())
                              || (now - sigIt->second > silenceTimeoutMs);

            snapshot[ch] = (silent && blinkRedPhase) ? COLOR_RED : base;
        }
    }

    // Send color updates outside the lock.
    for (auto& kv : snapshot) sendChannelColor(kv.first, kv.second);
}

// ---------------------------------------------------------------------------
// Sending
// ---------------------------------------------------------------------------

void WingTcpMixerSettings::sendNode(const juce::String& nodeCmd)
{
    if (!connected) return;

    if (logOutgoing)
        logOutgoing("wSetNode: " + nodeCmd);

    const int numBytes = nodeCmd.getNumBytesAsUTF8() + 1;
    std::vector<char> buf(static_cast<size_t>(numBytes));
    nodeCmd.copyToUTF8(buf.data(), numBytes);
    if (wSetNode(buf.data()) != WSUCCESS) handleWapiError();
}

juce::String WingTcpMixerSettings::sanitizeName(const juce::String& s)
{
    // Fold common accented Latin characters to their ASCII equivalents, then
    // keep only safe printable ASCII (no wapi grammar chars: , = / . " ).
    static const struct { juce::juce_wchar from; char to; } folds[] = {
        { 0xe0, 'a' }, { 0xe1, 'a' }, { 0xe2, 'a' }, { 0xe3, 'a' }, { 0xe4, 'a' }, { 0xe5, 'a' },
        { 0xc0, 'A' }, { 0xc1, 'A' }, { 0xc2, 'A' }, { 0xc3, 'A' }, { 0xc4, 'A' }, { 0xc5, 'A' },
        { 0xe7, 'c' }, { 0xc7, 'C' },
        { 0xe8, 'e' }, { 0xe9, 'e' }, { 0xea, 'e' }, { 0xeb, 'e' },
        { 0xc8, 'E' }, { 0xc9, 'E' }, { 0xca, 'E' }, { 0xcb, 'E' },
        { 0xec, 'i' }, { 0xed, 'i' }, { 0xee, 'i' }, { 0xef, 'i' },
        { 0xcc, 'I' }, { 0xcd, 'I' }, { 0xce, 'I' }, { 0xcf, 'I' },
        { 0xf1, 'n' }, { 0xd1, 'N' },
        { 0xf2, 'o' }, { 0xf3, 'o' }, { 0xf4, 'o' }, { 0xf5, 'o' }, { 0xf6, 'o' }, { 0xf8, 'o' },
        { 0xd2, 'O' }, { 0xd3, 'O' }, { 0xd4, 'O' }, { 0xd5, 'O' }, { 0xd6, 'O' }, { 0xd8, 'O' },
        { 0xf9, 'u' }, { 0xfa, 'u' }, { 0xfb, 'u' }, { 0xfc, 'u' },
        { 0xd9, 'U' }, { 0xda, 'U' }, { 0xdb, 'U' }, { 0xdc, 'U' },
        { 0xfd, 'y' }, { 0xff, 'y' }, { 0xdd, 'Y' },
    };

    juce::String out;
    for (auto c : s)
    {
        for (auto& f : folds) if (c == f.from) { c = (juce::juce_wchar) f.to; break; }
        if (c == ',' || c == '=' || c == '/' || c == '.' || c == '"') continue;
        if (c >= 32 && c < 127) out += juce::String::charToString(c);
    }

    return out.substring(0, MAX_NAME_LEN);
}

void WingTcpMixerSettings::sendChannelName(int channelNum, const juce::String& name)
{
    sendNode("/ch." + juce::String(channelNum) + ".name=" + sanitizeName(name));
}

void WingTcpMixerSettings::sendChannelIcon(int channelNum, int icon)
{
    sendNode("/ch." + juce::String(channelNum) + ".icon=" + juce::String(icon));
}

void WingTcpMixerSettings::applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                                               const juce::StringArray& dcaNames,
                                               const juce::Array<int>& definedChannels,
                                               const std::map<int, juce::String>& activeChannelNames,
                                               const std::map<int, std::set<int>>& channelFXBuses,
                                               const juce::Array<int>& definedBuses,
                                               const juce::Array<bool>& dcaHasFX,
                                               const std::map<int, float>& dcaForcedFaders)
{
    if (!connected) return;

    // --- DCA names, colors, LEDs, faders (batch per DCA) ---
    for (int i = 0; i < dcaNames.size(); ++i)
    {
        int channelCount = i < membership.size() ? membership[i].size() : 0;
        bool hasMembers = channelCount > 0;

        int color = COLOR_OFF;
        if (channelCount == 1)      color = COLOR_DCA_SINGLE;
        else if (channelCount > 1)  color = COLOR_DCA_MULTI;

        juce::String cmd = "/dca." + juce::String(i + 1)
            + ".name=" + sanitizeName(dcaNames[i])
            + ",col=" + juce::String(color)
            + ",led=" + juce::String(hasMembers ? 1 : 0);

        auto fdrIt = dcaForcedFaders.find(i);
        if (fdrIt != dcaForcedFaders.end())
            cmd += ",fdr=" + juce::String(fdrIt->second, 1);

        sendNode(cmd);
    }

    // --- Channel state (batch per channel) ---
    // We also record the "base color" of each channel, so the silence-blink logic can
    // alternate between red and the base color without losing the DCA assignment colour.
    std::map<int, int> newBaseColors;

    for (int ch : definedChannels)
    {
        juce::StringArray tags;
        for (int d = 0; d < membership.size(); ++d)
        {
            if (membership[d].contains(ch))
                tags.add("#D" + juce::String(d + 1));
        }

        int dcaChannelCount = 0;
        for (int d = 0; d < membership.size(); ++d)
        {
            if (membership[d].contains(ch)) { dcaChannelCount = membership[d].size(); break; }
        }
        bool isActive = dcaChannelCount > 0;

        int channelColor = COLOR_CHANNEL_OFF;
        if (dcaChannelCount == 1)       channelColor = COLOR_DCA_SINGLE;
        else if (dcaChannelCount > 1)   channelColor = COLOR_DCA_MULTI;

        newBaseColors[ch] = channelColor;

        juce::String cmd = "/ch." + juce::String(ch)
            + ".tags=" + tags.joinIntoString(",")
            + ",clink=0"
            + ",mute=" + juce::String(isActive ? 0 : 1)
            + ",col=" + juce::String(channelColor)
            + ",led=1";

        if (isActive)
        {
            auto it = activeChannelNames.find(ch);
            if (it != activeChannelNames.end())
                cmd += ",name=" + sanitizeName(it->second);
        }

        sendNode(cmd);

        auto fxIt = channelFXBuses.find(ch);
        const std::set<int>* activeBuses = (fxIt != channelFXBuses.end()) ? &fxIt->second : nullptr;
        for (int busNum : definedBuses)
        {
            bool on = (activeBuses != nullptr) && activeBuses->count(busNum) > 0;
            sendNode("/ch." + juce::String(ch) + ".send." + juce::String(busNum) + ".on=" + juce::String(on ? 1 : 0));
        }
    }

    // Commit the base-colour snapshot used by the blink timer.
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        baseColors = std::move(newBaseColors);
    }
}
