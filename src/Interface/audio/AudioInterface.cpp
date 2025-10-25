/*
  ==============================================================================

    AudioInterface.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioInterface.h"
#include "AudioOutput.h"
#include "juce_core/juce_core.h"
#include "ui/AudioInterfaceHardwareEditor.h"

AudioModuleHardwareSettings::AudioModuleHardwareSettings(AudioDeviceManager* am) :
	ControllableContainer("Hardware Settings"),
	am(am)
{
    includeInRecursiveSave = false;
}

InspectableEditor* AudioModuleHardwareSettings::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
	return new AudioInterfaceHardwareEditor(this, isRoot);
}

AudioInterface::AudioInterface() :
    Interface("Audio Interface 1"),
    hs(&am),
    outputs(this)
{
    this->logIncomingData->remove();
    this->logOutgoingData->remove();

    addChildControllableContainer(&hs);
    addChildControllableContainer(&outputs);
    initSetup();
}

AudioInterface::~AudioInterface()
{
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
}

void AudioInterface::updateAudioSetup()
{
    AudioDeviceManager::AudioDeviceSetup setup;
    am.getAudioDeviceSetup(setup);
    currentSampleRate = setup.sampleRate;
    currentBufferSize = setup.bufferSize;

	if (setup.outputDeviceName.isEmpty()) setWarningMessage("This interface isn't connected to an audio output");
    else clearWarning();
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
		am.initialise(0, 2, elem.get(), true);
	}

	updateAudioSetup();

	Interface::loadJSONDataInternal(data);

	AudioDeviceManager::AudioDeviceSetup setup;
	am.getAudioDeviceSetup(setup);
	if (setup.outputDeviceName.isEmpty()) setWarningMessage("This interface isn't connected to an audio output");
	else clearWarning();
}

void AudioInterface::changeListenerCallback(ChangeBroadcaster*)
{
	updateAudioSetup();
    outputs.refreshAllOutputsChannelOptions();
}
