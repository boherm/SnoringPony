/*
  ==============================================================================

    FrequencyPresetsManager.cpp
    Created: 29 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "FrequencyPresetsManager.h"
#include "ui/FrequencyPresetsManagerEditor.h"

FrequencyPresetsManager::FrequencyPresetsManager() :
    BaseManager<FrequencyPreset>("Presets")
{
}

FrequencyPresetsManager::~FrequencyPresetsManager()
{
}

InspectableEditor* FrequencyPresetsManager::getEditorInternal(bool isRoot, juce::Array<Inspectable*> inspectables)
{
    return new FrequencyPresetsManagerEditor(this, isRoot);
}
