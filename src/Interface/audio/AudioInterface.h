/*
  ==============================================================================

    AudioInterface.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "../../MainIncludes.h"
#include "../Interface.h"
#include "AudioOutput.h"
#include "juce_events/juce_events.h"
#include <memory>

#pragma once

class AudioModuleHardwareSettings :
	public ControllableContainer
{
public:
	AudioModuleHardwareSettings(AudioDeviceManager* am);
	~AudioModuleHardwareSettings() {}
	AudioDeviceManager* am;

	InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables = Array<Inspectable*>()) override;
};

class AudioInterface :
    public Interface,
    public ChangeListener
{
public:
    AudioInterface();
    ~AudioInterface();

    AudioModuleHardwareSettings hs;
    AudioDeviceManager am;

    AudioOutputManager outputs;

	double currentSampleRate;
	int currentBufferSize;

    void initSetup();
    void updateAudioSetup();

    var getJSONData(bool includeNonOverriden = false) override;
    void loadJSONDataInternal(var data) override;

    void refreshAvailableDevices();

    void changeListenerCallback(ChangeBroadcaster*) override;

    String getTypeString() const override { return "Audio"; }
    static AudioInterface* create(var params) { return new AudioInterface(); };

private:
    std::unique_ptr<StringArray> availableDevicesCache;
    int saveDevicesSize;
    String saveCurrentDevice;
    String saveOutputs;
};
