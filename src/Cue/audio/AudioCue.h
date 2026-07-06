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

class AudioFilesManager;
class AudioOutput;
class AudioSlicesManager;
class AudioWaveformSlicer;
class PluginChainManager;
class MIDIInterface;
class MTCSender;

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

    // MTC (MIDI Time Code) — playback position emitted as SMPTE quarter frames.
    std::unique_ptr<MTCSender> mtcSender;

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
    FloatParameter* duckFadeInCurrentTime;   // read-only, display: fade-in (restore) progress
    BoolParameter*  duckActive;              // read-only, hidden
    BoolParameter*  duckPrePlayActive;       // read-only, hidden: pre-play delay running
    BoolParameter*  duckPostPlayActive;      // read-only, hidden: post-play delay running
    BoolParameter*  duckFadeInActive;        // read-only, hidden: fade-in (restore) running

    Cue::CueTimer* duckPrePlayTimer = nullptr;
    Cue::CueTimer* duckPostPlayTimer = nullptr;
    Cue::CueTimer* duckFadeInTimer = nullptr;

    void startAudioPlayback();       // the actual audio start (former playInternal body)
    void startDuckSequence();        // fade others out + start pre-play timer
    void startDuckRestore();         // start post-play timer after the audio ends naturally
    void fadeOthersBackIn();         // fade(1, fadeIn) on the other playing cues
    void startFadeInPhase();         // fade the others back in, tracking the fade-in as a phase
    void cancelDuckSequence(bool fadeInImmediately); // stop/panic during the sequence
    Array<Cue*> getOtherPlayingAudioCues();          // playing audio/playlist cues except this
    // Gain a cue starting now should adopt because another cue is currently ducking others
    // (the lowest active duck volume), or 1.0 if none is ducking. `except` is the starting cue.
    static double getActiveDuckGain(Cue* except);

    void onCueTimerFinished(Cue::CueTimer* timer) override;

    void refreshVolume(); // push the master volume down to every file

    // Volume fader (Active Cues panel).
    bool  hasVolumeFader() override { return true; }
    float getFaderVolume() override;
    float getMaxFaderVolume() override { return 1.5f; }
    void  setFaderVolume(float v) override;
    float getOutputLevel() override;

    String getTypeString() const override { return "Audio Cue"; }
    String getCueType() const override { return "Audio"; }
    MTCSender* getMTCSender() override { return mtcSender.get(); }
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

    void createFromFiles(const StringArray& files);

private:
    void timerCallback() override;
};
