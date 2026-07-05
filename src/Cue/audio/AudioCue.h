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
    FloatParameter* volume; // master volume, combined with each file's own volume

    // MTC
    EnablingControllableContainer* mtcCC;
    TargetParameter* mtcMidiInterface;
    FloatParameter* mtcOffset;
    EnumParameter* mtcFrameRate;
    StringParameter* mtcTimecodeDisplay;

    // --- Duck others ---
    // When enabled, launching this cue fades out every other playing audio/playlist cue,
    // waits (pre-play), plays this cue alone, then (post-play) fades the others back in.
    EnablingControllableContainer* duckOthersCC;
    FloatParameter* duckVolume;              // gain the other cues are ducked down to (0..1)
    FloatParameter* duckFadeOutDuration;     // fade out applied to the other cues on launch
    FloatParameter* duckPrePlayDuration;     // delay before this cue's audio starts
    FloatParameter* duckPostPlayDuration;    // delay after this cue's audio ends
    FloatParameter* duckFadeInDuration;      // fade in used to bring the others back
    FloatParameter* duckPrePlayCurrentTime;  // read-only, display
    FloatParameter* duckPostPlayCurrentTime; // read-only, display
    BoolParameter*  duckActive;              // read-only, hidden
    BoolParameter*  duckPrePlayActive;       // read-only, hidden: pre-play delay running
    BoolParameter*  duckPostPlayActive;      // read-only, hidden: post-play delay running

    Cue::CueTimer* duckPrePlayTimer = nullptr;
    Cue::CueTimer* duckPostPlayTimer = nullptr;

    void startAudioPlayback();       // the actual audio start (former playInternal body)
    void startDuckSequence();        // fade others out + start pre-play timer
    void startDuckRestore();         // start post-play timer after the audio ends naturally
    void fadeOthersBackIn();         // fade(1, fadeIn) on the other playing cues
    void cancelDuckSequence(bool fadeInImmediately); // stop/panic during the sequence
    Array<Cue*> getOtherPlayingAudioCues();          // playing audio/playlist cues except this

    void onCueTimerFinished(Cue::CueTimer* timer) override;

    void refreshVolume(); // push the master volume down to every file

    // Volume fader (Active Cues panel).
    bool  hasVolumeFader() override { return true; }
    float getFaderVolume() override;
    float getMaxFaderVolume() override { return 1.5f; }
    void  setFaderVolume(float v) override;
    float getOutputLevel() override;

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

    // The interface this cue is currently sending MTC on (may differ from the selected
    // one right after an interface change), used to clean up the right entry on stop.
    MIDIInterface* mtcActiveInterface = nullptr;

    static HashMap<MIDIInterface*, AudioCue*> activeMTCSenders;

    String getTypeString() const override { return "Audio Cue"; }
    String getCueType() const override { return "Audio"; }
    juce::StringArray getMultiEditHiddenControllableNames() const override;
    juce::Component* createMultiEditExtraEditor(const juce::Array<Cue*>& scopeCues) override;
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
    void fadeAndStop(double fadeTime) override;
    void panicInternal() override;

    bool canSeekTo() const override;
    void seekToTime(double t) override;

    void fade(double targetGain, double duration) override;
    void fadeIn(double duration);
    void fadeOut(double duration, bool stopAfterFade = true);

    String autoDescriptionInternal() override;

    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
    void parameterValueChanged(Parameter* p) override;

    void startMTC();
    void stopMTC();
    MIDIInterface* getMTCInterface();

    void createFromFiles(const StringArray& files);

private:
    void timerCallback() override;
};
