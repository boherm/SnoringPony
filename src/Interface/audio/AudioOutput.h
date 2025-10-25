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

    EnumParameter* outputType;
    EnumParameter* chan1;
    EnumParameter* chan2;

    int savedChan1 = 0;
    int savedChan2 = 0;

	void onContainerParameterChangedInternal(Parameter *) override;

    void resolveWarning() override;

    void refreshChannelOptions();

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
