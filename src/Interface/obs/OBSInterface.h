/*
  ==============================================================================

    OBSInterface.h
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../Interface.h"
#include "../../MainIncludes.h"
#include "OBSCommand.h"

class OBSInterface :
    public Interface,
    public SimpleWebSocketClientBase::Listener,
    public BaseManager<OBSCommand>::ManagerListener,
    private juce::Timer
{
public:
    OBSInterface();
    ~OBSInterface();

    enum ConnectionState { Disconnected, Connecting, WaitingForHello, Authenticating, Identified };

    // Connection parameters
    StringParameter* host;
    IntParameter* port;
    StringParameter* password;
    BoolParameter* autoConnect;
    IntParameter* reconnectDelay;
    Trigger* connectBtn;
    BoolParameter* isConnectedParam;

    // Protocol state
    ConnectionState connectionState;
    std::atomic<int> nextRequestId { 1 };

    // WebSocket client
    std::unique_ptr<SimpleWebSocketClient> ws;

    // Request templates
    std::unique_ptr<BaseManager<OBSCommand>> templateManager;

    // Connection lifecycle
    void connectOBS();
    void disconnectOBS();

    // WebSocket listener callbacks
    void connectionOpened() override;
    void messageReceived(const juce::String& message) override;
    void connectionClosed(int status, const juce::String& reason) override;
    void connectionError(const juce::String& message) override;

    // Protocol
    void handleHello(const var& data);
    void handleIdentified(const var& data);
    void handleEvent(const var& data);
    void handleRequestResponse(const var& data);
    String computeAuthString(const String& pwd, const String& challenge, const String& salt);

    // Send an OBS request (op 6)
    void sendRequest(const String& requestType, const var& requestData = var());

    // Auto-reconnect timer
    void timerCallback() override;

    // Manager listener
    void itemAdded(OBSCommand* command) override;
    void itemsAdded(Array<OBSCommand*> commands) override;

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onContainerTriggerTriggered(Trigger* t) override;
    void loadJSONDataInternal(var data) override;

    String getTypeString() const override { return "OBS"; }
    static OBSInterface* create(var params) { return new OBSInterface(); }
};
