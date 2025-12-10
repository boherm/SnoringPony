/*
  ==============================================================================

    OSCInterface.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "OSCInterface.h"
#include "ui/OSCInputEditor.h"
#include "OSCCommand.h"

OSCInterface::OSCInterface() :
    Interface("OSC Interface 1"),
    Thread("OSCZeroconf"),
    servus("_osc._udp")
{
    // Inputs
    receiveCC.reset(new EnablingControllableContainer("OSC Input"));
	receiveCC->customGetEditorFunc = &OSCInputEditor::create;
	addChildControllableContainer(receiveCC.get());

    localPort = receiveCC->addIntParameter("Local Port", "Local port to receive OSC messages", 14000, 1024, 65535);
    localPort->warningResolveInspectable = this;

	receiver.registerFormatErrorHandler(&OSCHelpers::logOSCFormatError);
	receiver.addListener(this);

	if (!Engine::mainEngine->isLoadingFile) setupReceiver();

    // Outputs
    outputManager.reset(new BaseManager<OSCOutput>("OSC Outputs"));
    outputManager->addBaseManagerListener(this);
    addChildControllableContainer(outputManager.get());

	if (!Engine::mainEngine->isLoadingFile)
	{
		OSCOutput* o = outputManager->addItem(nullptr, var(), false);
		o->remotePort->setValue(44000);
		if (!Engine::mainEngine->isLoadingFile) setupSenders();
	}

    // Templates for commands
    templateManager.reset(new OSCCommandManager());
    templateManager->interface = this;
    addChildControllableContainer(templateManager.get());

    // start zeroconf thread
    if (!isThreadRunning() && !Engine::mainEngine->isLoadingFile) startThread();
}

OSCInterface::~OSCInterface()
{
	if (isThreadRunning())
	{
		signalThreadShouldExit();
		waitForThreadToExit(1000);
		stopThread(100);
	}
}

void OSCInterface::loadJSONDataInternal(var data)
{
	Interface::loadJSONDataInternal(data);
    setupSenders();
	if (!isThreadRunning()) startThread();
	setupReceiver();
}

void OSCInterface::onContainerNiceNameChanged()
{
	Interface::onContainerNiceNameChanged();
	if (Engine::mainEngine->isLoadingFile || Engine::mainEngine->isClearing) return;
	if (!isThreadRunning()) startThread();
}

void OSCInterface::onContainerParameterChangedInternal(Parameter* p)
{
    if (Engine::mainEngine->isLoadingFile || Engine::mainEngine->isClearing) return;
    if (p == localPort)
    {
        if (!isThreadRunning()) startThread();
    }
    NLOG(niceName, "Parameter changed: " << p->niceName);
}

void OSCInterface::setupZeroConf()
{
	if (Engine::mainEngine->isClearing || localPort == nullptr) return;

	String nameToAdvertise;
	int portToAdvertise = 0;
	while (nameToAdvertise != niceName || portToAdvertise != localPort->intValue())
	{
		nameToAdvertise = niceName;
		portToAdvertise = localPort->intValue();

		servus.withdraw();

		if (!receiveCC->enabled->boolValue()) return;

		servus.announce(portToAdvertise, ("SnoringPony - " + nameToAdvertise).toStdString());
	}

	NLOG(niceName, "Zeroconf service created : " << nameToAdvertise << ":" << portToAdvertise);
}

void OSCInterface::run()
{
    setupZeroConf();
}

void OSCInterface::setupReceiver()
{
	receiver.disconnect();
	if (receiveCC == nullptr) return;

	if (!receiveCC->enabled->boolValue())
	{
		localPort->clearWarning();
		return;
	}

	//DBG("Local port set to : " << localPort->intValue());
	bool result = receiver.connect(localPort->intValue());

	if (result)
	{
		NLOG(niceName, "Now receiving on port : " + localPort->stringValue());
		if (!isThreadRunning() && !Engine::mainEngine->isLoadingFile) startThread();

		Array<IPAddress> ad;

		IPAddress::findAllAddresses(ad);
		Array<String> ips;
		for (auto& a : ad) ips.add(a.toString());
		ips.sort();
		String s = "Local IPs:";
		for (auto& ip : ips) s += String("\n > ") + ip;

		NLOG(niceName, s);
		localPort->clearWarning();
	}
	else
	{
		NLOGERROR(niceName, "Error binding port " + localPort->stringValue());
		localPort->setWarningMessage("Error binding port " + localPort->stringValue());
	}

}

void OSCInterface::oscMessageReceived(const OSCMessage& message)
{
	if (!enabled->boolValue() || !receiveCC->enabled->boolValue()) return;
	processMessage(message);
}

void OSCInterface::oscBundleReceived(const OSCBundle& bundle)
{
	if (!enabled->boolValue()) return;
	for (auto& m : bundle)
	{
		processMessage(m.getMessage());
	}
}

void OSCInterface::processMessage(const OSCMessage& msg)
{
	if (logIncomingData->boolValue())
	{
		String s = "";
		for (auto& a : msg) s += String(" ") + OSCHelpers::getStringArg(a);
		NLOG(niceName, msg.getAddressPattern().toString() << " :" << s);
	}

	// processMessageInternal(msg);
}

void OSCInterface::itemAdded(OSCOutput* output)
{
	output->warningResolveInspectable = this;
}

void OSCInterface::setupSenders()
{
	if (outputManager == nullptr) return;
	for (auto& o : outputManager->items) o->setupSender();
}

void OSCInterface::sendOSC(const OSCMessage& msg)
{
	if (isClearing || outputManager == nullptr) return;
	if (!enabled->boolValue()) return;

	if (logOutgoingData->boolValue())
	{
		NLOG(niceName, "Send OSC : " << msg.getAddressPattern().toString());
		for (auto& a : msg)
		{
			LOG(OSCHelpers::getStringArg(a));
		}
	}

    for (auto& o : outputManager->items) o->sendOSC(msg);
}

// --------------------------------------------------------------------

OSCOutput::OSCOutput() :
	BaseItem("OSC Output"),
	Thread("OSC output"),
	forceDisabled(false),
	senderIsConnected(false)
{
	isSelectable = false;

	useLocal = addBoolParameter("Local", "Send to Local IP (127.0.0.1). Allow to quickly switch between local and remote IP.", true);
	remoteHost = addStringParameter("Remote Host", "Remote Host to send to.", "127.0.0.1");
	remoteHost->autoTrim = true;
	remoteHost->setEnabled(!useLocal->boolValue());
	remotePort = addIntParameter("Remote port", "Port on which the remote host is listening to", 44000, 1024, 65535);

	if (!Engine::mainEngine->isLoadingFile) setupSender();
}

OSCOutput::~OSCOutput()
{
	signalThreadShouldExit();
	waitForThreadToExit(1000);
	stopThread(1000);
}

void OSCOutput::setForceDisabled(bool value)
{
	if (forceDisabled == value) return;
	forceDisabled = value;
	setupSender();
}

void OSCOutput::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == remoteHost || p == remotePort || p == useLocal)
	{
		if (!Engine::mainEngine->isLoadingFile) setupSender();
		if (p == useLocal) remoteHost->setEnabled(!useLocal->boolValue());
	}
	else if (p == enabled)
	{
		if (!Engine::mainEngine->isLoadingFile) setupSender();
	}
}

void OSCOutput::setupSender()
{
	signalThreadShouldExit();
	notify();
	waitForThreadToExit(1000);

	senderIsConnected = false;
	sender.disconnect();

	if (isThreadRunning())
	{
		stopThread(1000);
		// Clear queue
		const ScopedLock sl(queueLock);
		while (!messageQueue.empty())
			messageQueue.pop();
	}

	if (!enabled->boolValue() || forceDisabled || Engine::mainEngine->isClearing) return;

	String targetHost = useLocal->boolValue() ? "127.0.0.1" : remoteHost->stringValue();
	senderIsConnected = sender.connect(targetHost, remotePort->intValue());
	if (senderIsConnected)
	{
		startThread();

		NLOG(niceName, "Now sending to " + remoteHost->stringValue() + ":" + remotePort->stringValue());
		clearWarning();
	}
	else
	{
		NLOGWARNING(niceName, "Could not connect to " << remoteHost->stringValue() << ":" + remotePort->stringValue());
		setWarningMessage("Could not connect to " + remoteHost->stringValue() + ":" + remotePort->stringValue());
	}
}

void OSCOutput::sendOSC(const OSCMessage& m)
{
	if (!enabled->boolValue() || forceDisabled || !senderIsConnected) return;

	{
		const ScopedLock sl(queueLock);
		messageQueue.push(std::make_unique<OSCMessage>(m));
	}
	notify();
}

void OSCOutput::run()
{
	while (!Engine::mainEngine->isClearing && !threadShouldExit())
	{
		std::unique_ptr<OSCMessage> msgToSend;

		{
			const ScopedLock sl(queueLock);
			if (!messageQueue.empty())
			{
				msgToSend = std::move(messageQueue.front());
				messageQueue.pop();
			}
		}

		if (msgToSend)
			sender.send(*msgToSend);
		else
			wait(1000); // notify() is called when a message is added to the queue
	}

	// Clear queue
	const ScopedLock sl(queueLock);
	while (!messageQueue.empty())
		messageQueue.pop();
}
