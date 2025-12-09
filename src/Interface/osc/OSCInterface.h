/*
  ==============================================================================

    OSCInterface.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../Interface.h"
#include "../../MainIncludes.h"

using namespace servus;

class OSCOutput :
	public BaseItem,
	public Thread
{
public:
	OSCOutput();
	~OSCOutput();

	bool forceDisabled;
	bool senderIsConnected;

	// send
	BoolParameter* useLocal;
	StringParameter* remoteHost;
	IntParameter* remotePort;

	void setForceDisabled(bool value);

	void setupSender();
	void sendOSC(const OSCMessage& m);

	virtual void run() override;

	void onContainerParameterChangedInternal(Parameter* p) override;

	// virtual InspectableEditor* getEditorInternal(bool isRoot);

private:
	OSCSender sender;
	std::queue<std::unique_ptr<OSCMessage>> messageQueue;
	CriticalSection queueLock;
};

class OSCInterface :
    public Interface,
    public Thread,
    public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>,
    public BaseManager<OSCOutput>::ManagerListener
{
public:
    OSCInterface();
    ~OSCInterface();

    // zeroconf
    Servus servus;

    // inputs
    std::unique_ptr<EnablingControllableContainer> receiveCC;
	IntParameter* localPort;
    OSCReceiver receiver;

    void setupReceiver();
	void processMessage(const OSCMessage& msg);
	void oscMessageReceived(const OSCMessage& message) override;
	void oscBundleReceived(const OSCBundle& bundle) override;

    // outputs
    std::unique_ptr<BaseManager<OSCOutput>> outputManager;
    void setupSenders();
	void sendOSC(const OSCMessage& msg);
    void itemAdded(OSCOutput* output) override;

    void loadJSONDataInternal(var data) override;
    void onContainerNiceNameChanged() override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "OSC"; }
    static OSCInterface* create(var params) { return new OSCInterface(); };

    void setupZeroConf();
    void run() override;
};
