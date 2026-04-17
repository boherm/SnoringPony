/*
  ==============================================================================

    WingTcpMixerSettings.h
    Created: 17 Apr 2026
    Author:  boherm

    Behringer Wing mixer settings using the wapi TCP library (port 2222).
    Uses wSetNode() for efficient batch parameter updates. Also subscribes to
    the Wing's UDP meter stream to detect silent channels and make them blink
    red when they stay silent for more than 2 seconds.

  ==============================================================================
*/

#pragma once

#include "../MixerSettings.h"
#include <mutex>

extern "C" {
#include "wapi.h"
#include "wext.h"
}

class WingTcpMixerSettings : public MixerSettings,
                             private juce::Timer
{
public:
    WingTcpMixerSettings();
    ~WingTcpMixerSettings() override;

    StringParameter* remoteHost;
    IntParameter* remotePort;
    IntParameter* numDCAs;
    BoolParameter* monitoringEnabled;
    FloatParameter* silenceTimeout;
    FloatParameter* silentThresholdDb;
    FloatParameter* clipThresholdDb;
    FloatParameter* clipHoldTime;
    BoolParameter* connectedParam;

    int getNumDCAs() const override { return numDCAs->intValue(); }

    void connect() override;
    void disconnect() override;

    // Disconnects without enabling auto-reconnect (called internally for clean teardown).
    void disconnectInternal();
    bool isConnected() const override;
    bool shouldReconnectOnChange(Controllable* c) const override;

    void sendChannelName(int channelNum, const juce::String& name) override;
    void sendChannelIcon(int channelNum, int icon) override;

    void applyDCAMembership(const juce::Array<juce::Array<int>>& membership,
                            const juce::StringArray& dcaNames,
                            const juce::Array<int>& definedChannels,
                            const std::map<int, juce::String>& activeChannelNames,
                            const std::map<int, std::set<int>>& channelFXBuses,
                            const juce::Array<int>& definedBuses,
                            const juce::Array<bool>& dcaHasFX,
                            const std::map<int, float>& dcaForcedFaders) override;

private:
    // Sends a wSetNode command string and logs it.
    void sendNode(const juce::String& nodeCmd);

    // Sanitize a channel/DCA name: strip accents/special chars, remove wapi grammar
    // characters (, = / . " ), and truncate to MAX_NAME_LEN.
    static juce::String sanitizeName(const juce::String& s);

    // Called periodically to drive keepalive, meter renew, and blink updates.
    void timerCallback() override;

    // Called when a wapi call returns an error, to mark the connection as broken.
    void handleWapiError();

    void onContainerParameterChanged(Parameter* p) override;

    // --- Meter / silence detection ---
    class MeterThread : public juce::Thread
    {
    public:
        MeterThread(WingTcpMixerSettings& o) : juce::Thread("WingMeter"), owner(o) {}
        void run() override;
    private:
        WingTcpMixerSettings& owner;
    };
    std::unique_ptr<MeterThread> meterThread;

    void setupMeterSubscription();
    void teardownMeterSubscription();
    void processMeterPacket(const unsigned char* data, int size);
    void updateBlinking();
    void sendChannelColor(int ch, int color);

    // State shared between meter thread and message thread. Protected by stateMutex.
    std::mutex stateMutex;
    std::map<int, double> lastSignalMs;   // channel -> last time signal was above threshold
    std::map<int, int>    baseColors;     // channel -> normal color (restore when signal OK)
    std::map<int, double> clipUntilMs;    // channel -> timestamp until which clip-red is held
    bool blinkRedPhase = false;

    juce::int64 tickCount = 0;

    static constexpr int DEFAULT_TCP_PORT = 2222;
    static constexpr int MAX_DCAS = 16;

    // Wing colour palette (same values as OSC protocol)
    static constexpr int COLOR_OFF = 0;
    static constexpr int COLOR_LIGHT_BLUE = 14;
    static constexpr int COLOR_GREEN = 5;
    static constexpr int COLOR_YELLOW = 7;
    static constexpr int COLOR_RED = 9;
    static constexpr int COLOR_DCA_SINGLE = COLOR_LIGHT_BLUE;
    static constexpr int COLOR_DCA_MULTI = COLOR_GREEN;
    static constexpr int COLOR_CHANNEL_OFF = COLOR_YELLOW;

    bool connected = false;

    // When true, the timer will keep retrying connect() if not connected.
    bool autoReconnect = false;

    // Timer cadence: connected = fast ticks (for blink). disconnected = slow (reconnect).
    static constexpr int TICK_INTERVAL_MS = 250;
    static constexpr int RECONNECT_INTERVAL_MS = 3000;

    // Slow operations triggered by tick counter
    static constexpr int KEEPALIVE_EVERY_N_TICKS = 5000 / TICK_INTERVAL_MS;   // 20 → 5s
    static constexpr int METER_RENEW_EVERY_N_TICKS = 4000 / TICK_INTERVAL_MS; // 16 → 4s

    // Silence detection — meter values are in 0..256 scale (0 = silent, 110+ = loud).
    static constexpr float DEFAULT_SILENCE_TIMEOUT_SEC = 2.0f;
    static constexpr float DEFAULT_SILENT_THRESHOLD_DB = -55.0f; // ≈ raw v=40
    static constexpr float DEFAULT_CLIP_THRESHOLD_DB = 0.0f;     // ≈ raw v=128
    static constexpr float DEFAULT_CLIP_HOLD_SEC = 2.0f;

    // Meter scale conversion: linear in dB from v=0 (-80 dBFS) to v=128 (0 dBFS).
    // dBFS = (v - 128) * (80 / 128) = (v - 128) * 0.625
    static constexpr float DB_PER_VALUE_UNIT = 0.625f;
    static int dbToMeterValue(float db) { return (int) std::round(db / DB_PER_VALUE_UNIT) + 128; }

    static constexpr int MAX_NAME_LEN = 16;

    // Meter UDP subscription
    static constexpr int METER_LOCAL_UDP_PORT = 4443;
    static constexpr int METER_REQ_ID = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingTcpMixerSettings)
};
