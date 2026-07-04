/*
  ==============================================================================

    AudioMultiFilesEditor.cpp
    Created: 04 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "AudioMultiFilesEditor.h"
#include "../AudioCue.h"
#include "../AudioFile.h"
#include "../../../Interface/InterfaceManager.h"

AudioMultiFilesEditor::AudioMultiFilesEditor(const juce::Array<Cue*>& audioCues)
{
    for (auto* c : audioCues)
        if (c != nullptr) cues.add(c);

    proxy.reset(new ControllableContainer("Files (all)"));

    proxyOutput = proxy->addTargetParameter("Audio Output", "Set the audio output of every file of the selected cues", InterfaceManager::getInstance());
    proxyOutput->targetType = TargetParameter::CONTAINER;
    proxyOutput->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;

    proxyVolume = proxy->addFloatParameter("Volume", "Set the volume of every file of the selected cues", 1.0, 0.0, 1.5, 0.01);

    // Seed the controls with the first file's values, for a representative display.
    for (auto& w : cues)
    {
        auto* ac = dynamic_cast<AudioCue*>(w.get());
        if (ac != nullptr && ac->filesManager != nullptr && ac->filesManager->items.size() > 0)
        {
            AudioFile* f = ac->filesManager->items.getFirst();
            if (f != nullptr)
            {
                proxyVolume->setValue(f->volume->floatValue());
                proxyOutput->setValue(f->targetAudioInterface->getValue());
            }
            break;
        }
    }

    proxyEditor.reset(new GenericControllableContainerEditor(juce::Array<ControllableContainer*>({ proxy.get() }), false));
    addAndMakeVisible(proxyEditor.get());

    proxy->addAsyncContainerListener(this);

    setSize(100, 10);
}

AudioMultiFilesEditor::~AudioMultiFilesEditor()
{
    if (proxy != nullptr) proxy->removeAsyncContainerListener(this);
    proxyEditor.reset();
    proxy.reset();
}

void AudioMultiFilesEditor::resized()
{
    if (getWidth() == 0 || proxyEditor == nullptr) return;

    proxyEditor->setSize(getWidth(), proxyEditor->getHeight());
    proxyEditor->setTopLeftPosition(0, 0);

    int h = jmax(proxyEditor->getHeight(), 10);
    if (getHeight() != h) setSize(getWidth(), h);
}

void AudioMultiFilesEditor::childBoundsChanged(juce::Component* c)
{
    if (proxyEditor != nullptr && getWidth() != 0) setSize(getWidth(), jmax(proxyEditor->getHeight(), 10));
}

void AudioMultiFilesEditor::newMessage(const ContainerAsyncEvent& e)
{
    if (isApplying) return;
    if (e.type != ContainerAsyncEvent::ControllableFeedbackUpdate) return;

    Controllable* c = e.targetControllable;
    if (c == nullptr || e.targetControllable.wasObjectDeleted()) return;

    const bool isOutput = (c == proxyOutput);
    const bool isVolume = (c == proxyVolume);
    if (!isOutput && !isVolume) return;

    const var newVal = isOutput ? proxyOutput->getValue() : proxyVolume->getValue();

    Array<UndoableAction*> actions;
    for (auto& w : cues)
    {
        auto* ac = dynamic_cast<AudioCue*>(w.get());
        if (ac == nullptr || ac->filesManager == nullptr) continue;

        for (auto& file : ac->filesManager->items)
        {
            if (file == nullptr) continue;
            Parameter* target = isOutput ? (Parameter*)file->targetAudioInterface : (Parameter*)file->volume;
            if (target->getValue() == newVal) continue;
            if (UndoableAction* a = target->setUndoableValue(target->getValue(), newVal, true))
                actions.add(a);
        }
    }

    if (actions.isEmpty()) return;

    isApplying = true;
    UndoMaster::getInstance()->performActions("Edit audio files", actions);
    isApplying = false;
}
