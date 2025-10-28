/*
  ==============================================================================

    AudioOutput.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class AudioInterface;

class AudioOutput :
    public BaseItem
{
public:
    AudioOutput(var params = var(), AudioInterface* audioInterface = nullptr);
    virtual ~AudioOutput();

    AudioInterface* audioInterface = nullptr;
    AudioDeviceManager am;

    EnumParameter* outputType;
    EnumParameter* chan1;
    EnumParameter* chan2;

    Trigger* testTrigger;

    int savedChan1 = 1;
    int savedChan2 = 1;

    void initAudioSetup();

	void onContainerParameterChangedInternal(Parameter *) override;

    void resolveWarning() override;

    void refreshChannelOptions();

    void setupAudioOutputs();
    void playTestSound();

    void onContainerTriggerTriggered(Trigger* t);

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables) override;

    var getJSONData(bool includeNonOverriden = false) override;
    void loadJSONDataInternal(var data) override;

    String getTypeString() const override { return "AudioOutput"; }
    static AudioOutput *create(var params) { return new AudioOutput(params); }
};

class AudioOutputManager :
    public BaseManager<AudioOutput>
{
public:
    AudioOutputManager(AudioInterface* audioInterface);
    ~AudioOutputManager();

    AudioInterface* audioInterface;

    void refreshAllOutputsChannelOptions();
    AudioOutput* createItem() override;
};
