/*
  ==============================================================================

    AudioSettings.h
    Created: 30 Dec 2025 01:43:20pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class AudioSettings :
    public ControllableContainer
{

public:
    AudioSettings();
    virtual ~AudioSettings() {}

    TargetParameter* previewOutput;

    void clear();
};
