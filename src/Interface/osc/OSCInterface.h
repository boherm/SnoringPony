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
#include "OSCCommand.h"
#include "OSCMapping.h"
#include "OSCFeedbackItem.h"

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

// -----------------------------------------------------

class OSCInterface :
    public Interface,
    public Thread,
    public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>,
    public BaseManager<OSCOutput>::ManagerListener,
    public BaseManager<OSCCommand>::ManagerListener,
    public BaseManager<OSCFeedbackItem>::ManagerListener
{
public:
    OSCInterface();
    ~OSCInterface();

    // zeroconf
    Servus servus;

    // inputs
    std::unique_ptr<EnablingControllableContainer> receiveCC;
	IntParameter* localPort;
    BoolParameter* autoAdd;
    OSCReceiver receiver;

    std::unique_ptr<BaseManager<OSCMapping>> mappings;

    void setupReceiver();
	void processMessage(const OSCMessage& msg);
	void oscMessageReceived(const OSCMessage& message) override;
	void oscBundleReceived(const OSCBundle& bundle) override;

    OSCMapping* findExistingMapping(const OSCMessage& msg) const;
    OSCMapping* createMappingFromMessage(const OSCMessage& msg);

    // outputs
    std::unique_ptr<BaseManager<OSCOutput>> outputManager;
    void setupSenders();
	void sendOSC(const OSCMessage& msg);
    void itemAdded(OSCOutput* output) override;
    void itemAdded(OSCCommand* command) override;
    void itemsAdded(Array<OSCCommand*> commands) override;

    // templates
    std::unique_ptr<BaseManager<OSCCommand>> templateManager;

    // feedback
    std::unique_ptr<BaseManager<OSCFeedbackItem>> feedbacks;
    void itemAdded(OSCFeedbackItem* item) override;

    void loadJSONDataInternal(var data) override;
    void onContainerNiceNameChanged() override;
    void onContainerParameterChangedInternal(Parameter* p) override;

    String getTypeString() const override { return "OSC"; }
    static OSCInterface* create(var params) { return new OSCInterface(); };

    void setupZeroConf();
    void run() override;
};
