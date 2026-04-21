/*
  ==============================================================================

    OBSInterface.cpp
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OBSInterface.h"

OBSInterface::OBSInterface() :
    Interface("OBS Interface 1"),
    connectionState(Disconnected)
{
    host = addStringParameter("Host", "OBS WebSocket server address", "127.0.0.1");
    host->autoTrim = true;
    port = addIntParameter("Port", "OBS WebSocket server port", 4455, 1024, 65535);
    password = addStringParameter("Password", "OBS WebSocket password (leave empty if no auth)", "");
    autoConnect = addBoolParameter("Auto Connect", "Automatically reconnect when loading a session", false);
    reconnectDelay = addIntParameter("Reconnect Delay", "Delay in seconds before attempting reconnection", 2, 1, 30);
    connectBtn = addTrigger("Connect", "Connect / Disconnect from OBS");

    logIncomingData->hideInEditor = true;

    isConnectedParam = addBoolParameter("Connected", "Connection status", false);
    isConnectedParam->setControllableFeedbackOnly(true);

    // Request templates
    templateManager.reset(new BaseManager<OBSCommand>("OBS Request Templates"));
    templateManager->selectItemWhenCreated = false;
    templateManager->addBaseManagerListener(this);
    addChildControllableContainer(templateManager.get());
}

OBSInterface::~OBSInterface()
{
    stopTimer();
    if (templateManager != nullptr) templateManager->removeBaseManagerListener(this);
    disconnectOBS();
}

void OBSInterface::connectOBS()
{
    disconnectOBS();

    String serverPath = host->stringValue() + ":" + String(port->intValue());
    NLOG(niceName, "Connecting to OBS at " << serverPath << "...");

    connectionState = Connecting;

    ws.reset(new SimpleWebSocketClient());
    ws->addWebSocketListener(this);
    ws->start(serverPath);
}

void OBSInterface::disconnectOBS()
{
    if (ws != nullptr)
    {
        ws->removeWebSocketListener(this);
        ws->stop();
        ws.reset();
    }

    connectionState = Disconnected;

    MessageManager::callAsync([safeThis = WeakReference<Inspectable>(this)]()
    {
        if (safeThis == nullptr) return;
        auto* self = dynamic_cast<OBSInterface*>(safeThis.get());
        if (self != nullptr && self->isConnectedParam != nullptr)
            self->isConnectedParam->setValue(false);
    });
}

// --- WebSocket listener callbacks ---

void OBSInterface::connectionOpened()
{
    connectionState = WaitingForHello;
    NLOG(niceName, "WebSocket connected, waiting for Hello...");
}

void OBSInterface::messageReceived(const juce::String& message)
{
    var json = JSON::parse(message);
    if (!json.isObject()) return;

    int op = json.getProperty("op", -1);
    var d = json.getProperty("d", var());

    switch (op)
    {
        case 0: handleHello(d); break;
        case 2: handleIdentified(d); break;
        case 5: handleEvent(d); break;
        case 7: handleRequestResponse(d); break;
        default: break;
    }
}

void OBSInterface::connectionClosed(int status, const juce::String& reason)
{
    connectionState = Disconnected;
    MessageManager::callAsync([safeThis = WeakReference<Inspectable>(this), status, reason]()
    {
        if (safeThis == nullptr) return;
        auto* self = dynamic_cast<OBSInterface*>(safeThis.get());
        if (self == nullptr) return;
        self->isConnectedParam->setValue(false);
        self->setWarningMessage("Connection lost: " + reason);
        self->startTimer(self->reconnectDelay->intValue() * 1000);
        NLOG(self->niceName, "Connection closed (status " << status << "): " << reason);
    });
}

void OBSInterface::connectionError(const juce::String& message)
{
    connectionState = Disconnected;
    MessageManager::callAsync([safeThis = WeakReference<Inspectable>(this), message]()
    {
        if (safeThis == nullptr) return;
        auto* self = dynamic_cast<OBSInterface*>(safeThis.get());
        if (self == nullptr) return;
        self->isConnectedParam->setValue(false);
        self->setWarningMessage("Connection error: " + message);
        self->startTimer(self->reconnectDelay->intValue() * 1000);
        NLOGERROR(self->niceName, "Connection error: " << message);
    });
}

// --- Protocol handlers ---

void OBSInterface::handleHello(const var& d)
{
    // Build Identify message (op 1)
    auto* identifyData = new DynamicObject();
    identifyData->setProperty("rpcVersion", 1);

    // Subscribe to all events
    identifyData->setProperty("eventSubscriptions", (1 << 16) - 1);

    var authField = d.getProperty("authentication", var());
    if (authField.isObject() && password->stringValue().isNotEmpty())
    {
        String challenge = authField.getProperty("challenge", "").toString();
        String salt = authField.getProperty("salt", "").toString();
        String authString = computeAuthString(password->stringValue(), challenge, salt);
        identifyData->setProperty("authentication", authString);
        connectionState = Authenticating;
    }
    else
    {
        connectionState = Authenticating;
    }

    auto* envelope = new DynamicObject();
    envelope->setProperty("op", 1);
    envelope->setProperty("d", var(identifyData));

    String jsonStr = JSON::toString(var(envelope), true);

    if (logOutgoingData->boolValue())
    {
        NLOG(niceName, "Sending Identify: " << jsonStr);
    }

    if (ws != nullptr) ws->send(jsonStr);
}

void OBSInterface::handleIdentified(const var& d)
{
    connectionState = Identified;
    MessageManager::callAsync([safeThis = WeakReference<Inspectable>(this)]()
    {
        if (safeThis == nullptr) return;
        auto* self = dynamic_cast<OBSInterface*>(safeThis.get());
        if (self == nullptr) return;
        self->stopTimer();
        self->clearWarning();
        self->isConnectedParam->setValue(true);
        NLOG(self->niceName, "Successfully connected and identified with OBS");
    });
}

void OBSInterface::handleEvent(const var& d)
{
}

void OBSInterface::handleRequestResponse(const var& d)
{
    String requestType = d.getProperty("requestType", "").toString();
    var requestStatus = d.getProperty("requestStatus", var());
    bool success = requestStatus.getProperty("result", false);

    if (!success)
    {
        MessageManager::callAsync([safeThis = WeakReference<Inspectable>(this), requestType, requestStatus]()
        {
            if (safeThis == nullptr) return;
            auto* self = dynamic_cast<OBSInterface*>(safeThis.get());
            if (self == nullptr) return;
            NLOGWARNING(self->niceName, "Request " << requestType << " failed: "
                << requestStatus.getProperty("comment", "").toString());
        });
    }
    else if (logOutgoingData->boolValue())
    {
        MessageManager::callAsync([safeThis = WeakReference<Inspectable>(this), requestType]()
        {
            if (safeThis == nullptr) return;
            auto* self = dynamic_cast<OBSInterface*>(safeThis.get());
            if (self == nullptr) return;
            NLOG(self->niceName, "Request " << requestType << " succeeded");
        });
    }
}

// --- Authentication ---

String OBSInterface::computeAuthString(const String& pwd, const String& challenge, const String& salt)
{
    // Step 1: secret = Base64(SHA256(password + salt))
    String secretInput = pwd + salt;
    juce::SHA256 secretHash(secretInput.toUTF8());
    MemoryBlock secretBytes = secretHash.getRawData();

    MemoryOutputStream secretStream;
    Base64::convertToBase64(secretStream, secretBytes.getData(), secretBytes.getSize());
    String secretBase64 = secretStream.toString();

    // Step 2: auth = Base64(SHA256(secret + challenge))
    String authInput = secretBase64 + challenge;
    juce::SHA256 authHash(authInput.toUTF8());
    MemoryBlock authBytes = authHash.getRawData();

    MemoryOutputStream authStream;
    Base64::convertToBase64(authStream, authBytes.getData(), authBytes.getSize());
    return authStream.toString();
}

// --- Send OBS request ---

void OBSInterface::sendRequest(const String& requestType, const var& requestData)
{
    if (ws == nullptr || connectionState != Identified) return;
    if (!enabled->boolValue()) return;

    int reqId = nextRequestId.fetch_add(1);

    auto* reqObj = new DynamicObject();
    reqObj->setProperty("requestType", requestType);
    reqObj->setProperty("requestId", String("req-") + String(reqId));

    if (requestData.isObject())
        reqObj->setProperty("requestData", requestData);

    auto* envelope = new DynamicObject();
    envelope->setProperty("op", 6);
    envelope->setProperty("d", var(reqObj));

    String jsonStr = JSON::toString(var(envelope), true);

    if (logOutgoingData->boolValue())
    {
        NLOG(niceName, "Send request: " << requestType << " " << jsonStr);
    }

    ws->send(jsonStr);
}

// --- Manager listener ---

void OBSInterface::itemAdded(OBSCommand* command)
{
    command->interface = this;
    command->warningResolveInspectable = this;
}

void OBSInterface::itemsAdded(Array<OBSCommand*> commands)
{
    for (auto* c : commands)
    {
        c->interface = this;
        c->warningResolveInspectable = this;
    }
}

// --- Parameter / trigger callbacks ---

void OBSInterface::onContainerParameterChangedInternal(Parameter* p)
{
    if (Engine::mainEngine->isLoadingFile || Engine::mainEngine->isClearing) return;

    if (p == autoConnect && !autoConnect->boolValue())
    {
        if (isTimerRunning())
        {
            stopTimer();
            NLOG(niceName, "Auto-reconnect stopped");
        }
    }
    else if (p == host || p == port || p == password)
    {
        if (connectionState == Identified)
        {
            connectOBS();
        }
    }
}

void OBSInterface::onContainerTriggerTriggered(Trigger* t)
{
    if (t == connectBtn)
    {
        if (connectionState == Identified)
        {
            stopTimer();
            clearWarning();
            disconnectOBS();
        }
        else
        {
            connectOBS();
        }
    }
}

void OBSInterface::loadJSONDataInternal(var data)
{
    Interface::loadJSONDataInternal(data);

    if (autoConnect->boolValue())
    {
        MessageManager::callAsync([safeThis = WeakReference<Inspectable>(this)]()
        {
            if (safeThis == nullptr) return;
            auto* self = dynamic_cast<OBSInterface*>(safeThis.get());
            if (self != nullptr) self->connectOBS();
        });
    }
}

void OBSInterface::timerCallback()
{
    stopTimer();
    if (connectionState == Disconnected)
    {
        NLOG(niceName, "Attempting auto-reconnect...");
        connectOBS();
    }
}
