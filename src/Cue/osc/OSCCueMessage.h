/*
  ==============================================================================

    OSCCueMessage.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "OSCCue.h"

class OSCCommand;

class OSCCueMessageArguments :
    public GenericControllableManager
{
public:
    OSCCueMessageArguments();
    virtual ~OSCCueMessageArguments() {}
};

// -----------------------------------------------------

class OSCCueMessage :
    public BaseItem,
    public ContainerAsyncListener
{
public:
    OSCCueMessage(String name = "OSC Message 1");
    virtual ~OSCCueMessage();

    OSCCue* cue;
    Trigger* testBtn;
    TargetParameter* targetTemplate;
    StringParameter* address;

    OSCCommand* templateMessage = nullptr;

    std::unique_ptr<OSCCueMessageArguments> argumentsContainer;

    String getTypeString() const override { return "OSC Message"; }
    static OSCCueMessage* create(var params) { return new OSCCueMessage(params.getProperty("name", "OSC Message 1")); };

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);
    void showMenuForTargetOscMessage(ControllableContainer* startFromCC, std::function<void(ControllableContainer*)> returnFunc);

    void onContainerTriggerTriggered(Trigger* t) override;
	void onContainerParameterChanged(Parameter *) override;

	void loadJSONDataItemInternal(juce::var data) override;

    OSCMessage buildMessage();
    void sendTestMessage();

    void newMessage(const ContainerAsyncEvent& e) override;
};
