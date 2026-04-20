/*
  ==============================================================================

    AudioInterface.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "../Interface.h"
#include "AudioInterface.h"
#include "AudioOutput.h"
#include "ui/AudioInterfaceHardwareEditor.h"

AudioModuleHardwareSettings::AudioModuleHardwareSettings(AudioDeviceManager* am) :
	ControllableContainer("Hardware Settings"),
	am(am)
{
    includeInRecursiveSave = false;
    hideEditorHeader = true;
}

InspectableEditor* AudioModuleHardwareSettings::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
	return new AudioInterfaceHardwareEditor(this, isRoot);
}

AudioInterface::AudioInterface() :
    Interface("Audio Interface 1"),
    hs(&am),
    availableDevicesCache()
{
    logIncomingData->remove();
    logOutgoingData->remove();

    outputs = new AudioOutputManager(this);
    addChildControllableContainer(&hs);
    addChildControllableContainer(outputs);
    initSetup();

    if (outputs->items.isEmpty() && !Engine::mainEngine->isLoadingFile)
    {
        AudioOutput* defaultOutput = outputs->addItem();
        defaultOutput->setNiceName("Stereo Out");
        defaultOutput->outputType->setValueWithData("s");
        defaultOutput->savedChan1 = 1;
        defaultOutput->savedChan2 = 2;
        defaultOutput->refreshChannelOptions();
        defaultOutput->setupAudioOutputs();
    }
}

AudioInterface::~AudioInterface()
{
    delete outputs;
	// am.removeAudioCallback(&player);
	// am.removeAudioCallback(this);
	am.removeChangeListener(this);
}

void AudioInterface::initSetup()
{
	if (Thread::getCurrentThreadId() != MessageManager::getInstance()->getCurrentMessageThread())
	{
		MessageManager::callAsync([this]() { initSetup(); });
		return;
	}

	//AUDIO
	// am.addAudioCallback(this);
	am.addChangeListener(this);
	am.initialiseWithDefaultDevices(0, 2);

	// am.addAudioCallback(&player);

	AudioDeviceManager::AudioDeviceSetup setup;
	am.getAudioDeviceSetup(setup);
	currentSampleRate = setup.sampleRate;
	currentBufferSize = setup.bufferSize;

	if (setup.outputDeviceName.isEmpty()) setWarningMessage("This interface isn't connected to an audio output");

    refreshAvailableDevices();
    saveDevicesSize = availableDevicesCache->size();
    saveCurrentDevice = setup.outputDeviceName;
}

void AudioInterface::updateAudioSetup()
{
    AudioDeviceManager::AudioDeviceSetup setup;
    am.getAudioDeviceSetup(setup);
    currentSampleRate = setup.sampleRate;
    currentBufferSize = setup.bufferSize;

    if (getWarningMessage().isEmpty()) {
        if (setup.outputDeviceName.isEmpty()) setWarningMessage("This interface isn't connected to an audio output");
        else if (setup.outputDeviceName != saveCurrentDevice) setWarningMessage(saveCurrentDevice + " isn't connected anymore");
        else clearWarning();
    }
}

var AudioInterface::getJSONData(bool includeNonOverriden)
{
	var data = Interface::getJSONData(includeNonOverriden);

	std::unique_ptr<XmlElement> xmlData(am.createStateXml());
	if (xmlData != nullptr)
	{
		data.getDynamicObject()->setProperty("audioSettings", xmlData->toString());
	}

	return data;
}

void AudioInterface::loadJSONDataInternal(var data)
{
	if (data.getDynamicObject()->hasProperty("audioSettings"))
	{

		std::unique_ptr<XmlElement> elem = XmlDocument::parse(data.getProperty("audioSettings", ""));
        saveCurrentDevice = elem.get()->getStringAttribute("audioOutputDeviceName", "");
        saveOutputs = elem.get()->getStringAttribute("audioDeviceOutChans", "0");
        saveDevicesSize = 0;
		am.initialise(0, 2, elem.get(), true);
	}

	updateAudioSetup();

	Interface::loadJSONDataInternal(data);
}

void AudioInterface::changeListenerCallback(ChangeBroadcaster*)
{
    AudioDeviceManager::AudioDeviceSetup setup;
    am.getAudioDeviceSetup(setup);

    refreshAvailableDevices();
    int newDeviceAvailableSize = availableDevicesCache->size();

    // if we detect a change in the available audio devices
    if (newDeviceAvailableSize != saveDevicesSize) {

        if (newDeviceAvailableSize < saveDevicesSize && saveCurrentDevice != setup.outputDeviceName) {
            setWarningMessage(saveCurrentDevice + " isn't connected anymore");
        }

        if (newDeviceAvailableSize > saveDevicesSize) {

            for (int i = 0; i < newDeviceAvailableSize; i++) {
                if (availableDevicesCache->getReference(i) == saveCurrentDevice) {
                    setup.outputDeviceName = saveCurrentDevice;
                    if (saveOutputs.isNotEmpty()) {
                        BigInteger test = BigInteger(std::stoi(saveOutputs.toStdString(), nullptr, 2));
                        setup.outputChannels = test;
                        saveOutputs = "";
                    }
                    am.initialise(0, setup.outputChannels.countNumberOfSetBits(), nullptr, true);
                    am.setAudioDeviceSetup(setup, true);

                    clearWarning();
                }
            }

        }

        saveDevicesSize = newDeviceAvailableSize;
    } else {
        saveCurrentDevice = setup.outputDeviceName;
        saveOutputs = setup.outputChannels.toString(2);
    }

    updateAudioSetup();
    outputs->refreshAllOutputsChannelOptions();
}

void AudioInterface::refreshAvailableDevices()
{
    availableDevicesCache.reset(new StringArray());

    const auto& deviceTypes = am.getAvailableDeviceTypes();

    for (auto* deviceType : deviceTypes)
    {
        deviceType->scanForDevices();

        for (const auto& device : deviceType->getDeviceNames())
        {
            availableDevicesCache->add(device);
        }
    }
}
