/*
  ==============================================================================

    AudioCue.h
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"
#include "../../Interface/midi/MTCEncoder.h"

class AudioFilesManager;
class AudioOutput;
class AudioSlicesManager;
class AudioWaveformSlicer;
class PluginChainManager;
class MIDIInterface;

class AudioCue :
    public Cue,
    public ContainerAsyncListener,
    public Timer,
    public ChangeListener
{
public:
    AudioCue(var params = var());
    virtual ~AudioCue();

    std::unique_ptr<ControllableContainer> audioSlicer;
    std::unique_ptr<AudioWaveformSlicer> waveformSlicer;
    std::unique_ptr<AudioSlicesManager> slicesManager;

    AudioFilesManager* filesManager;
    PluginChainManager* pluginChainManager;
    AudioFormatManager formatManager;

    bool askedToStop = false;

    FloatParameter* initialDuration;
    BoolParameter* loop;

    // MTC
    EnablingControllableContainer* mtcCC;
    TargetParameter* mtcMidiInterface;
    FloatParameter* mtcOffset;
    EnumParameter* mtcFrameRate;

    class MTCTimer : public juce::HighResolutionTimer
    {
    public:
        MTCTimer(AudioCue& owner);
        void hiResTimerCallback() override;

        AudioCue& owner;
        int currentPiece = 0;
        double lastFrameTime = 0.0;
    };
    std::unique_ptr<MTCTimer> mtcTimer;

    static HashMap<MIDIInterface*, AudioCue*> activeMTCSenders;

    String getTypeString() const override { return "Audio Cue"; }
    String getCueType() const override { return "Audio"; }
    static AudioCue* create(var params) { return new AudioCue(params); }

    void newMessage(const ContainerAsyncEvent& e) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

    void updateWarnings();
    bool canBePlayed() override;

    void playInternal() override;
    void previewInternal() override;
    bool canBePreviewed() override { return true; }
    void stopInternal() override;
    void retriggerStop() override;
    void panicInternal() override;

    void fade(double targetGain, double duration) override;
    void fadeIn(double duration);
    void fadeOut(double duration, bool stopAfterFade = true);

    String autoDescriptionInternal() override;

    void startMTC();
    void stopMTC();
    MIDIInterface* getMTCInterface();

    void createFromFiles(const StringArray& files);

private:
    void timerCallback() override;
};
