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

namespace
{
    // The block stands in for the file list of the cue the section renders.
    Inspectable* getFilesAnchor(const Array<Cue*>& audioCues)
    {
        if (audioCues.isEmpty()) return nullptr;
        auto* ac = dynamic_cast<AudioCue*>(audioCues.getFirst());
        if (ac != nullptr && ac->filesManager != nullptr) return ac->filesManager;
        return audioCues.getFirst();
    }
}

AudioMultiFilesEditor::AudioMultiFilesEditor(const Array<Cue*>& audioCues) :
    CueMultiBulkEditor(getFilesAnchor(audioCues), audioCues, "Audio Files (all)", true)
{
    proxyOutput = proxy->addTargetParameter("Audio Output", "Set the audio output of every file of the selected cues", InterfaceManager::getInstance());
    proxyOutput->targetType = TargetParameter::CONTAINER;
    proxyOutput->customGetTargetContainerFunc = &InterfaceManager::showMenuForTargetAudioOutput;

    // Left at its minimum this volume changes nothing: it is only applied once moved, so a
    // group edit never silently overwrites each file's own level.
    proxyVolume = proxy->addFloatParameter("Files Volume", "Move to apply this volume to every file of the selected cues; left at minimum, each file keeps its own value", 0.0, 0.0, 1.5, 0.01);

    // Seed the output with the first file's value, for a representative display.
    for (auto& w : cues)
    {
        auto* ac = dynamic_cast<AudioCue*>(w.get());
        if (ac != nullptr && ac->filesManager != nullptr && ac->filesManager->items.size() > 0)
        {
            AudioFile* f = ac->filesManager->items.getFirst();
            if (f != nullptr) proxyOutput->setValue(f->targetAudioInterface->getValue());
            break;
        }
    }

    buildProxyEditor();
}

AudioMultiFilesEditor::~AudioMultiFilesEditor()
{
}

void AudioMultiFilesEditor::applyProxyChange(Parameter* changedProxyParam)
{
    const bool isOutput = (changedProxyParam == proxyOutput);
    const bool isVolume = (changedProxyParam == proxyVolume);
    if (!isOutput && !isVolume) return;

    Array<Parameter*> targets;
    for (auto& w : cues)
    {
        auto* ac = dynamic_cast<AudioCue*>(w.get());
        if (ac == nullptr || ac->filesManager == nullptr) continue;

        for (auto& file : ac->filesManager->items)
        {
            if (file == nullptr) continue;
            targets.add(isOutput ? (Parameter*)file->targetAudioInterface : (Parameter*)file->volume);
        }
    }

    applyToParameters(targets, changedProxyParam->getValue(),
                      isOutput ? "Edit audio files output" : "Edit audio files volume");
}
