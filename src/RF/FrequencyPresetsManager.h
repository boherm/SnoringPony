/*
  ==============================================================================

    FrequencyPresetsManager.h
    Created: 29 Jun 2026
    Author:  boherm

    The list of a profile's frequency presets. A plain BaseManager, but with a
    custom inspector editor that adds an "Import" button (paste many frequencies
    at once). See FrequencyPresetsManagerEditor.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "FrequencyPreset.h"

class FrequencyPresetsManager :
    public BaseManager<FrequencyPreset>
{
public:
    FrequencyPresetsManager();
    virtual ~FrequencyPresetsManager();

    InspectableEditor* getEditorInternal(bool isRoot,
        juce::Array<Inspectable*> inspectables = juce::Array<Inspectable*>()) override;
};
