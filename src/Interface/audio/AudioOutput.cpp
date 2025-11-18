/**
    AudioOutput.cpp
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#include "AudioOutput.h"
#include "AudioInterface.h"
#include "ui/AudioOutputEditor.h"

AudioOutput::AudioOutput(var params, AudioInterface* audioInterface) :
    BaseItem("Output 1"),
    audioInterface(audioInterface),
    am()
{
    saveAndLoadRecursiveData = true;
    userCanRemove = true;
    canBeDisabled = false;
    showWarningInUI = true;

    testTrigger = addTrigger("Test", "Send a test sound to this output");
    testTrigger->hideInEditor = true;
    testTrigger->setEnabled(false);

    outputType = addEnumParameter("Output Type", "Type of audio output");
    outputType->lockManualControlMode = true;
    outputType->addOption("Mono", "m")->addOption("Stereo", "s");
    outputType->isSavable = false;

    chan1 = addEnumParameter("Channel 1", "Audio channel 1");
    chan1->warningResolveInspectable = audioInterface;
    chan1->lockManualControlMode = true;
    chan1->showWarningInUI = true;
    chan1->isSavable = false;

    chan2 = addEnumParameter("Channel 2", "Audio channel 2");
    chan2->warningResolveInspectable = audioInterface;
    chan2->lockManualControlMode = true;
    chan2->hideInEditor = true;
    chan2->showWarningInUI = true;
    chan2->isSavable = false;

    initAudioSetup();
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::refreshChannelOptions()
{
    AudioDeviceManager::AudioDeviceSetup setup = audioInterface->am.getAudioDeviceSetup();
    AudioIODevice* currentDevice = audioInterface->am.getCurrentAudioDevice();

    // Reset options for channels!
    chan1->clearOptions();
    chan2->clearOptions();

    chan1->addOption("-- Select an output --", 0);
    chan2->addOption("-- Select an output --", 0);

    if (currentDevice == nullptr) return;

    StringArray items = currentDevice->getOutputChannelNames();
    for (int i = 0; i < items.size(); i++) {
        if (setup.outputChannels[i] == false) continue; //skip unavailable channels
        chan1->addOption(String(i + 1) + ") " + String(items[i]), (i + 1));
        chan2->addOption(String(i + 1) + ") " + String(items[i]), (i + 1));
    }

    // Try to restore previous selections
    int tmpSavedChan1 = savedChan1;
    int tmpSavedChan2 = savedChan2;
    chan1->setValueWithData(savedChan1);
    chan2->setValueWithData(savedChan2);

    chan1->clearWarning();
    chan2->clearWarning();

    testTrigger->setEnabled(true);

    if (chan1->intValue() != tmpSavedChan1) {
        chan1->setValue(0);
        chan1->setWarningMessage(tmpSavedChan1 > 0 ? "Previously selected channel isn't available anymore" : "No channel selected");
        savedChan1 = tmpSavedChan1;
        testTrigger->setEnabled(false);
    }

    if (outputType->getValue() == "s" && chan2->intValue() != tmpSavedChan2) {
        chan2->setValue(0);
        chan2->setWarningMessage(tmpSavedChan2 > 0 ? "Previously selected channel isn't available anymore" : "No channel selected");
        savedChan2 = tmpSavedChan2;
        testTrigger->setEnabled(false);
    }
}

void AudioOutput::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == outputType)
    {
        String type = outputType->getValue();
        if (type == "m") {
            chan1->hideInEditor = false;
            chan2->hideInEditor = true;
            chan2->setValueWithData(0);
            setupAudioOutputs();
            clearWarning();
        } else if (type == "s") {
            chan1->hideInEditor = false;
            chan2->hideInEditor = false;
            chan2->setValueWithData(savedChan2);
            setupAudioOutputs();
            clearWarning();
        } else {
            chan1->hideInEditor = true;
            chan2->hideInEditor = true;
            setWarningMessage("Unknown output type selected");
        }
        queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
    }

    if (p == chan1 && chan1->intValue() > 0) {
        savedChan1 = chan1->intValue();
        chan1->clearWarning();
        setupAudioOutputs();
    }
    if (p == chan2 && chan2->intValue() > 0) {
        savedChan2 = chan2->intValue();
        chan2->clearWarning();
        setupAudioOutputs();
    }

    if (chan1->intValue() == 0 || (outputType->getValue() == "s" && chan2->intValue() == 0)) {
        testTrigger->setEnabled(false);
    } else {
        testTrigger->setEnabled(true);
    }
}

void AudioOutput::resolveWarning()
{
    this->parentContainer->parentContainer->selectThis();
}

void AudioOutput::onContainerTriggerTriggered(Trigger* t)
{
    if (t == testTrigger) {
        playTestSound();
    }
}

void AudioOutput::initAudioSetup()
{
	if (Thread::getCurrentThreadId() != MessageManager::getInstance()->getCurrentMessageThread())
	{
		MessageManager::callAsync([this]() { initAudioSetup(); });
		return;
	}
	am.initialiseWithDefaultDevices(0, 2);
    refreshChannelOptions();
    setupAudioOutputs();
}

void AudioOutput::setupAudioOutputs()
{
    AudioDeviceManager::AudioDeviceSetup setup = audioInterface->am.getAudioDeviceSetup();
    AudioDeviceManager::AudioDeviceSetup newSetup;

    BigInteger outputs;

    outputs.setBit(chan1->intValue() - 1, true);
    if (outputType->getValue() == "s" && chan2->intValue() > 0) {
        outputs.setBit(chan2->intValue() - 1, true);
    }

    newSetup.outputDeviceName = setup.outputDeviceName;
    newSetup.sampleRate = setup.sampleRate;
    newSetup.bufferSize = setup.bufferSize;
    newSetup.useDefaultOutputChannels = false;
    newSetup.outputChannels = outputs;

	am.initialise(0, outputType->getValue() == "s" ? 2 : 1, nullptr, true);
    am.setAudioDeviceSetup(newSetup, true);
}

void AudioOutput::playTestSound()
{
    am.playTestSound();
}

InspectableEditor* AudioOutput::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
	return new AudioOutputEditor(this, isRoot);
}


var AudioOutput::getJSONData(bool includeNonOverriden)
{
    var data = BaseItem::getJSONData(includeNonOverriden);
    data.getDynamicObject()->setProperty("type", outputType->getValue());
    data.getDynamicObject()->setProperty("ch1", chan1->intValue());
    if (outputType->getValue() == "s")
        data.getDynamicObject()->setProperty("ch2", chan2->intValue());
    return data;
}

void AudioOutput::loadJSONDataInternal(var data)
{
    BaseItem::loadJSONDataInternal(data);
    outputType->setValueWithData(data.getProperty("type", "m"));
    savedChan1 = data.getProperty("ch1", 0);
    chan1->setValueWithData(savedChan1);
    savedChan2 = data.getProperty("ch2", 0);
    chan2->setValueWithData(savedChan2);
}

AudioOutputManager::AudioOutputManager(AudioInterface* audioInterface) :
    BaseManager<AudioOutput>("Outputs"),
    audioInterface(audioInterface)
{
    selectItemWhenCreated = false;
}

AudioOutputManager::~AudioOutputManager()
{
}

AudioOutput * AudioOutputManager::createItem()
{
	return new AudioOutput(var(), audioInterface);
}

void AudioOutputManager::refreshAllOutputsChannelOptions()
{
    for (int i = 0; i < items.size(); i++) {
        items[i]->refreshChannelOptions();
    }
}
